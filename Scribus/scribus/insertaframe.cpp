/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
//
//
// Author: Craig Bradney <cbradney@zip.com.au>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "insertaframe.h"
//#include "insertaframe.moc"

#include "createrange.h"
#include "customfdialog.h"
#include "scrspinbox.h"
#include "prefsfile.h"
#include "prefsmanager.h"
#include "scribusdoc.h"
#include "util.h"
#include "scpaths.h"

#include <q3buttongroup.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <q3textedit.h>
#include <q3widgetstack.h>

InsertAFrame::InsertAFrame(QWidget* parent, ScribusDoc *doc) :
	QDialog(parent),
	m_Doc(doc)
{
	setupUi(this);
	//Hide some unused items for now
	radioButtonTable->setShown(false);
	radioButtonShape->setShown(false);
	radioButtonPolygon->setShown(false);
	
	placementPagesRangeButton->setPixmap(loadIcon("ellipsis.png"));
	
	//set tab order
	QWidget::setTabOrder(radioButtonCustomPosition, xPosScrSpinBox);
	QWidget::setTabOrder(xPosScrSpinBox, yPosScrSpinBox);
	QWidget::setTabOrder(radioButtonCustomSize, widthScrSpinBox);
	QWidget::setTabOrder(widthScrSpinBox, heightScrSpinBox);
	QWidget::setTabOrder(textColumnCountSpinBox, textColumnGapScrSpinBox);
	
	typeButtonGroup->setButton(0);
	pagePlacementButtonGroup->setButton(0);
	framePositionButtonGroup->setButton(0);
	sizeButtonGroup->setButton(0);
	slotSelectType(0);
	slotSelectPagePlacement(0);
	slotSelectPosition(0);
	slotSelectSize(0);
	
	int docUnitIndex = m_Doc->unitIndex();
	int decimals = unitGetPrecisionFromIndex(docUnitIndex);
	QString unitSuffix(unitGetSuffixFromIndex(docUnitIndex));
	xPosScrSpinBox->setValues(0.0, 1000.0, decimals, 0.0);
	yPosScrSpinBox->setValues(0.0, 1000.0, decimals, 0.0);
	widthScrSpinBox->setValues(0.0, 1000.0, decimals, 0.0);
	heightScrSpinBox->setValues(0.0, 1000.0, decimals, 0.0);
	textColumnGapScrSpinBox->setValues(0.0, 1000.0, decimals, 0.0);
	xPosScrSpinBox->setSuffix(unitSuffix);
	yPosScrSpinBox->setSuffix(unitSuffix);
	widthScrSpinBox->setSuffix(unitSuffix);
	heightScrSpinBox->setSuffix(unitSuffix);
	textColumnGapScrSpinBox->setSuffix(unitSuffix);

	sourceDocLineEdit->setText("");
 	connect(typeButtonGroup, SIGNAL(clicked(int)), this, SLOT(slotSelectType(int)));
 	connect(pagePlacementButtonGroup, SIGNAL(clicked(int)), this, SLOT(slotSelectPagePlacement(int)));
	connect(placementPagesRangeButton, SIGNAL(clicked()), this, SLOT(slotCreatePageNumberRange()));
 	connect(framePositionButtonGroup, SIGNAL(clicked(int)), this, SLOT(slotSelectPosition(int)));
 	connect(sizeButtonGroup, SIGNAL(clicked(int)), this, SLOT(slotSelectSize(int)));
 	connect(selectImageFileButton, SIGNAL(clicked()), this, SLOT(locateImageFile()));
 	connect(selectDocFileButton, SIGNAL(clicked()), this, SLOT(locateDocFile()));
}

void InsertAFrame::slotSelectType( int id )
{
	checkBoxLinkCreatedTextFrames->setEnabled(id==0);
	radioButtonImageSize->setShown(id==1);
	switch (id)
	{
		case 0:
			typeTextEdit->setText( tr("<b>Insert a text frame</b><br/>A text frame allows you to enter any text in a defined position with the formatting you choose. You may select a text file on the Options tab if you want to immediately import a document into the frame. Scribus supports a wide variety of importable format from plain text to OpenOffice.org.<br/>Your text may be edited and formatted on the page directly or in the simple Story Editor."));
			optionsWidgetStack->raiseWidget(2);
			break;
		case 1:
			typeTextEdit->setText( tr("<b>Insert an image frame</b><br/>An image frame allows you to place an image onto your page. Various image effects may be applied or combined including transparencies, brightness, posterisation that allow retouching or the creation of interesting visual results. Image scaling and shaping is performed with the Properties Palette."));
			optionsWidgetStack->raiseWidget(0);
			break;
		case 2:
			typeTextEdit->setText("Insert a table");
			optionsWidgetStack->raiseWidget(1);
			break;
		case 3:
			typeTextEdit->setText("Insert a shape");
			optionsWidgetStack->raiseWidget(1);
			break;
		case 4:
			typeTextEdit->setText("Insert a polygon");
			optionsWidgetStack->raiseWidget(1);
			break;
		case 5:
			typeTextEdit->setText("Insert a line");
			optionsWidgetStack->raiseWidget(1);
			break;
		case 6:
			typeTextEdit->setText("Insert a bezier curve");
			optionsWidgetStack->raiseWidget(1);
			break;
	}
}

void InsertAFrame::slotSelectPagePlacement( int id )
{
	placementPagesLineEdit->setEnabled(id==1);
	placementPagesRangeButton->setEnabled(id==1);
	checkBoxLinkCreatedTextFrames->setEnabled(typeButtonGroup->selectedId()==0 && (id!=0));
}

void InsertAFrame::slotSelectPosition( int id )
{
	xPosScrSpinBox->setEnabled(id==99);
	yPosScrSpinBox->setEnabled(id==99);
}

void InsertAFrame::slotSelectSize( int id )
{
	widthScrSpinBox->setEnabled(id==99);
	heightScrSpinBox->setEnabled(id==99);
}

void InsertAFrame::getNewFrameProperties(InsertAFrameData &iafData)
{
	int type=typeButtonGroup->selectedId();
	iafData.source="";
	switch(type)
	{
		case 0:
			iafData.frameType=PageItem::TextFrame;
			iafData.source=ScPaths::separatorsToSlashes(sourceDocLineEdit->text());
			break;
		case 1:
			iafData.frameType=PageItem::ImageFrame;
			iafData.source=ScPaths::separatorsToSlashes(sourceImageLineEdit->text());
			break;
	}
	iafData.locationType=pagePlacementButtonGroup->selectedId();
	iafData.pageList=placementPagesLineEdit->text();
	iafData.positionType=framePositionButtonGroup->selectedId();
	iafData.sizeType=sizeButtonGroup->selectedId();
	iafData.x=xPosScrSpinBox->value();
	iafData.y=yPosScrSpinBox->value();
	iafData.width=widthScrSpinBox->value();
	iafData.height=heightScrSpinBox->value();
	iafData.impsetup=m_ImportSetup;
	iafData.columnCount=textColumnCountSpinBox->value();
	iafData.columnGap=textColumnGapScrSpinBox->value();
	iafData.linkTextFrames=checkBoxLinkCreatedTextFrames->isChecked();
}

void InsertAFrame::locateImageFile()
{
	QString formatD(setupImageFormats());
	QString docDir = ".";
	PrefsManager* prefsManager=PrefsManager::instance();
	QString prefsDocDir(prefsManager->documentDir());
	if (!prefsDocDir.isEmpty())
		docDir = prefsManager->prefsFile->getContext("dirs")->get("images", prefsDocDir);
	else
		docDir = prefsManager->prefsFile->getContext("dirs")->get("images", ".");
		
	QString fileName("");
	CustomFDialog dia(this, docDir, tr("Open"), formatD, fdShowPreview | fdExistingFiles);
	if (dia.exec() == QDialog::Accepted)
		fileName = dia.selectedFile();
	
	sourceImageLineEdit->setText(QDir::convertSeparators(fileName));
}

void InsertAFrame::locateDocFile()
{
	m_ImportSetup.runDialog=false;
	gtGetText* gt = new gtGetText(m_Doc);
	m_ImportSetup=gt->run();
	if (m_ImportSetup.runDialog)
		sourceDocLineEdit->setText(QDir::convertSeparators(m_ImportSetup.filename));
	delete gt;
}

void InsertAFrame::slotCreatePageNumberRange( )
{
	if (m_Doc!=0)
	{
		CreateRange cr(placementPagesLineEdit->text(), m_Doc->DocPages.count(), this);
		if (cr.exec())
		{
			CreateRangeData crData;
			cr.getCreateRangeData(crData);
			placementPagesLineEdit->setText(crData.pageRange);
			return;
		}
	}
	placementPagesLineEdit->setText(QString::null);
}

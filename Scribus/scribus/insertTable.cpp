/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "insertTable.h"
//#include "insertTable.moc"

#include "commonstrings.h"
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <QPixmap>
#include <Q3HBoxLayout>
#include <QLabel>

extern QPixmap loadIcon(QString nam);

InsertTable::InsertTable( QWidget* parent, int maxRow, int maxCol )
		: QDialog( parent, "InsertTable", true, 0 )

{
	setCaption( tr( "Insert Table" ) );
	setIcon(loadIcon("AppIcon.png"));

	InsertTableLayout = new Q3VBoxLayout( this, 11, 8, "InsertTableLayout");
	layout2 = new Q3GridLayout( 0, 1, 1, 0, 6, "layout2");
	Cols = new QSpinBox( 1, maxCol, 1, this, "Cols" );
	layout2->addWidget( Cols, 1, 1 );
	Text1 = new QLabel( tr( "Number of rows:" ), this, "Text1" );
	Text2 = new QLabel( tr( "Number of columns:" ), this, "Text2" );
	layout2->addWidget( Text1, 0, 0 );
	layout2->addWidget( Text2, 1, 0 );
	Rows = new QSpinBox( 1, maxRow, 1, this, "Rows" );
	layout2->addWidget( Rows, 0, 1 );
	InsertTableLayout->addLayout( layout2 );
	layout1 = new Q3HBoxLayout( 0, 0, 6, "layout1");
	okButton = new QPushButton( CommonStrings::tr_OK, this, "okButton" );
	cancelButton = new QPushButton( CommonStrings::tr_Cancel, this, "cancelButton" );
	okButton->setDefault( true );
	layout1->addWidget( okButton );
	layout1->addWidget( cancelButton );
	InsertTableLayout->addLayout( layout1 );
	resize( QSize(200, 111).expandedTo(minimumSizeHint()) );

	setTabOrder ( Rows, Cols );
	setTabOrder ( Cols, okButton );
	setTabOrder ( okButton, cancelButton);
	setTabOrder ( cancelButton, Rows );
	Rows->setFocus();
	// signals and slots connections
	connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
}


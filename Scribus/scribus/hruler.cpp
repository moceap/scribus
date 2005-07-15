/***************************************************************************
                          hruler.cpp  -  description
                             -------------------
    begin                : Tue Apr 10 2001
    copyright            : (C) 2001 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "hruler.h"
#include "hruler.moc"
#include "page.h"
#include <qcursor.h>
#include <qcolor.h>
#include <qrect.h>
#include <qpointarray.h>
#include "scribusview.h"
#include "scribusdoc.h"
#include "scribus.h"
#include "units.h"
#include "prefsmanager.h"

#if QT_VERSION  > 0x030102
	#define SPLITHC SplitVCursor
#else
	#define SPLITHC SplitHCursor
#endif

extern QPixmap loadIcon(QString nam);
extern ScribusApp* ScApp;

Hruler::Hruler(ScribusView *pa, ScribusDoc *doc) : QWidget(pa)
{
	prefsManager=PrefsManager::instance();
	setEraseColor(QColor(255,255,255));
	currDoc = doc;
	currView = pa;
	offs = -10;
	Markp = 0;
	repX = false;
	Mpressed = false;
	ItemPosValid = false;
	RulerCode = 0;
	setMouseTracking(true);
	unitChange();
}

void Hruler::mousePressEvent(QMouseEvent *m)
{
	Mpressed = true;
	if (currDoc->loading)
		return;
	if (ItemPosValid)
	{
		RulerCode = 0;
		Markp = -1;
		double Pos = (ItemPos+Extra+lineCorr+Offset)*Scaling-offs;
		if ((static_cast<int>(Pos) < (m->x()+currDoc->guidesSettings.grabRad)) && (static_cast<int>(Pos) > (m->x()-currDoc->guidesSettings.grabRad)))
			RulerCode = 1;
		Pos = (ItemEndPos-RExtra-lineCorr+Offset)*Scaling-offs;
		if ((static_cast<int>(Pos) < (m->x()+currDoc->guidesSettings.grabRad)) && (static_cast<int>(Pos) > (m->x()-currDoc->guidesSettings.grabRad)))
			RulerCode = 2;
		double ColWidth = (ItemEndPos - ItemPos - (ColGap * (Cols - 1)) - Extra - RExtra - 2*lineCorr) / Cols;
		QRect fpo;
		ActCol = 0;
		for (int CurrCol = 0; CurrCol < Cols; ++CurrCol)
		{
			Pos = (ItemPos+(ColWidth+ColGap)*CurrCol+Offset+Extra+lineCorr)*Scaling-offs;
			fpo = QRect(static_cast<int>(Pos), 11, static_cast<int>(ColWidth*Scaling), 12);
			if (fpo.contains(m->pos()))
			{
				ActCol = CurrCol+1;
				break;
			}
		}
		if (ActCol == 0)
			return;
		if (currDoc->currentParaStyle > 4)
		{
			Pos = (ItemPos+First+Indent+(ColWidth+ColGap)*(ActCol-1)+Offset+Extra+lineCorr)*Scaling-offs;
			fpo = QRect(static_cast<int>(Pos)-3, 11, 6, 6);
			if (fpo.contains(m->pos()))
			{
				RulerCode = 3;
				MouseX = m->x();
				return;
			}
			Pos = (ItemPos+Indent+(ColWidth+ColGap)*(ActCol-1)+Offset+Extra+lineCorr)*Scaling-offs;
			fpo = QRect(static_cast<int>(Pos)-3, 17, 6, 6);
			if (fpo.contains(m->pos()))
			{
				RulerCode = 4;
				MouseX = m->x();
				return;
			}
		}
		if (TabValues.count() != 0)
		{
			for (int yg = 0; yg < static_cast<int>(TabValues.count()); yg++)
			{
				Pos = (ItemPos+TabValues[yg].tabPosition+(ColWidth+ColGap)*(ActCol-1)+Offset+Extra+lineCorr)*Scaling-offs;
				fpo = QRect(static_cast<int>(Pos)-3, 15, 8, 8);
				if (fpo.contains(m->pos()))
				{
					RulerCode = 5;
					ActTab = yg;
					break;
				}
			}
		}
		if ((RulerCode == 0) && (ActCol != 0) && (m->button() == LeftButton))
		{
			double Pos = (ItemPos+Extra+lineCorr+Offset)*Scaling-offs;
			int newY = m->x() - static_cast<int>(Pos);
			struct PageItem::TabRecord tb;
			tb.tabPosition = newY / Scaling;
			tb.tabType = 0;
			tb.tabFillChar = currDoc->toolSettings.tabFillChar[0];
			TabValues.prepend(tb);
			ActTab = 0;
			RulerCode = 5;
			UpdateTabList();
			qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
			emit DocChanged(false);
		}
		MouseX = m->x();
	}
	else
	{
		if (prefsManager->appPrefs.guidesSettings.guidesShown)
		{
			QPoint py = currView->viewport()->mapFromGlobal(m->globalPos());
			currView->DrHY = py.y();
			qApp->setOverrideCursor(QCursor(SPLITHC), true);
		}
	}
}

void Hruler::mouseReleaseEvent(QMouseEvent *m)
{
	if (currDoc->loading)
	{
		Mpressed = false;
		return;
	}
	if (ItemPosValid)
	{
		if ((m->y() < height()) && (m->y() > 0))
		{
			switch (RulerCode)
			{
				case 1:
					currView->SelItem.at(0)->Extra = Extra;
					emit DocChanged(false);
					break;
				case 2:
					currView->SelItem.at(0)->RExtra = RExtra;
					emit DocChanged(false);
					break;
				case 3:
					currDoc->docParagraphStyles[currDoc->currentParaStyle].First = First;
					emit DocChanged(false);
					break;
				case 4:
					currDoc->docParagraphStyles[currDoc->currentParaStyle].Indent = Indent;
					currDoc->docParagraphStyles[currDoc->currentParaStyle].First = First;
					emit DocChanged(false);
					break;
				case 5:
					if (m->button() == RightButton)
					{
						TabValues[ActTab].tabType += 1;
						if (TabValues[ActTab].tabType > 4)
							TabValues[ActTab].tabType = 0;
					}
					if (currDoc->currentParaStyle > 4)
						currDoc->docParagraphStyles[currDoc->currentParaStyle].TabValues = TabValues;
					else
						currView->SelItem.at(0)->TabValues = TabValues;
					emit DocChanged(false);
					break;
				default:
					break;
			}
		}
		else
		{
			if (RulerCode == 5)
			{
				QValueList<PageItem::TabRecord>::Iterator it;
				it = TabValues.at(ActTab);
				TabValues.remove(it);
				ActTab = 0;
				if (currDoc->currentParaStyle > 4)
					currDoc->docParagraphStyles[currDoc->currentParaStyle].TabValues = TabValues;
				else
					currView->SelItem.at(0)->TabValues = TabValues;
				emit DocChanged(false);
				qApp->setOverrideCursor(QCursor(ArrowCursor), true);
			}
		}
		RulerCode = 0;
		currView->DrawNew();
		currView->EmitValues(currView->SelItem.at(0));
	}
	else
	{
		if ((Mpressed) && (m->pos().y() > height()))
		{
			currView->DrHY = -1;
			currView->SetYGuide(m, -1);
		}
		qApp->setOverrideCursor(QCursor(ArrowCursor), true);
		emit DocChanged(false);
	}
	Mpressed = false;
}

void Hruler::mouseMoveEvent(QMouseEvent *m)
{
	if (currDoc->loading)
		return;
	if (ItemPosValid)
	{
		double ColWidth = (ItemEndPos - ItemPos - (ColGap * (Cols - 1)) - Extra - RExtra - 2*lineCorr) / Cols;
		int ColEnd, ColStart;
		double oldInd;
		if (RulerCode > 2)
		{
			ColStart  = static_cast<int>((ItemPos+(ColWidth+ColGap)*(ActCol-1)+Extra+lineCorr+Offset)*Scaling-offs);
			ColEnd = static_cast<int>((ItemPos+(ColWidth+ColGap)*(ActCol-1)+ColWidth+Extra+lineCorr+Offset)*Scaling-offs);
		}
		else
		{
			ColStart =static_cast<int>((ItemPos+lineCorr+Offset)*Scaling-offs);
			ColEnd = static_cast<int>((ItemEndPos-lineCorr+Offset)*Scaling-offs);
		}
		if ((Mpressed) && (m->y() < height()) && (m->y() > 0) && (m->x() > ColStart) && (m->x() < ColEnd))
		{
			qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
			double toplimit = ItemEndPos-ItemPos-2*lineCorr-Extra - (ColGap * (Cols - 1))-1;
			double toplimit2 = ItemEndPos-ItemPos-2*lineCorr-RExtra - (ColGap * (Cols - 1))-1;
			switch (RulerCode)
			{
				case 1:
					Extra -= (MouseX - m->x()) / Scaling;
					if (Extra < 0)
						Extra = 0;
					if (Extra > toplimit2)
						Extra = toplimit2;
					emit MarkerMoved(Extra, 0);
					repaint();
					break;
				case 2:
					RExtra += (MouseX - m->x()) / Scaling;
					if (RExtra < 0)
						RExtra = 0;
					if (RExtra > toplimit)
						RExtra = toplimit;
					emit MarkerMoved(RExtra, 0);
					repaint();
					break;
				case 3:
					First -= (MouseX - m->x()) / Scaling;
					if ((ItemPos+(ColWidth+ColGap)*ActCol+First+Indent+Offset)*Scaling-offs < (ItemPos+(ColWidth+ColGap)*ActCol+Offset)*Scaling-offs)
						First += (MouseX - m->x()) / Scaling;
					if (First+Indent > ColWidth)
						First  = ColWidth-Indent;
					emit MarkerMoved(First, 0);
					repaint();
					break;
				case 4:
					oldInd = Indent+First;
					Indent -= (MouseX - m->x()) / Scaling;
					if (Indent < 0)
						Indent = 0;
					if (Indent > ColWidth-1)
						Indent  = ColWidth-1;
					First = oldInd - Indent;
					emit MarkerMoved(Indent, 0);
					repaint();
					break;
				case 5:
					TabValues[ActTab].tabPosition -= (MouseX - m->x()) / Scaling;
					if (TabValues[ActTab].tabPosition < 0)
						TabValues[ActTab].tabPosition = 0;
					if (TabValues[ActTab].tabPosition > ColWidth-1)
						TabValues[ActTab].tabPosition  = ColWidth-1;
					emit MarkerMoved(TabValues[ActTab].tabPosition, 0);
					UpdateTabList();
					repaint();
					break;
				default:
					break;
			}
			MouseX = m->x();
			if (RulerCode != 0)
			{
				QPoint py = currView->viewport()->mapFromGlobal(m->globalPos());
				QPainter p;
				p.begin(currView->viewport());
				p.setRasterOp(XorROP);
				p.setPen(QPen(white, 1, DotLine, FlatCap, MiterJoin));
				QPoint out = currView->contentsToViewport(QPoint(0, qRound(currDoc->currentPage->Yoffset*Scaling)));
				p.drawLine(Markp, out.y(), Markp, out.y()+qRound(currDoc->currentPage->Height * Scaling));
				p.drawLine(py.x(), out.y(), py.x(), out.y()+qRound(currDoc->currentPage->Height * Scaling));
				p.end();
				Markp = py.x();
			}
			return;
		}
		if ((!Mpressed) && (m->y() < height()) && (m->y() > 0) && (m->x() > ColStart) && (m->x() < ColEnd))
		{
			qApp->setOverrideCursor(QCursor(loadIcon("tab.png"), 3), true);
			double Pos = (ItemPos+Extra+lineCorr+Offset)*Scaling-offs;
			if ((static_cast<int>(Pos) < (m->x()+currDoc->guidesSettings.grabRad)) && (static_cast<int>(Pos) > (m->x()-currDoc->guidesSettings.grabRad)))
				qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
			Pos = (ItemEndPos-RExtra-lineCorr+Offset)*Scaling-offs;
			if ((static_cast<int>(Pos) < (m->x()+currDoc->guidesSettings.grabRad)) && (static_cast<int>(Pos) > (m->x()-currDoc->guidesSettings.grabRad)))
				qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
			QRect fpo;
			double ColWidth = (ItemEndPos - ItemPos - (ColGap * (Cols - 1)) - Extra - RExtra - 2*lineCorr) / Cols;
			if (currDoc->currentParaStyle > 4)
			{
				for (int CurrCol = 0; CurrCol < Cols; ++CurrCol)
				{
					Pos = (ItemPos+First+Indent+(ColWidth+ColGap)*CurrCol+Offset+Extra+lineCorr)*Scaling-offs;
					fpo = QRect(static_cast<int>(Pos)-3, 11, 6, 6);
					if (fpo.contains(m->pos()))
					{
						qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
						break;
					}
					Pos = (ItemPos+Indent+(ColWidth+ColGap)*CurrCol+Offset+Extra+lineCorr)*Scaling-offs;
					fpo = QRect(static_cast<int>(Pos)-3, 17, 6, 6);
					if (fpo.contains(m->pos()))
					{
						qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
						break;
					}
				}
			}
			if (TabValues.count() != 0)
			{
				for (int CurrCol = 0; CurrCol < Cols; ++CurrCol)
				{
					for (int yg = 0; yg < static_cast<int>(TabValues.count()); yg++)
					{
						Pos = (ItemPos+TabValues[yg].tabPosition+(ColWidth+ColGap)*CurrCol+Offset+Extra+lineCorr)*Scaling-offs;
						fpo = QRect(static_cast<int>(Pos)-3, 15, 8, 8);
						if (fpo.contains(m->pos()))
						{
							qApp->setOverrideCursor(QCursor(SizeHorCursor), true);
							break;
						}
					}
				}
			}
			emit MarkerMoved((m->x() - static_cast<int>((ItemPos+Extra+lineCorr+Offset)*Scaling-offs)) / Scaling, 0);
			return;
		}
		if ((Mpressed) && (RulerCode == 5) && ((m->y() > height()) || (m->y() < 0)))
		{
			qApp->setOverrideCursor(QCursor(loadIcon("DelPoint.png"), 4, 3), true);
			return;
		}
		qApp->setOverrideCursor(QCursor(ArrowCursor), true);
	}
	else
	{
		if ((Mpressed) && (m->pos().y() > height()))
			currView->FromHRuler(m);
	}
}

void Hruler::paintEvent(QPaintEvent *)
{
	if (currDoc->loading)
		return;
	int pc, xx;
	double of, xl;
	double sc = currView->getScale();
	QFont ff = font();
	ff.setPointSize(8);
	setFont(ff);
	QPainter p;
	p.begin(this);
	p.drawLine(0, 24, width(), 24);
	p.translate(-offs, 0);
	if (repX)
	{
		p.setPen(red);
		p.setBrush(red);
		QPointArray cr;
		cr.setPoints(3, Markp, 9, Markp+2, 0, Markp-2, 0);
		p.drawPolygon(cr);
		p.end();
		repX = false;
		return;
	}
	p.setBrush(black);
	p.setPen(black);
	p.setFont(font());
	Offset = 0;
	Scaling = sc;
	currDoc->PageFP ? pc = 2 : pc = 1;
	if (currDoc->MasterP)
	{
		pc = 1;
		Offset = 0;
	}
	for (xx = 0; xx < pc; ++xx)
	{
		p.setPen(QPen(black, 1, SolidLine, FlatCap, MiterJoin));
		of = xx * currDoc->pageWidth;
		for (xl = 0; xl < currDoc->pageWidth; xl += iter)
		{
			int markerX=qRound((xl+of)*sc)+1;
			p.drawLine(markerX, 18, markerX, 24);
		}
		for (xl = 0; xl < currDoc->pageWidth+(iter2/2); xl += iter2)
		{
			int markerX=qRound((xl+of)*sc)+1;
			p.drawLine(markerX, 11, markerX, 24);
			int textX=qRound((xl+of+2/sc) * sc)+1;
			switch (currDoc->docUnitIndex)
			{
				case 2:
				{
					QString tx = "";
					int num1 = static_cast<int>(xl / iter2 / cor);
					if (num1 != 0)
						tx = QString::number(num1);
					double frac = (xl / iter2 / cor) - num1;
					if ((frac > 0.24) && (frac < 0.26))
						tx += QChar(0xBC);
					if ((frac > 0.49) && (frac < 0.51))
						tx += QChar(0xBD);
					if ((frac > 0.74) && (frac < 0.76))
						tx += QChar(0xBE);
					p.drawText(textX, 17, tx);
					break;
				}
				case 3:
				case 5:
					p.drawText(textX, 17, QString::number(xl / iter / cor));
					break;
				case 4:
					p.drawText(textX, 17, QString::number(xl / iter / 10 / cor));
					break;
				default:
					p.drawText(textX, 17, QString::number(xl / iter * 10 / cor));
					break;
			}
		}
		if (((xx == 0) && (currDoc->currentPage->PageNr % 2 == 0) && (currDoc->FirstPageLeft))
		    || ((xx == 1) && (currDoc->currentPage->PageNr % 2 != 0) && (currDoc->FirstPageLeft))
		    || ((xx == 0) && (currDoc->currentPage->PageNr % 2 != 0) && (!currDoc->FirstPageLeft))
		    || ((xx == 1) && (currDoc->currentPage->PageNr % 2 == 0) && (!currDoc->FirstPageLeft))
			|| (pc == 1))
		{
			if (ItemPosValid)
			{
				p.eraseRect(QRect(QPoint(qRound((ItemPos+Extra)*sc), 10), QPoint(qRound((ItemEndPos-RExtra)*sc), 23)));
				p.drawLine(qRound((ItemPos+Extra)*sc), 24, qRound((ItemEndPos-RExtra)*sc), 24);
				p.save();
				if (Revers)
				{
					p.translate(qRound((ItemPos)*sc), 0);
					p.scale(-1, 1);
					p.translate(qRound((ItemPos+Extra)*sc-(ItemEndPos-RExtra)*sc), 0);
					p.translate(-qRound(ItemPos*sc), 0);
				}
				double Pos = ItemPos;
				double EndPos = ItemEndPos;
				for (int CurrCol = 0; CurrCol < Cols; ++CurrCol)
				{
					double ColWidth = (ItemEndPos - ItemPos - (ColGap * (Cols - 1)) - Extra - RExtra - 2*lineCorr) / Cols;
					Pos = ItemPos + (ColWidth + ColGap) * CurrCol+Extra + lineCorr;
					EndPos = Pos+ColWidth;
					p.setPen(QPen(blue, 1, SolidLine, FlatCap, MiterJoin));
					for (xl = Pos; xl < EndPos; xl += iter)
						p.drawLine(qRound(xl*sc), 18, qRound(xl*sc), 24);
					for (xl = Pos; xl < EndPos; xl += iter2)
					{
						p.drawLine(qRound(xl*sc), 11, qRound(xl*sc), 24);
						switch (currDoc->docUnitIndex)
						{
							case 2:
							{
								QString tx = "";
								int num1 = static_cast<int>((xl-Pos) / iter2 / cor);
								if (num1 != 0)
									tx = QString::number(num1);
								double frac = (xl / iter2 / cor) - num1;
								if ((frac > 0.24) && (frac < 0.26))
									tx += QChar(0xBC);
								if ((frac > 0.49) && (frac < 0.51))
									tx += QChar(0xBD);
								if ((frac > 0.74) && (frac < 0.76))
									tx += QChar(0xBE);
								if (Revers)
								{
									p.save();
									p.translate(qRound((xl-2/sc) * sc),0);
									p.scale(-1,1);
									p.drawText(0, 17, tx);
									p.restore();
								}
								else
									p.drawText(qRound((xl+2/sc) * sc), 17, tx);
								break;
							}
							case 3:
								if (Revers)
								{
									p.save();
									p.translate(qRound((xl-2/sc) * sc),0);
									p.scale(-1,1);
									p.drawText(0, 17, QString::number((xl-Pos) / iter / cor));
									p.restore();
								}
								else
									p.drawText(qRound((xl+2/sc) * sc), 17, QString::number((xl-Pos) / iter / cor));
								break;
							default:
								if (Revers)
								{
									p.save();
									p.translate(qRound((xl-2/sc) * sc),0);
									p.scale(-1,1);
									p.drawText(0, 17, QString::number((xl-Pos) / iter * 10 / cor));
									p.restore();
								}
								else
									p.drawText(qRound((xl+2/sc) * sc), 17, QString::number((xl-Pos) / iter * 10 / cor));
								break;
						}
					}
					p.setPen(QPen(blue, 2, SolidLine, FlatCap, MiterJoin));
					p.drawLine(qRound(Pos*sc), 11, qRound(Pos*sc), 23);
					if (CurrCol == 0)
					{
						p.drawLine(qRound(Pos*sc), 23, qRound((Pos+4/sc)*sc), 23);
						p.drawLine(qRound(Pos*sc), 11, qRound((Pos+4/sc)*sc), 11);
					}
					if (currDoc->currentParaStyle > 4)
					{
						p.setPen(QPen(blue, 1, SolidLine, FlatCap, MiterJoin));
						double fpos = Pos+First+Indent;
						QPointArray cr;
						cr.setPoints(3, qRound(fpos*sc), 17, qRound((fpos+3/sc)*sc), 11, qRound((fpos-3/sc)*sc), 11);
						p.drawPolygon(cr);
						QPointArray cr2;
						cr2.setPoints(3, qRound((Pos+Indent)*sc), 17, qRound((Pos+Indent+3/sc)*sc), 23, qRound((Pos+Indent-3/sc)*sc), 23);
						p.drawPolygon(cr2);
					}
					p.setPen(QPen(blue, 2, SolidLine, FlatCap, MiterJoin));
					if (TabValues.count() != 0)
					{
						p.setPen(QPen(black, 2, SolidLine, FlatCap, MiterJoin));
						for (int yg = 0; yg < static_cast<int>(TabValues.count()); yg++)
						{
							if (Pos+TabValues[yg].tabPosition < EndPos)
							{
								switch (static_cast<int>(TabValues[yg].tabType))
								{
									case 0:
										if (Revers)
										{
											p.save();
											p.translate(qRound((Pos+TabValues[yg].tabPosition)*sc),0);
											p.scale(-1,1);
											p.drawLine(0, 15, 0, 23);
											p.drawLine(0, 23, 8, 23);
											p.restore();
										}
										else
										{
											p.drawLine(qRound((Pos+TabValues[yg].tabPosition)*sc), 15, qRound((Pos+TabValues[yg].tabPosition)*sc), 23);
											p.drawLine(qRound((Pos+TabValues[yg].tabPosition)*sc), 23, qRound((Pos+TabValues[yg].tabPosition+8/sc)*sc), 23);
										}
										break;
									case 1:
										if (Revers)
										{
											p.save();
											p.translate(qRound((Pos+TabValues[yg].tabPosition)*sc),0);
											p.scale(-1,1);
											p.drawLine(0, 15, 0, 23);
											p.drawLine(0, 23, -8, 23);
											p.restore();
										}
										else
										{
											p.drawLine(qRound((Pos+TabValues[yg].tabPosition)*sc), 15, qRound((Pos+TabValues[yg].tabPosition)*sc), 23);
											p.drawLine(qRound((Pos+TabValues[yg].tabPosition)*sc), 23, qRound((Pos+TabValues[yg].tabPosition-8/sc)*sc), 23);
										}
										break;
									case 2:
									case 3:
										p.drawLine(qRound((Pos+TabValues[yg].tabPosition)*sc), 15, qRound((Pos+TabValues[yg].tabPosition)*sc), 23);
										p.drawLine(qRound((Pos+TabValues[yg].tabPosition-4/sc)*sc), 23, qRound((Pos+TabValues[yg].tabPosition+4/sc)*sc), 23);
										p.drawLine(qRound((Pos+TabValues[yg].tabPosition+3/sc)*sc), 20, qRound((Pos+TabValues[yg].tabPosition+2/sc)*sc), 20);
										break;
									case 4:
										p.drawLine(qRound((Pos+TabValues[yg].tabPosition)*sc), 15, qRound((Pos+TabValues[yg].tabPosition)*sc), 23);
										p.drawLine(qRound((Pos+TabValues[yg].tabPosition-4/sc)*sc), 23, qRound((Pos+TabValues[yg].tabPosition+4/sc)*sc), 23);
										break;
									default:
										break;
								}
							}
						}
					}
					p.setPen(QPen(blue, 2, SolidLine, FlatCap, MiterJoin));
					p.drawLine(qRound(EndPos*sc), 11, qRound(EndPos*sc), 23);
					if (CurrCol == Cols-1)
					{
						p.drawLine(qRound(EndPos*sc), 23, qRound((EndPos-4/sc)*sc), 23);
						p.drawLine(qRound(EndPos*sc), 11, qRound((EndPos-4/sc)*sc), 11);
					}
				}
			p.restore();
			}
		}
	}
	p.end();
}

/** Zeichnet den Pfeil */
void Hruler::Draw(int wo)
{
	repX = true;
	Markp = wo-qRound(currDoc->ScratchLeft*currView->getScale());
	repaint(QRect(0, 0, width(), 9));
}

void Hruler::UpdateTabList()
{
	struct PageItem::TabRecord tb;
	tb.tabPosition = TabValues[ActTab].tabPosition;
	tb.tabType = TabValues[ActTab].tabType;
	tb.tabFillChar =  TabValues[ActTab].tabFillChar;
	QValueList<PageItem::TabRecord>::Iterator it;
	int gg = static_cast<int>(TabValues.count());
	int g = gg;
	it = TabValues.at(ActTab);
	TabValues.remove(it);
	for (int yg = static_cast<int>(TabValues.count()-1); yg > -1; yg--)
	{
		if (tb.tabPosition < TabValues[yg].tabPosition)
			g = yg;
	}
	ActTab = g;
	if (gg == g)
	{
		TabValues.append(tb);
		ActTab = static_cast<int>(TabValues.count()-1);
	}
	else
	{
		it = TabValues.at(ActTab);
		TabValues.insert(it, tb);
	}
}

void Hruler::unitChange()
{
	double sc = currView->getScale();
	cor=1;
	switch (currDoc->docUnitIndex)
	{
		case 0:
			if (sc > 1 && sc <= 4)
				cor = 2;
			if (sc > 4)
				cor = 10;
			iter = unitRulerGetIter1FromIndex(currDoc->docUnitIndex) / cor;
	  		iter2 = unitRulerGetIter2FromIndex(currDoc->docUnitIndex) / cor;
			break;
		case 1:
			if (sc > 1)
				cor = 10;
			iter = unitRulerGetIter1FromIndex(currDoc->docUnitIndex) / cor;
  			iter2 = unitRulerGetIter2FromIndex(currDoc->docUnitIndex) / cor;
			break;
		case 2:
			iter = unitRulerGetIter1FromIndex(currDoc->docUnitIndex);
			iter2 = unitRulerGetIter2FromIndex(currDoc->docUnitIndex);
			if (sc > 1 && sc <= 4)
			{
				cor = 2;
				iter /= cor;
				iter2 /= cor;
			}
			if (sc > 4)
			{
				cor = 4;
				iter /= cor;
				iter2 /= cor;
			}
			break;
		case 3:
			iter = unitRulerGetIter1FromIndex(currDoc->docUnitIndex);
			iter2 = unitRulerGetIter2FromIndex(currDoc->docUnitIndex);
			if (sc > 1 && sc <= 4)
			{
				cor = 1;
				iter = 12.0;
				iter2 = 60.0;
			}
			if (sc > 4)
			{
				cor = 2;
				iter = 6.0;
				iter2 = 12.0;
			}
			break;
		case 4:
			if (sc > 1 && sc <= 4)
				cor = 1;
			if (sc > 4)
				cor = 10;
			iter = unitRulerGetIter1FromIndex(currDoc->docUnitIndex) / cor;
			iter2 = unitRulerGetIter2FromIndex(currDoc->docUnitIndex) / cor;
			break;
		case 5:
			iter = unitRulerGetIter1FromIndex(currDoc->docUnitIndex);
			iter2 = unitRulerGetIter2FromIndex(currDoc->docUnitIndex);
			if (sc > 1 && sc <= 4)
			{
				cor = 1;
				iter = 72.0/25.4*4.512;
				iter2 = 72.0/25.4*4.512*5.0;
			}
			if (sc > 4)
			{
				cor = 2;
				iter = 72.0/25.4*4.512/2.0;
				iter2 = 72.0/25.4*4.512;
			}
			break;			
		default:
			if (sc > 1 && sc <= 4)
				cor = 2;
			if (sc > 4)
				cor = 10;
			iter = unitRulerGetIter1FromIndex(0) / cor;
	 		iter2 = unitRulerGetIter2FromIndex(0) / cor;
			break;
	}
}

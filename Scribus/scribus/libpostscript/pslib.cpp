/***************************************************************************
                          pslib.cpp  -  description
                             -------------------
    begin                : Sat May 26 2001
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
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qimage.h>
#include <qcolor.h>
#include <qcstring.h>
#include <qfontinfo.h>
#include <cstdlib>
#include <qregexp.h>

#if (_MSC_VER >= 1200)
 #include "win-config.h"
#else
 #include "config.h"
#endif

#include "pslib.h"
#include "scfonts.h"
#include <cmath>

extern bool loadText(QString nam, QString *Buffer);
extern QImage LoadPict(QString fn, bool *gray = 0);
extern QString CompressStr(QString *in);
extern QString ImageToCMYK_PS(QImage *im, int pl, bool pre);
extern QString MaskToTxt(QImage *im, bool PDF = true);
extern char *toHex( uchar u );
extern QString String2Hex(QString *in, bool lang = true);
#ifdef HAVE_CMS
extern bool CMSuse;
extern QImage LoadPictCol(QString fn, QString Prof, bool UseEmbedded, bool *realCMYK);
#endif

extern "C" void* Run(bool psart, SCFonts &AllFonts, QMap<QString,QFont> DocFonts, CListe DocColors, bool pdf);

void* Run(bool psart, SCFonts &AllFonts, QMap<QString,QFont> DocFonts, CListe DocColors, bool pdf)
{
	PSLib *dia = new PSLib(psart, AllFonts, DocFonts, DocColors, pdf);
	return dia;
}

PSLib::PSLib(bool psart, SCFonts &AllFonts, QMap<QString,QFont> DocFonts, CListe DocColors, bool pdf)
{
	QString tmp, tmp2, tmp3, tmp4, CHset;
	QStringList wt;
  	QString Epfad = PREL;
	Seiten = 0;
	User = "";
	Creator = "Scribus 1.2cvs";
	Titel = "";
	FillColor = "0.0 0.0 0.0 0.0";
	StrokeColor = "0.0 0.0 0.0 0.0";
	Header = psart ? "%!PS-Adobe-3.0\n" : "%!PS-Adobe-3.0 EPSF-3.0\n";
	BBox = "";
	BBoxH = "";
	Art = psart;
	isPDF = pdf;
	UsedFonts.clear();
	Fonts = "";
	FontDesc = "";
	GraySc = false;
	DoSep = false;
#ifdef HAVE_LIBZ
	CompAvail = true;
#else
	CompAvail = false;
#endif
	GrayCalc =  "/setcmykcolor {exch 0.11 mul add exch 0.59 mul add exch 0.3 mul add\n";
	GrayCalc += "               dup 1 gt {pop 1} if 1 exch sub oldsetgray} bind def\n";
	GrayCalc += "/setrgbcolor {0.11 mul exch 0.59 mul add exch 0.3 mul add\n";
	GrayCalc += "              oldsetgray} bind def\n";
	Farben = "%%CMYKCustomColor: ";
	FNamen = "%%DocumentCustomColors: ";
	CListe::Iterator itf;
	int c, m, y, k;
	bool erst = true;
	for (itf = DocColors.begin(); itf != DocColors.end(); ++itf)
	{
		if ((itf.key() != "Cyan") && (itf.key() != "Magenta") && (itf.key() != "Yellow") && 
			(itf.key() != "Black"))
		{
			DocColors[itf.key()].getCMYK(&c, &m, &y, &k);
			if (!erst)
			{
				Farben += "%%+ ";
				FNamen += "%%+ ";
			}
			Farben += ToStr(static_cast<double>(c) / 255) + " " +  
					ToStr(static_cast<double>(m) / 255) + " ";
			Farben += ToStr(static_cast<double>(y) / 255) + " " +  
					ToStr(static_cast<double>(k) / 255) + " (" + itf.key() + ")\n";
			FNamen += "(" + itf.key() + ")\n";
			erst = false;
		}
	}
	QMap<QString,QFont>::Iterator it;
	int a = 0;
	for (it = DocFonts.begin(); it != DocFonts.end(); ++it)
	{
/* Subset all TTF Fonts until the bug in the TTF-Embedding Code is fixed */
		QFileInfo fd = QFileInfo(AllFonts[it.key()]->Datei);
		QString fext = fd.extension(false).lower();

		if ((fext == "ttf") || (AllFonts[it.key()]->isOTF) || (AllFonts[it.key()]->Subset))
		{
			FontDesc += "/"+AllFonts[it.key()]->RealName()+
					" "+IToStr(AllFonts[it.key()]->RealGlyphs.count()+1)+" dict def\n";
			FontDesc += AllFonts[it.key()]->RealName()+" begin\n";
			QMap<uint,FPointArray>::Iterator ig;
			for (ig = AllFonts[it.key()]->RealGlyphs.begin(); 
				ig != AllFonts[it.key()]->RealGlyphs.end(); ++ig)
			{
				FontDesc += "/G"+IToStr(ig.key())+" { newpath\n";
				FPoint np, np1, np2;
				bool nPath = true;
				if (ig.data().size() > 3)
				{
					for (uint poi = 0; poi < ig.data().size()-3; poi += 4)
					{
						if (ig.data().point(poi).x() > 900000)
						{
							FontDesc += "cl\n";
							nPath = true;
							continue;
						}
						if (nPath)
						{
							np = ig.data().point(poi);
							FontDesc += ToStr(np.x()) + " " + ToStr(-np.y()) + " m\n";
							nPath = false;
						}
						np = ig.data().point(poi+1);
						np1 = ig.data().point(poi+3);
						np2 = ig.data().point(poi+2);
						FontDesc += ToStr(np.x()) + " " + ToStr(-np.y()) + " " + 
								ToStr(np1.x()) + " " + ToStr(-np1.y()) + " " + 
								ToStr(np2.x()) + " " + ToStr(-np2.y()) + " cu\n";
					}
				}
				FontDesc += "cl\n} bind def\n";
			}
			FontDesc += "end\n";
			AllFonts[it.key()]->RealGlyphs.clear();
		}
		else
		{
			UsedFonts.insert(it.key(), "/Fo"+IToStr(a));
			Fonts += "/Fo"+IToStr(a)+" /"+AllFonts[it.key()]->RealName()+" findfont definefont pop\n";
			if (AllFonts[it.key()]->EmbedPS)
			{
				QString tmp;
				if(AllFonts[it.key()]->EmbedFont(tmp))
				{
					FontDesc += "%%BeginFont: " + AllFonts[it.key()]->RealName() + "\n";
					FontDesc += tmp + "\n%%EndFont\n";
				}
			}
			GListe gl;
			AllFonts[it.key()]->GlNames(&gl);
			GlyphsOfFont.insert(it.key(), gl);
			a++;
		}
	}
	Prolog = "%%BeginProlog\n";
	Prolog += "/Scribusdict 100 dict def\n";
	Prolog += "Scribusdict begin\n";
	Prolog += "/sp {showpage} bind def\n";
	Prolog += "/oldsetgray /setgray load def\n";
	Prolog += "/cmyk {setcmykcolor} def\n";
	Prolog += "/m {moveto} bind def\n";
	Prolog += "/l {lineto} bind def\n";
	Prolog += "/li {lineto} bind def\n";
	Prolog += "/cu {curveto} bind def\n";
	Prolog += "/cl {closepath} bind def\n";
	Prolog += "/gs {gsave} bind def\n";
	Prolog += "/gr {grestore} bind def\n";
	Prolog += "/tr {translate} bind def\n";
	Prolog += "/ro {rotate} bind def\n";
	Prolog += "/sh {show} bind def\n";
	Prolog += "/shg {setcmykcolor moveto glyphshow} def\n";
	Prolog += "/sc {scale} bind def\n";
	Prolog += "/se {selectfont} bind def\n";
	Prolog += "/sf {setfont} bind def\n";
	Prolog += "/sw {setlinewidth} bind def\n";
	Prolog += "/f  {findfont} bind def\n";
	Prolog += "/fi {fill} bind def\n";
	Prolog += "/st {stroke} bind def\n";
	Prolog += "/shgf {gs dup scale begin cvx exec fill end gr} bind def\n";
	Prolog += "/shgs {gs dup 1 exch div currentlinewidth mul sw dup scale\n";
	Prolog += "       begin cvx exec st end gr} bind def\n";
	Prolog += "/bEPS {\n";
	Prolog += "    /b4_Inc_state save def\n";
	Prolog += "    /dict_count countdictstack def\n";
	Prolog += "    /op_count count 1 sub def\n";
	Prolog += "    userdict begin\n";
	Prolog += "    /showpage { } def\n";
	Prolog += "    0 setgray 0 setlinecap\n";
	Prolog += "    1 setlinewidth 0 setlinejoin\n";
	Prolog += "    10 setmiterlimit [ ] 0 setdash newpath\n";
	Prolog += "    /languagelevel where\n";
	Prolog += "    {pop languagelevel\n";
	Prolog += "    1 ne\n";
	Prolog += "    {false setstrokeadjust false setoverprint\n";
	Prolog += "    } if } if } bind def\n";
	Prolog += "/eEPS { count op_count sub {pop} repeat\n";
	Prolog += "    countdictstack dict_count sub {end} repeat\n";
	Prolog += "    b4_Inc_state restore } bind def\n";
	Prolog += "    end\n";
	Prolog += "%%EndProlog\n";
}

void PSLib::PutSeite(QString c)
{
	QTextStream t(&Spool);
	t.writeRawBytes(c, c.length());
}

void PSLib::PutDoc(QString c)
{
	QTextStream t(&Spool);
	t.writeRawBytes(c, c.length());
}

QString PSLib::ToStr(double c)
{
	QString cc;
	return cc.setNum(c);
}

QString PSLib::IToStr(int c)
{
	QString cc;
	return cc.setNum(c);
}

void PSLib::PS_set_Info(QString art, QString was)
{
	if (art == "Author")
		User = was;
	if (art == "Creator")
		Creator = was;
	if (art == "Title")
		Titel = was;
}

bool PSLib::PS_set_file(QString fn)
{
	Spool.setName(fn);
	return Spool.open(IO_WriteOnly);
}

void PSLib::PS_begin_doc(int Ori, double breite, double hoehe, int numpage)
{
	PutDoc(Header);
	PutDoc("%%For: " + User + "\n");
	PutDoc("%%Title: " + Titel + "\n");
	PutDoc("%%Creator: " + Creator + "\n");
	PutDoc("%%Pages: " + IToStr(numpage) + "\n");
	BBox = "%%BoundingBox: 0 0 " + IToStr(qRound(breite)) + " " + IToStr(qRound(hoehe)) + "\n";
	BBoxH = "%%HiResBoundingBox: 0 0 " + ToStr(breite) + " " + ToStr(hoehe) + "\n";
  	if (!Art)
	{
		PutDoc(BBox);
		PutDoc(BBoxH);
	}
	PutDoc(FNamen);
	PutDoc(Farben);
	PutDoc("%%LanguageLevel: 3\n");
	PutDoc("%%EndComments\n");
	PutDoc(Prolog);
	PutDoc("%%BeginSetup\n");
	PutDoc("/pdfmark where {pop} {userdict /pdfmark /cleartomark load put} ifelse\n");
	if (FontDesc != "")
		PutDoc(FontDesc);
	PutDoc("Scribusdict begin\n");
	PutDoc(Fonts);
	if (GraySc)
		PutDoc(GrayCalc);
	PutDoc("%%EndSetup\n");
/*  if ((Art) && (Ori != 0))
  	{
		PutSeite("<< /PageSize [ "+ToStr(breite)+" "+ToStr(hoehe)+" ]\n");
		PutSeite("/Orientation 3\n");
		PutSeite(">> setpagedevice\n");
		}                    */
	Prolog = "";
	FontDesc = "";
}

QString PSLib::PSEncode(QString in)
{
	QString tmp = "";
	QString cc;
	for (uint d = 0; d < in.length(); ++d)
	{
		cc = in.at(d);
		if ((cc == "(") || (cc == ")") || (cc == "\\"))
			tmp += "\\";
		tmp += cc;
	}
	tmp = tmp.simplifyWhiteSpace().replace( QRegExp("\\s"), "" );
	return tmp;
}

void PSLib::PS_TemplateStart(QString Name, double breite, double hoehe)
{
	PutDoc("/"+PSEncode(Name)+"\n<<\n");
	PutDoc("/FormType 1\n");
	PutDoc("/BBox [0 0 "+ToStr(breite)+" "+ToStr(hoehe)+"]\n");
	PutDoc("/Matrix [1 0 0 1 0 0]\n");
	PutDoc("/PaintProc\n{\n");
}

void PSLib::PS_UseTemplate(QString Name)
{
	PutDoc(PSEncode(Name)+" execform\n");
}

void PSLib::PS_TemplateEnd()
{
	PutDoc("pop } bind\n");
	PutDoc(">> def\n");
}

void PSLib::PS_begin_page(double breite, double hoehe, struct Margs* Ma, bool Clipping)
{
	if (Clipping)
	{
		PDev = ToStr(Ma->Left) + " " + ToStr(Ma->Bottom) + " m\n";
		PDev += ToStr(breite - Ma->Right) + " " + ToStr(Ma->Bottom) + " li\n";
		PDev += ToStr(breite - Ma->Right) + " " + ToStr(hoehe - Ma->Top) + " li\n";
		PDev += ToStr(Ma->Left) + " " + ToStr(hoehe - Ma->Top) + " li cl clip newpath\n";
	}
	Seiten++;
	PutSeite("%%Page: " + IToStr(Seiten) + " " + IToStr(Seiten) + "\nsave\n");
	if (Clipping)
		PutSeite(PDev);
  	PutSeite("/DeviceCMYK setcolorspace\n");
}

void PSLib::PS_end_page()
{
	PutSeite("%%PageTrailer\nrestore\nsp\n");
}

void PSLib::PS_curve(double x1, double y1, double x2, double y2, double x3, double y3)
{
	PutSeite(ToStr(x1) + " " + ToStr(y1) + " " + ToStr(x2) + " " + ToStr(y2) + " " + ToStr(x3) + " " + 
			ToStr(y3) + " curveto\n");
}

void PSLib::PS_moveto(double x, double y)
{
	PutSeite(ToStr(x) + " " + ToStr(y) + " m\n");
}

void PSLib::PS_lineto(double x, double y)
{
	PutSeite(ToStr(x) + " " + ToStr(y) + " li\n");
}

void PSLib::PS_closepath()
{
	PutSeite("cl\n");
}

void PSLib::PS_translate(double x, double y)
{
	PutSeite(ToStr(x) + " " + ToStr(y) + " tr\n");
}

void PSLib::PS_scale(double x, double y)
{
	PutSeite(ToStr(x) + " " + ToStr(y) + " sc\n");
}

void PSLib::PS_rotate(double x)
{
	PutSeite(ToStr(x) + " ro\n");
}

void PSLib::PS_clip(bool mu)
{
	PutSeite( mu ? "eoclip newpath\n" : "clip newpath\n" );
}

void PSLib::PS_save()
{
	PutSeite("gs\n");
}

void PSLib::PS_restore()
{
	PutSeite("gr\n");
}

void PSLib::PS_setcmykcolor_fill(double c, double m, double y, double k)
{
	FillColor = ToStr(c) + " " + ToStr(m) + " " + ToStr(y) + " " + ToStr(k);
}

void PSLib::PS_setcmykcolor_dummy()
{
	PutSeite("0 0 0 0 cmyk\n");
}

void PSLib::PS_setcmykcolor_stroke(double c, double m, double y, double k)
{
	StrokeColor = ToStr(c) + " " + ToStr(m) + " " + ToStr(y) + " " + ToStr(k);
}

void PSLib::PS_setlinewidth(double w)
{
	PutSeite(ToStr(w) + " sw\n");
	LineW = w;
}

void PSLib::PS_setdash(Qt::PenStyle st, Qt::PenCapStyle ca, Qt::PenJoinStyle jo)
{
	QString Dt = ToStr(QMAX(LineW, 1));
	QString Da = ToStr(QMAX(3*LineW, 1));
	switch (st)
	{
		case Qt::SolidLine:
			PutSeite("[] 0 setdash\n");
			break;
		case Qt::DashLine:
			PutSeite("["+Da+" "+Dt+"] 0 setdash\n");
			break;
		case Qt::DotLine:
			PutSeite("["+Dt+"] 0 setdash\n");
			break;
		case Qt::DashDotLine:
			PutSeite("["+Da+" "+Dt+" "+Dt+" "+Dt+"] 0 setdash\n");
			break;
		case Qt::DashDotDotLine:
			PutSeite("["+Da+" "+Dt+" "+Dt+" "+Dt+" "+Dt+" "+Dt+"] 0 setdash\n");
			break;
		default:
			PutSeite("[] 0 setdash\n");
			break;
	}
	switch (ca)
		{
		case Qt::FlatCap:
			PutSeite("0 setlinecap\n");
			break;
		case Qt::SquareCap:
			PutSeite("2 setlinecap\n");
			break;
		case Qt::RoundCap:
			PutSeite("1 setlinecap\n");
			break;
		default:
			PutSeite("0 setlinecap\n");
			break;
		}
	switch (jo)
		{
		case Qt::MiterJoin:
			PutSeite("0 setlinejoin\n");
			break;
		case Qt::BevelJoin:
			PutSeite("2 setlinejoin\n");
			break;
		case Qt::RoundJoin:
			PutSeite("1 setlinejoin\n");
			break;
		default:
			PutSeite("0 setlinejoin\n");
			break;
		}
}

void PSLib::PS_selectfont(QString f, double s)
{
	PutSeite(UsedFonts[f] + " " + ToStr(s) + " se\n");
}

void PSLib::PS_fill()
{
	PutSeite(FillColor + " cmyk eofill\n");
}

void PSLib::PS_stroke()
{
	PutSeite(StrokeColor + " cmyk st\n");
}

void PSLib::PS_fill_stroke()
{
	PS_save();
	PS_fill();
	PS_restore();
	PS_stroke();
}

void PSLib::PS_newpath()
{
	PutSeite("newpath\n");
}

void PSLib::PS_GradientCol1(double c, double m, double y, double k)
{
	GrColor1 = ToStr(c) + " " + ToStr(m) + " " + ToStr(y) + " " + ToStr(k);
}

void PSLib::PS_GradientCol2(double c, double m, double y, double k)
{
	GrColor2 = ToStr(c) + " " + ToStr(m) + " " + ToStr(y) + " " + ToStr(k);
}

void PSLib::PS_LinGradient(double w, double h, int item, int grad)
{
	if (item == 1)
		PutSeite("-"+ToStr(w / 2.0)+" "+ToStr(-h / 2.0)+" tr\n");
	PutSeite( "clipsave\n" );
	PutSeite("eoclip\n");
	PutSeite("<<\n");
	PutSeite("/ShadingType 2\n");
	PutSeite( DoSep ? "/ColorSpace /DeviceGray\n" : "/ColorSpace /DeviceCMYK\n" );
	switch (grad)
	{
		case 1:
			PutSeite("/Coords [0 "+ToStr(h / 2.0)+" "+ToStr(w)+" "+ToStr(h / 2.0)+"]\n");
			break;
		case 2:
			PutSeite("/Coords ["+ToStr(w / 2.0)+" 0 "+ToStr(w / 2.0)+" "+ToStr(h)+"]\n");
			break;
		case 3:
			PutSeite("/Coords [0 0 "+ToStr(w)+" "+ToStr(h)+"]\n");
			break;
		case 4:
			PutSeite("/Coords ["+ToStr(w)+" 0 0 "+ToStr(h)+"]\n");
			break;
	}
	PutSeite("/BBox [0 "+ToStr(h)+" "+ToStr(w)+" 0]\n");
	PutSeite(DoSep ? QString("/Background [0]\n") : QString("/Background ["+FillColor+"]\n"));
	PutSeite("/Extend [false false]\n");
	PutSeite("/Function\n");
	PutSeite("<<\n");
	PutSeite("/FunctionType 2\n");
	PutSeite("/Domain [0 1]\n");
	if (DoSep)
	{
		int pla = Plate - 1 < 0 ? 3 : Plate - 1;
		QStringList cols1 = QStringList::split(" ", GrColor2);
		QStringList cols2 = QStringList::split(" ", GrColor1);
		if (grad == 4)
		{
			PutSeite("/C0 ["+ToStr(1-cols1[pla].toDouble())+"]\n");
			PutSeite("/C1 ["+ToStr(1-cols2[pla].toDouble())+"]\n");
		}
		else
		{
			PutSeite("/C1 ["+ToStr(1-cols1[pla].toDouble())+"]\n");
			PutSeite("/C0 ["+ToStr(1-cols2[pla].toDouble())+"]\n");
		}
	}
	else
	{
		if (grad == 4)
		{
			PutSeite("/C1 ["+GrColor1+"]\n");
			PutSeite("/C0 ["+GrColor2+"]\n");
		}
		else
		{
			PutSeite("/C0 ["+GrColor1+"]\n");
			PutSeite("/C1 ["+GrColor2+"]\n");
		}
	}
	PutSeite("/N 1\n");
	PutSeite(">>\n");
	PutSeite(">>\n");
	PutSeite("shfill\ncliprestore\n");
}

void PSLib::PS_RadGradient(double w, double h, int item)
{
	double w2, h2, rad;
	w2 = w / 2.0;
	h2 = h / 2.0;
	rad = QMIN(w, fabs(h)) / 2.0;
	PutSeite("/cmtx matrix currentmatrix def\n");
	PutSeite( "clipsave\n" );
	PutSeite("eoclip\n");
	PutSeite("<<\n");
	PutSeite("/ShadingType 3\n");
	PutSeite( DoSep ? "/ColorSpace /DeviceGray\n" : "/ColorSpace /DeviceCMYK\n" );
	PutSeite("/Coords ["+ToStr(w2)+" "+ToStr(h2)+" 0 "+ToStr(w2)+" "+ToStr(h2)+" "+ToStr(rad)+"]\n");
	PutSeite("/BBox [0 "+ToStr(h)+" "+ToStr(w)+" 0]\n");
	PutSeite( DoSep ? QString("/Background [0]\n") : "/Background ["+FillColor+"]\n" );
	PutSeite("/Extend [true true]\n");
	PutSeite("/Function\n");
	PutSeite("<<\n");
	PutSeite("/FunctionType 2\n");
	PutSeite("/Domain [0 1]\n");
	if (DoSep)
	{
		int pla = Plate - 1 < 0 ? 3 : Plate - 1;
		QStringList cols1 = QStringList::split(" ", GrColor2);
		QStringList cols2 = QStringList::split(" ", GrColor1);
		PutSeite("/C0 ["+ToStr(1-cols1[pla].toDouble())+"]\n");
		PutSeite("/C1 ["+ToStr(1-cols2[pla].toDouble())+"]\n");
	}
	else
	{
		PutSeite("/C0 ["+GrColor2+"]\n");
		PutSeite("/C1 ["+GrColor1+"]\n");
	}
	PutSeite("/N 1\n");
	PutSeite(">>\n");
	PutSeite(">>\n");
	if (item == 1)
		PutSeite( w > fabs(h) ? "-"+ToStr(w2*(w2 / rad))+" "+ToStr(fabs(h2))+" tr\n" :
							"-"+ToStr(w2*(w2 / rad))+" "+
								ToStr(fabs(h2)*(fabs(h2) /rad))+" tr\n" );
	else
	{
		if (w > fabs(h))
			PutSeite("-"+ToStr(w2*(w2 / rad)-w2)+" 0 tr\n");
		else
		{
			if (w < fabs(h))
				PutSeite("0 "+ToStr(fabs(h2)*(fabs(h2) /rad)-fabs(h2))+" tr\n");
		}
	}
	PutSeite(ToStr(w2 / rad)+" "+ToStr(fabs(h2) / rad)+" scale\n");
	PutSeite("shfill\ncliprestore\n");
	PutSeite("cmtx setmatrix\n");
}

void PSLib::PS_show_xyG(QString font, QString ch, double x, double y)
{
	QString Name;
	uint cc = ch[0].unicode();
	Name = GlyphsOfFont[font].contains(cc) ? GlyphsOfFont[font][cc] : QString(".notdef");
	PutSeite("/"+Name+" "+ToStr(x)+" "+ToStr(y)+" "+StrokeColor+" shg\n");
}

void PSLib::PS_show(double x, double y)
{
	PS_moveto(x, y);
	PutSeite("/hyphen glyphshow\n");
}

void PSLib::PS_showSub(uint chr, QString font, double size, bool stroke)
{
	PutSeite(FillColor + " cmyk (G"+IToStr(chr)+") "+font+" "+ToStr(size / 10.0)+" ");
	PutSeite(stroke ? "shgs\n" : "shgf\n");
}

void PSLib::PS_ImageData(bool inver, QString fn, QString Name, QString Prof, bool UseEmbedded, bool UseProf)
{
	QString tmp;
	QFileInfo fi = QFileInfo(fn);
	QString ext = fi.extension(false).lower();
	if (ext == "eps")
	{
		if (loadText(fn, &tmp))
		{
			PutSeite("currentfile 1 (%ENDEPSDATA) /SubFileDecode filter /ReusableStreamDecode filter\n");
      			PutSeite("%%BeginDocument: " + fi.fileName() + "\n");
      			PutSeite(tmp+"\n");
      			PutSeite("%ENDEPSDATA\n");
      			PutSeite("%%EndDocument\n");
			PutSeite("/"+PSEncode(Name)+"Bild exch def\n");
		}
		return;
	}
  	QString ImgStr = "";
	QImage image;
#ifdef HAVE_CMS
	QImage image2;
	bool cmy = false;
	if ((CMSuse) && (UseProf))
	{
		image = LoadPict(fn);
		image = image.convertDepth(32);
		image2 = LoadPictCol(fn, Prof, UseEmbedded, &cmy);
		if (inver)
		{
			image.invertPixels();
			image2.invertPixels();
		}
		ImgStr = ImageToCMYK_PS(&image2, -1, cmy);
	}
	else
	{
		image = LoadPict(fn);
		if (inver)
			image.invertPixels();
		ImgStr = ImageToCMYK_PS(&image, -1, false);
	}
#else
	image = LoadPict(fn);
  image = image.convertDepth(32);
	if (inver)
		image.invertPixels();
	ImgStr = ImageToCMYK_PS(&image, -1, false);
#endif
	if (CompAvail)
	{
		PutSeite("currentfile /ASCIIHexDecode filter /FlateDecode filter /ReusableStreamDecode filter\n");
		ImgStr = CompressStr(&ImgStr);
	}
	else
		PutSeite("currentfile /ASCIIHexDecode filter /ReusableStreamDecode filter\n");
	ImgStr = String2Hex(&ImgStr);
	PutSeite(ImgStr);
	PutSeite("\n>\n");
	PutSeite("/"+PSEncode(Name)+"Bild exch def\n");
	ImgStr = "";
  if (image.hasAlphaBuffer())
   	{
		QImage iMask = image.createAlphaMask();
		ImgStr = MaskToTxt(&iMask, false);
		if (CompAvail)
		{
			PutSeite("currentfile /ASCIIHexDecode filter /FlateDecode filter /ReusableStreamDecode filter\n");
			ImgStr = CompressStr(&ImgStr);
		}
		else
			PutSeite("currentfile /ASCIIHexDecode filter /ReusableStreamDecode filter\n");
		ImgStr = String2Hex(&ImgStr);
		PutSeite(ImgStr);
		PutSeite("\n>\n");
		PutSeite("/"+PSEncode(Name)+"Mask exch def\n");
	}
}

void PSLib::PS_image(bool inver, double x, double y, QString fn, double scalex, double scaley, QString Prof, bool UseEmbedded, bool UseProf, QString Name)
{
	QString tmp;
	QFileInfo fi = QFileInfo(fn);
	QString ext = fi.extension(false).lower();
	if (ext == "eps")
	{
		if (loadText(fn, &tmp))
		{
			PutSeite("bEPS\n");
      			PutSeite(ToStr(scalex) + " " + ToStr(scaley) + " sc\n");
      			PutSeite(ToStr(x) + " " + ToStr(y) + " tr\n");
			if (Name != "")
			{
				PutSeite(PSEncode(Name)+"Bild cvx exec\n");
				PutSeite(PSEncode(Name)+"Bild resetfile\n");
			}
			else
			{
      				PutSeite("%%BeginDocument: " + fi.fileName() + "\n");
      				PutSeite(tmp+"\n");
      				PutSeite("%%EndDocument\n");
			}
			PutSeite("eEPS\n");
		}
	}
	else
	{
    		QString ImgStr = "";
		QImage image;
#ifdef HAVE_CMS
		QImage image2;
		bool cmy = false;
    		image = LoadPict(fn);
		image = image.convertDepth(32);
    		if ((CMSuse) && (UseProf))
			image2 = LoadPictCol(fn, Prof, UseEmbedded, &cmy);
		if (inver)
		{
			image.invertPixels();
			image2.invertPixels();
		}
#else
		image = LoadPict(fn);
  		image = image.convertDepth(32);
		if (inver)
			image.invertPixels();
#endif
		int w = image.width();
		int h = image.height();
    		PutSeite(ToStr(x*scalex) + " " + ToStr(y*scaley) + " tr\n");
    		PutSeite(ToStr(scalex*w) + " " + ToStr(scaley*h) + " sc\n");
    		PutSeite(((!DoSep) && (!GraySc)) ? "/DeviceCMYK setcolorspace\n" : "/DeviceGray setcolorspace\n");
    		if (image.hasAlphaBuffer())
    		{
			QImage iMask = image.createAlphaMask();
#ifdef HAVE_CMS
			if ((CMSuse) && (UseProf))
			{
				if (DoSep)
					ImgStr = ImageToCMYK_PS(&image2, Plate, cmy);
				else
					ImgStr = GraySc ? ImageToCMYK_PS(&image2, -2, cmy) : 
								ImageToCMYK_PS(&image2, -1, cmy);
			}
			else
			{
				if (DoSep)
					ImgStr = ImageToCMYK_PS(&image, Plate, false);
				else
					ImgStr = GraySc ? ImageToCMYK_PS(&image, -2, false) : 
								ImageToCMYK_PS(&image, -1, false);
			}
#else
			if (DoSep)
				ImgStr = ImageToCMYK_PS(&image, Plate, false);
			else
				ImgStr = GraySc ? ImageToCMYK_PS(&image, -2, false) : ImageToCMYK_PS(&image, -1, 
													false);
#endif
			if (Name == "")
			{
				if (CompAvail)
				{
					PutSeite("currentfile /ASCIIHexDecode filter /FlateDecode filter /ReusableStreamDecode filter\n");
					ImgStr = CompressStr(&ImgStr);
				}
				else
					PutSeite("currentfile /ASCIIHexDecode filter /ReusableStreamDecode filter\n");
				ImgStr = String2Hex(&ImgStr);
				PutSeite(ImgStr);
				ImgStr = "";
				PutSeite("\n>\n");
				PutSeite("/Bild exch def\n");
				ImgStr = MaskToTxt(&iMask, false);
				if (CompAvail)
				{
					PutSeite("currentfile /ASCIIHexDecode filter /FlateDecode filter /ReusableStreamDecode filter\n");
					ImgStr = CompressStr(&ImgStr);
				}
				else
					PutSeite("currentfile /ASCIIHexDecode filter /ReusableStreamDecode filter\n");
				ImgStr = String2Hex(&ImgStr);
				PutSeite(ImgStr);
				PutSeite("\n>\n");
				PutSeite("/Mask exch def\n");
			}
			PutSeite("<<\n");
			PutSeite("  /PaintType   1\n");
			PutSeite("  /PatternType 1\n");
			PutSeite("  /TilingType  3\n");
			PutSeite("  /BBox        [ 0 0 1 1 ]\n");
			PutSeite("  /XStep       2\n");
			PutSeite("  /YStep       2\n");
			PutSeite("  /PaintProc   {\n");
			PutSeite("   pop\n");
			PutSeite("   1 1 1 1 setcmykcolor\n");
			PutSeite("   <<\n");
			PutSeite("   /ImageType 1\n");
			PutSeite("   /Height    " + IToStr(h) + "\n");
			PutSeite("   /Width     " + IToStr(w) + "\n");
			PutSeite("   /ImageMatrix [" + IToStr(w) + " 0 0 " + IToStr(-h) + " 0 " + IToStr(h) 
				+"]\n");
			if (DoSep)
				PutSeite("   /Decode [1 0]\n");
			else
				PutSeite( GraySc ? "   /Decode [1 0]\n" : "   /Decode [0 1 0 1 0 1 0 1]\n" );
			PutSeite("   /BitsPerComponent 8\n");
			PutSeite("   /DataSource "+PSEncode(Name)+"Bild\n");
			PutSeite("   >>\n");
			PutSeite("   image\n");
			PutSeite("   }\n");
			PutSeite(">> matrix makepattern setpattern\n");
			PutSeite("<< /ImageType 1\n");
			PutSeite("   /Width " + IToStr(w) + "\n");
			PutSeite("   /Height " + IToStr(h) + "\n");
			PutSeite("   /BitsPerComponent 1\n");
			PutSeite("   /Decode [1 0]\n");
			PutSeite("   /ImageMatrix [" + IToStr(w) + " 0 0 " + IToStr(-h) + " 0 " + IToStr(h) + "]\n");
			PutSeite("   /DataSource "+PSEncode(Name)+"Mask\n");
			PutSeite(">>\n");
			PutSeite("imagemask\n");
			if (Name != "")
			{
				PutSeite(PSEncode(Name)+"Bild resetfile\n");
				PutSeite(PSEncode(Name)+"Mask resetfile\n");
			}
		}
		else
		{
			PutSeite("<< /ImageType 1\n");
			PutSeite("   /Width " + IToStr(w) + "\n");
			PutSeite("   /Height " + IToStr(h) + "\n");
			PutSeite("   /BitsPerComponent 8\n");
			if (DoSep)
				PutSeite("   /Decode [1 0]\n");
			else
				PutSeite( GraySc ? "   /Decode [1 0]\n" : "   /Decode [0 1 0 1 0 1 0 1]\n");
			PutSeite("   /ImageMatrix [" + IToStr(w) + " 0 0 " + IToStr(-h) + " 0 " + IToStr(h) + 
					"]\n");
			if (Name != "")
			{
				PutSeite("   /DataSource "+PSEncode(Name)+"Bild >>\n");
				PutSeite("image\n");
				PutSeite(PSEncode(Name)+"Bild resetfile\n");
			}
			else
			{
				PutSeite ( CompAvail ? "   /DataSource currentfile /ASCIIHexDecode filter /FlateDecode filter >>\n" :
							"   /DataSource currentfile /ASCIIHexDecode filter >>\n");
				PutSeite("image\n");
#ifdef HAVE_CMS
				if ((CMSuse) && (UseProf))
				{
					if (DoSep)
						ImgStr = ImageToCMYK_PS(&image2, Plate, cmy);
					else
						ImgStr = GraySc ? ImageToCMYK_PS(&image2, -2, cmy) : 
									ImageToCMYK_PS(&image2, -1, cmy);
				}
				else
				{
					if (DoSep)
						ImgStr = ImageToCMYK_PS(&image, Plate, false);
					else
						ImgStr = GraySc ? ImageToCMYK_PS(&image, -2, false) : 
									ImageToCMYK_PS(&image, -1, false);
				}
#else
				if (DoSep)
					ImgStr = ImageToCMYK_PS(&image, Plate, false);
				else
					ImgStr = GraySc ? ImageToCMYK_PS(&image, -2, false) : 		
								ImageToCMYK_PS(&image, -1, false);
#endif
				if (CompAvail)
					ImgStr = CompressStr(&ImgStr);
				ImgStr = String2Hex(&ImgStr);
				PutSeite(ImgStr);
				PutSeite("\n>\n");
			}
		}
	}
}


void PSLib::PS_plate(int nr)
{
	switch (nr)
	{
		case 0:
			PutSeite("%%PlateColor Black\n");
			PutSeite("/setcmykcolor {exch pop exch pop exch pop 1 exch sub oldsetgray} bind def\n");
			PutSeite("/setrgbcolor {pop pop pop 1 oldsetgray} bind def\n");
			break;
		case 1:
			PutSeite("%%PlateColor Cyan\n");
			PutSeite("/setcmykcolor {pop pop pop 1 exch sub oldsetgray} bind def\n");
			PutSeite("/setrgbcolor {pop pop oldsetgray} bind def\n");
			break;
		case 2:
			PutSeite("%%PlateColor Magenta\n");
			PutSeite("/setcmykcolor {pop pop exch pop 1 exch sub oldsetgray} bind def\n");
			PutSeite("/setrgbcolor {pop exch pop oldsetgray} bind def\n");
			break;
		case 3:
			PutSeite("%%PlateColor Yellow\n");
			PutSeite("/setcmykcolor {pop exch pop exch pop 1 exch sub oldsetgray} bind def\n");
			PutSeite("/setrgbcolor {exch pop exch pop oldsetgray} bind def\n");
			break;
	}
	Plate = nr;
	DoSep = true;
}

void PSLib::PS_setGray()
{
	GraySc = true;
}

void PSLib::PDF_Bookmark(QString text, uint Seite)
{
	PutSeite("[/Title ("+text+") /Page "+IToStr(Seite)+" /View [/Fit]\n");
	PutSeite("/OUT pdfmark\n");
	isPDF = true;
}

void PSLib::PDF_Annotation(QString text, double x, double y, double b, double h)
{
	PutSeite("[ /Rect [ "+ToStr(static_cast<int>(x))+" "+ToStr(static_cast<int>(y))
			+" "+ToStr(static_cast<int>(b))+" "+ToStr(static_cast<int>(h))+" ]\n");
	PutSeite("  /Contents ("+text+")\n  /Open false\n");
	PutSeite("/ANN pdfmark\n");
	isPDF = true;
}


void PSLib::PS_close()
{
	PutDoc("%%Trailer\n");
	PutDoc("end\n");
	PutDoc("%%EOF\n");
	Spool.close();
}


void PSLib::PS_insert(QString i)
{
	PutDoc(i);
}

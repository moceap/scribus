/**
    HarfBuzz glue

    Author: Hasen il Judy 
 */

#include <QString>
#include <QVector>
#include <QDebug>

#include "shaper.h"
#include "fonts/ftface.h"
#include "fonts/scface.h"

//pointless proxy
HB_Script charScript(uint cp)
{
    return code_point_to_script(cp);
}

HB_Script stringScript(QVector<uint> ustr)
{
    HB_Script script = charScript(ustr[0]);

    if(script == HB_Script_Inherited) 
    {
        //oh no, generic script! grab the first
        //script we can find
        for(int i = 0; i < ustr.size(); i++)
        {
            HB_Script script_candidate = charScript(ustr[i]);
            if(script_candidate != script)
            {
                script = script_candidate;
                break;
            }
        }
    }
    if(script == HB_Script_Inherited)
    {
        script = HB_Script_Common;
    }

    return script;
}

/**
    The FT_Face should be initialized already in scribus

    based on tests/fuzzing/fuzz.cc
 */
ShaperFontInfo::ShaperFontInfo(ScFace scface)
{
    FT_Library library = 0;
    // XXX: deal with error
    bool error = FT_Init_FreeType(&library);
    FT_Face face = 0;
    error = FT_New_Face(library, 
            scface.fontFilePath().toUtf8().data(), 
            scface.faceIndex(), &face);

    hbFace = HB_NewFace(face, hb_freetype_table_sfnt_get);
    hbFont.klass = &hb_freetype_class;
    hbFont.userData = face;
    hbFont.x_ppem  = face->size->metrics.x_ppem;
    hbFont.y_ppem  = face->size->metrics.y_ppem;
    hbFont.x_scale = face->size->metrics.x_scale;
    hbFont.y_scale = face->size->metrics.y_scale;
}


/**
    based on tests/fuzzing/fuzz.cc
 */
ShaperItemInfo::ShaperItemInfo(ShaperFontInfo *font)
{
    shaper_item.kerning_applied = false;
    shaper_item.string = (HB_UChar16 *) str;
    shaper_item.stringLength = 0;
    shaper_item.item.bidiLevel = 0;
    shaper_item.shaperFlags = 0;
    shaper_item.font = &(font->hbFont);
    shaper_item.face = font->hbFace;
    shaper_item.glyphIndicesPresent = false;
    shaper_item.initialGlyphCount = 0;

    shaper_item.glyphs = new HB_Glyph[maxLength];
    shaper_item.attributes = new HB_GlyphAttributes[maxLength];
    shaper_item.advances = new HB_Fixed[maxLength];
    shaper_item.offsets = new HB_FixedPoint[maxLength];
    shaper_item.log_clusters = new unsigned short[maxLength];
    shaper_item.num_glyphs = maxLength; // XXX really?
}

ShaperItemInfo::~ShaperItemInfo()
{
    delete[] shaper_item.glyphs;
    delete[] shaper_item.attributes;
    delete[] shaper_item.advances;
    delete[] shaper_item.offsets;
    delete[] shaper_item.log_clusters;
}

/**
    Do the shaping for this string.

    Assume str is an item, i.e. same script across the entire string
 */
ShaperOutput ShaperItemInfo::shapeItem(QString str)
{
    if(str.length() == 0)
        return ShaperOutput();

    QVector<uint> ustr = str.toUcs4(); //XXX let stringScript work with utf16

    shaper_item.string = str.utf16();
    shaper_item.stringLength = str.length();
    shaper_item.item.script = stringScript(ustr);
    shaper_item.item.pos = 0;
    shaper_item.item.length = str.length();

    HB_ShapeItem(&shaper_item);

    return ShaperOutput(&shaper_item);
}

/**
    Use HarfBuzz to shape segment in question
 */
void shapeGlyphs(StoryText *itemText, int startIndex, int endIndex)
{
    QString text = itemText->text(startIndex, endIndex-startIndex); //this one takes pos, len

    ScFace scface = itemText->item(startIndex)->font();

    ShaperFontInfo font(scface);
    ShaperItemInfo shaper(&font);
    ShaperOutput out = shaper.shapeItem(text);

    //out.glyphs has our glyphs
    for(int i = 0; i < out.num_glyphs; i++)
    {
        itemText->item(startIndex + i)->glyph.glyph = out.glyphs[i];
        // itemText->item(startIndex + i)->glyph.xadvance = out.advances[i];
        qDebug() << "advance[" << i << "] =" << out.advances[i];
        qDebug() << "offsets[" << i << "] =" << out.offsets[i].x;
    }
}


#ifndef SHAPER_H
#define SHAPER_H

#include "pageitem_textframe.h"
extern "C" {
#include "harfbuzz/harfbuzz-shaper.h"
#include "harfbuzz/harfbuzz-unicode.h"
#include "harfbuzz/harfbuzz-freetype.h"
}

HB_Script charScript(uint cp);
HB_Script stringScript(QVector<uint> ustr);

void shapeGlyphs(StoryText *itemText, int startIndex, int endIndex);

/**
    This is an attempt to encapsulate and streamline the font stuff
    in the harfbuzz API with freetype
 */
class ShaperFontInfo
{
public:
    HB_Face hbFace;
    HB_FontRec hbFont;

public:    
    /**
        The FT_Face should be initialized already in scribus

        based on tests/fuzzing/fuzz.cc
     */
    ShaperFontInfo(ScFace face);
};


struct ShaperOutput
{
    hb_uint32 num_glyphs;
    HB_Glyph* glyphs;
    HB_GlyphAttributes* attributes;
    HB_Fixed* advances;
    HB_FixedPoint* offsets;
    ushort* log_clusters;

    ShaperOutput()
    {
        num_glyphs = 0;
        glyphs = 0;
        attributes = 0;
        advances = 0;
        offsets = 0;
        log_clusters = 0;
    }

    ShaperOutput(HB_ShaperItem *item)
    {
        num_glyphs = item->num_glyphs;
        glyphs = item->glyphs;
        attributes = item->attributes;
        advances = item->advances;
        offsets = item->offsets;
        log_clusters = item->log_clusters;
    }

};

/**
    Attempt to simplify and streamline the HarfBuzz shaper item stuff
 */
class ShaperItemInfo
{
private:
    HB_ShaperItem shaper_item;
    static const int maxLength = 1024;
    uint8_t str[maxLength];

public:
    ShaperItemInfo(ShaperFontInfo *font);
    ~ShaperItemInfo();
    ShaperOutput shapeItem(QString str);
};

#endif


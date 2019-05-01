/*
    File: font.hpp
    Author: Philip Haynes
    Utilities to load data from font files.
    TODOs:
        1) Support loading of data from fonts.
        2) Organize glyph data, expose combination glyphs to user.
        3) Add basic rendering of vector-based glyphs.
        4) Add support for bitmapped glyphs.
*/

#ifndef FONT_HPP
#define FONT_HPP

#include "common.hpp"
#include "log_stream.hpp"

#include <fstream>

namespace font {

    extern String error;
    extern io::logStream cout;

    // These are the basic types used for TrueType fonts
    typedef i16 shortFrac_t;
    union Fixed_t {
        struct {
            i16 iPart;
            u16 fPart;
        }; // For the actual fractions
        struct {
            u16 major;
            u16 minor;
        }; // For versions
    };
    typedef i16 FWord_t;
    typedef u16 uFWord_t;
    typedef i16 F2Dot14_t;
    typedef i64 longDateTime_t;

    union Tag_t {
        u32 data;
        char name[4];
        Tag_t()=default;
        constexpr Tag_t(const u32 in);
        constexpr Tag_t(const char *in);
    };

    constexpr Tag_t operator "" _Tag(const char*, const size_t);
    Tag_t operator "" _Tag(const u64);

    namespace tables {

        u32 Checksum(u32 *table, u32 length);

        /*  struct: Record
            Author: Philip Haynes
            Contains information about one table.   */
        struct Record {
            Tag_t tableTag;
            u32 checkSum;
            u32 offset; // Offset from beginning of the font file.
            u32 length;

            void Read(std::ifstream &file);
        };

        /*  struct: Offset
            Author: Philip Haynes
            Contains information about the tables in the font file.     */
        struct Offset {
            Tag_t sfntVersion;
                // 0x00010000 for TrueType outlines
                // 0x74727565 "true" for TrueType
                // 0x74797031 "typ1" for old-style PostScript
                // 0x4F54544F "OTTO" for OpenType fonts containing CFF data
            u16 numTables;
            u16 searchRange; // (maximum power of 2 that's <= numTables) * 16
            u16 entrySelector; // log2(maximum power of 2 that's <= numTables)
            u16 rangeShift; // numTables * 16 - searchRange
            Array<tables::Record> tables;
            void Read(std::ifstream &file);
        };

        /*  struct: TTCHeader
            Author: Philip Haynes
            Contains information about the fonts contained in this collection.  */
        struct TTCHeader {
            Tag_t ttcTag;
            Fixed_t version;
            u32 numFonts;
            Array<u32> offsetTables; // Offsets to the individial offset tables
            Tag_t dsigTag;
            u32 dsigLength;
            u32 dsigOffset;
            bool Read(std::ifstream &file);
        };

        struct cmap_encoding {
            u16 platformID;
            u16 platformSpecificID;
            u32 offset; // Bytes from beginning of cmap table
            void EndianSwap();
        };

        struct cmap_index {
            u16 version; // Must be set to zero
            u16 numberSubtables; // How many encoding subtables there are
            void EndianSwap();
        };

        /*  struct: head
            Author: Philip Haynes
            Contains all the information in a standard 'head' table.    */
        struct head {
            Fixed_t version;
            Fixed_t fontRevision;
            u32 checkSumAdjustment;
            u32 magicNumber;
            u16 flags;
                // bit 0  - y value of 0 specifies baseline
                // bit 1  - x position of left most black bit is LSB
                // bit 2  - scaled point size and actual point size will differ
                // bit 3  - use integer scaling instead of fractional
                // bit 4  - (Microsoft - OTF) Instructions may alter advance width
                // bit 5  - Intended to be laid out vertically (i.e. x-coord 0 is the vertical baseline)
                // bit 6  - Must be zero
                // bit 7  - Requires layout for correct linguistic rendering
                // bit 8  - AAT font which has one or more metamorphosis effects by default
                // bit 9  - Contains any strong right-to-left glyphs
                // bit 10 - Contains Indic-style rearrangement effects.
                // bit 11 - (Adobe - OTF) Font data is "lossless". The DSIG table may be invalidated.
                // bit 12 - (Adobe - OTF) Font converted (produce compatible metrics)
                // bit 13 - (Adobe - OTF) Font optimized for ClearType™
                // bit 14 - Glyphs are simply generic symbols for code point ranges
                // bit 15 - Reserved, set to zero.
            u16 unitsPerEm; // range from 64 to 16384
            longDateTime_t created; // international date
            longDateTime_t modified; // international date
            FWord_t xMin; // for all glyph bounding boxes
            FWord_t yMin; // ^^^
            FWord_t xMax; // ^^^
            FWord_t yMax; // ^^^
            u16 macStyle;
                // bit 0 - bold
                // bit 1 - italic
                // bit 2 - underline
                // bit 3 - outline
                // bit 4 - shadow
                // bit 5 - condensed (narrow)
                // bit 6 - extended
            u16 lowestRecPPEM; // Smallest readable size in pixels
            i16 fontDirectionHint;
                // 0  - Mixed directional glyphs
                // 1  - Only strongly left to right
                // 2  - Like 1 but contains neutrals
                // -1 - Only strongly right to left
                // -2 - Like -1 but contains neutrals
            i16 indexToLocFormat; // 0 for short offsets, 1 for long
            i16 glyphDataFormat; // 0 for current format
            void EndianSwap();
        };

        // Same exact thing, only used to indicate that the font doesn't
        // have any glyph outlines and only contains embedded bitmaps.
        typedef head bhed;

    }

    /*  struct: Font
        Author: Philip Haynes
        Allows loading a single font file, and getting useful information from them.    */
    struct Font {
        struct {
            std::ifstream file;
            tables::TTCHeader ttcHeader;
            Array<tables::Offset> offsetTables;
            // In the case of font collections, we should determine which tables
            // are unique since some of them will be shared between fonts.
            Array<tables::Record> uniqueTables;
            u32 offsetMin = UINT32_MAX;
            u32 offsetMax = 0;
            // Holds all of the tables, which can then be read by referencing
            // any of the offsetTables and finding the desired ones using tags
            Array<char> tableData;
        } data;
        String filename = String(false);

        bool Load();
    };

}

#endif // FONT_HPP

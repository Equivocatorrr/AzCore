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

        // All the structs used to represent data extracted directly from the file cannot have implicit alignment
        #pragma pack(1)

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
        static_assert(sizeof(Record) == 16);

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
            Array<Record> tables;
            void Read(std::ifstream &file);
        };
        static_assert(sizeof(Offset) == 12 + sizeof(Array<Record>));

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
        static_assert(sizeof(TTCHeader) == 24 + sizeof(Array<u32>));

        struct cmap_encoding {
            u16 platformID;
            u16 platformSpecificID;
            u32 offset; // Bytes from beginning of cmap table
            void EndianSwap();
        };
        static_assert(sizeof(cmap_encoding) == 8);

        struct cmap_index {
            u16 version; // Must be set to zero
            u16 numberSubtables; // How many encoding subtables there are
            void EndianSwap();
        };
        static_assert(sizeof(cmap_index) == 4);

        /*
            Here we have the 3 cmap formats supported by Apple's implementations.
            This should be sufficient for reading just about any font file.
            structs:
                cmap_format_any
                    - this one will parse the data from any of the following structs, whichever is appropriate
                cmap_format0
                    - Windows encoding 0
                    - Supports up to 256 glyphs with one 8-bit index per glyph
                cmap_format4
                    - Unicode encoding 3
                    - Windows encoding 1
                    - Support for Unicode Basic Multilingual Plane (UCS-2)
                cmap_format12
                    - Unicode encoding 4
                    - Microsoft encoding 10
                    - Support for full Unicode (UCS-4)
        */
        struct cmap_format_any {
            u16 format; // Varies. Used to determine which kind of table we are.
            // Will polymorph if format is supported or return false to indicate it's not supported
            bool EndianSwap();
        };

        struct cmap_format0 {
            u16 format;                     // Should be 0
            u16 length;                     // Length in bytes of the subtable, including header. Should be 262.
            u16 language;                   // Should be 0 for tables that aren't Macintosh platform cmaps.
            u8 glpyhIndexArray[256];        // The mapping
            void EndianSwap();
        };
        static_assert(sizeof(cmap_format0) == 262);

        struct cmap_format4 {
            u16 format;                     // Should be 4
            u16 length;
            u16 language;
            // Can't say I understand why these are how they are
            u16 segCountX2;                 // 2 * segCount
            u16 searchRange;                // 2 * (2*floor(log2(segCount)))
            u16 entrySelector;              // log2(searchRange/2)
            u16 rangeShift;                 // (2 * segCount) - searchRange
            // The data past this point is variable-sized like so
            // u16 endCode[segCount];       // Ending character code for each segment, last = 0xFFFF
            // u16 reservedPad;             // Should be zero.
            // u16 startCode[segCount];     // Starting character code for each segment.
            // u16 idDelta[segCount];       // Delta for all character codes in segment.
            // u16 idRangeOffset[segCount]; // Offset in bytes to glpyhIndexArray, or 0
            // u16 glpyhIndexArray[segCount];
            void EndianSwap();
            // Easy accessors
            inline u16& endCode(const u16 i) {
                return *(&rangeShift + 1 + i);
            }
            inline u16& startCode(const u16 i) {
                return *(&rangeShift + segCountX2/2 + 2 + i);
            }
            inline u16& idDelta(const u16 i) {
                return *(&rangeShift + segCountX2 + 2 + i);
            }
            inline u16& idRangeOffset(const u16 i) {
                return *(&rangeShift + segCountX2*3/2 + 2 + i);
            }
            inline u16& glyphIndexArray(const u16 i) {
                return *(&rangeShift + segCountX2*2 + 2 + i);
            }
        };
        static_assert(sizeof(cmap_format4) == 14);

        struct cmap_format12_group {
            u32 startCharCode;              // First character code in this group
            u32 endCharCode;                // Last character code in this group
            u32 startGlpyhCode;             // Glyph index corresponding to the starting character code.
            void EndianSwap();
        };
        static_assert(sizeof(cmap_format12_group) == 12);

        struct cmap_format12 {
            Fixed_t format; // Should be 12.0
            u32 length; // Byte length of this subtable including the header.
            u32 language;
            u32 nGroups; // Number of groupings which follow
            //  The data past this point is variable-sized like so
            // cmap_format12_group groups[nGroups];
            void EndianSwap();
            inline cmap_format12_group& groups(const u32 i) {
                return *((cmap_format12_group*)((char*)this + sizeof(cmap_format12) + sizeof(cmap_format12_group) * i));
            }
        };
        static_assert(sizeof(cmap_format12) == 16);

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
        static_assert(sizeof(head) == 54);

        // Same exact thing, only used to indicate that the font doesn't
        // have any glyph outlines and only contains embedded bitmaps.
        typedef head bhed;

        /*  struct: maxp
            Author: Philip Haynes
            Maximum Profile. Necessary for its numGlyphs data.      */
        struct maxp {
            Fixed_t version;
            u16 numGlyphs;
            // If version is 1.0 then the following data is valid, otherwise ignore them
            u16 maxPoints;              // Max points in a non-composite glyph
            u16 maxContours;            // Max contours in a non-composite glyph
            u16 maxCompositePoints;     // Max points in a composite glyph
            u16 maxCompositeContours;   // Max contours in a composite glyph
            u16 maxZones;               // 1 if instructions don't use the Twilight Zone (Z0) or 2 if they do
            u16 maxTwilightPoints;      // Maximum points used in Z0
            u16 maxStorage;             // Number of storage area locations
            u16 maxFunctionDefs;        // Number of FDEFs, equal to the highest function number +1
            u16 maxInstructionDefs;     // Number of IDEFs.
            // Maximum stack depth across Font Program ('fpgm' table),
            // CVT Program ('prep' table) and all glyph instructions (in 'glyf' table)
            u16 maxStackElements;
            u16 maxSizeOfInstructions;  // Maximum byte count for glyph instructions
            u16 maxComponentElements;   // Maximum number of components referenced at top level for any composite glyph
            u16 maxComponentDepth;      // Maximum levels of recursion; 1 for simple components
            void EndianSwap();
        };
        static_assert(sizeof(maxp) == 32);

        /*  struct: loca
            Author: Philip Haynes
            Index to Location. Used to carry the offsets into the 'glyf' table of character indices. */
        struct loca {
            // All data in this table is variable-sized in the following forms
            // Short version, for when head.indexToLocFormat == 0
            // u16 offsets[maxp.numGlyphs + 1]; // This version is the actual offset / 2
            // Long version, for when head.indexToLocFormat == 1
            // u32 offsets[maxp.numGlyphs + 1];
            void EndianSwap(u16 numGlyphs, bool longOffsets);
            inline u16& offsets16(const u16 i) {
                return *((u16*)this + i);
            }
            inline u32& offsets32(const u16 i) {
                return *((u32*)this + i);
            }
        };

        /*  struct: glyf_header
            Author: Philip Haynes
            Beginning of every glyph definition.        */
        struct glyf_header {
            i16 numberOfContours; // if >= 0, it's a simple glyph. if < 0 then it's a composite glyph.
            i16 xMin;
            i16 yMin;
            i16 xMax;
            i16 yMax;
            void EndianSwap();
        };
        static_assert(sizeof(glyf_header) == 10);

        /*  struct: glyf
            Author: Philip Haynes
            The actual glyph outline data. Cannot be parsed correctly without the loca table.      */
        struct glyf {
            /*  All data in this table is of variable size AND offset
                Since the actual offsets of glyphs depend on the loca table,
                I'm just going to describe the anatomy of a single glyph:
            glyf_header header;

                For a simple glyph:
            u16 endPtsOfContours[header.numberOfContours];
            u16 instructionLength;
            u8 instructions[instructionLength];
            u8 flags[...]; // Actual array size varies based on contents according to the following rules:
                Bit 0: Point is on-curve
                Bit 1: xCoord is 1 byte, else xCoord is 2 bytes
                Bit 2: yCoord is 1 byte, else yCoord is 2 bytes
                Bit 3: Repeat this set of flags n times. n is the next u8.
                Bit 4: Dual meaning
                    - If bit 1 is set:
                        - 1: xCoord is positive
                        - 0: xCoord is negative
                    - If bit 1 is not set:
                        - 1: xCoord is the same as the previous and doesn't have an entry in the xCoords array
                        - 0: xCoord in the array is a delta vector from the previous xCoord
                Bit 5: Same as bit 4, but refers to bit 2 and yCoord
            u8 or i16 xCoord[...];
            u8 or i16 yCoord[...];

                For a compound glyph:
            u16 flags;
                Bit 0:  arguments are 2-byte, else arguments are 1 byte
                Bit 1:  arguments are xy values, else arguments are points
                Bit 2:  Round xy values to grid
                Bit 3:  There is a simple scale for the component, else scale is 1.0
                Bit 4:  obsolete
                Bit 5:  At least one additional glyph follows this one
                Bit 6:  x direction has different scale than y direction
                Bit 7:  There is a 2-by-2 transformation matrix that will be used to scale the component
                Bit 8:  Instructions for the component character follow the last component
                Bit 9:  Use metrics from this component for the compound glyph
                Bit 10: The components of this compound glyph overlap
            u16 glyphIndex; // Glyph index of the component
                The arguments are unsigned if they're points, and signed if they're offsets.
            i16 or u16 or i8 or u8 argument1; // X-offset for component or point number
            i16 or u16 or i8 or u8 argument2; // Y-offset for component or point number
                In the case of Instructions as per flags bit 8, they come after the last component as follows:
            u16 numInstructions;
            u8 instructions[numInstructions];
            */
            void EndianSwap(loca *loc, u16 numGlyphs, bool longOffsets);
            static void EndianSwapSimple(glyf_header *header);
            static void EndianSwapCompound(glyf_header *header);
        };

        namespace cffs {

            /*
            Information on the CFF table is courtesy of Adobe.
            http://wwwimages.adobe.com/www.adobe.com/content/dam/acom/en/devnet/font/pdfs/5176.CFF.pdf
            */

            // Data types used in a CFF table
            typedef u8 Card8;       // 1-byte unsigned integer. Range: 0-255
            typedef u16 Card16;     // 2-byte unsigned integer. Range: 0-65535
            typedef u8 OffSize;     // Size of an Offset. Range: 1-4
            typedef u8 Offset8;     // When OffSize is 1
            typedef u16 Offset16;   // When OffSize is 2
            struct Offset24 {       // When OffSize is 3
                u8 bytes[3];        // Always big-endian
                u32 value() const;
                void set(u32 val);
            };
            // I'm probably being paranoid here, but if these assertions failed it would be a huge pain to debug.
            // I believe "#pragma pack(1)" could fix it, but according to the C99 standard, these should pass.
            // I guess it depends on how the C++ compiler conforms to C memory standards. GCC passes.
            static_assert(sizeof(Offset24) == 3);
            static_assert(sizeof(Offset24)*3 == sizeof(Offset24[3]));
            typedef u32 Offset32;   // When OffSize is 4
            typedef u16 SID;        // 2-byte string identifier. Range: 0-64999

            constexpr u32 nStdStrings = 391;
            extern const char *stdStrings[nStdStrings];
            extern const SID stdEncoding0[256];
            extern const SID stdEncoding1[256];
            inline const SID stdCharset0(const SID& in) {
                return in > 228 ? 0 : in;
            }
            extern const SID stdCharset1[166];
            extern const SID stdCharset2[87];

            /*      Charsets
                Author: Philip Haynes
                With different formats come different responsibilities.
                struct: charset_format_any
                Can polymorph into the appropriate format, according to its format byte.
                struct: charset_format0
                Uses an array of SIDs with a size that depends on the CharStrings INDEX
                struct: charset_format1
                Uses an array of structs that hold SIDs and the number of glyphs left in the array as a byte.
                struct: charset_format2
                Same as format1 but uses Card16 instead of Card8 in the struct.     */
            struct charset_format_any {
                Card8 format;
                // The rest of the data is specific to the format.
                void EndianSwap(Card16 nGlyphs);
            };

            struct charset_format0 {
                Card8 format; // Should be 0
                // The rest of the data is variable-sized like so:
                // SID glyph[nGlyphs-1]; // nGlyphs is the value of the count field in the CharStrings INDEX
                void EndianSwap(Card16 nGlyphs);
            };

            struct charset_range1 {
                SID first; // First glyph in range.
                Card8 nLeft; // Glyphs left in range (excluding first)
                void EndianSwap();
            };
            static_assert(sizeof(charset_range1) == 3);

            struct charset_range2 {
                SID first; // First glyph in range.
                Card16 nLeft; // Glyphs left in range (excluding first)
                void EndianSwap();
            };
            static_assert(sizeof(charset_range2) == 4);

            struct charset_format1 {
                Card8 format; // Should be 1
                // The rest of the data is variable-sized like so:
                // charset_range1 range[...];
                void EndianSwap(Card16 nGlyphs);
            };

            struct charset_format2 {
                Card8 format; // Should be 2
                // The rest of the data is variable-sized like so:
                // charset_range2 range[...];
                void EndianSwap(Card16 nGlyphs);
            };

            /*          FDSelect & FDArray (Font Dict INDEX)
                Author: Philip Haynes
                Font DICT association for each glyph.
                struct: FDSelect_any
                Basic type that will polymorph based on its format.
                struct: FDSelect_format0
                For relatively randomly-ordered fonts.
                struct: FDSelect_range3
                Used for format3.
                struct: FDSelect_format3
                For more sequentially-ordered fonts.       */

            struct FDSelect_any {
                Card8 format;
                void EndianSwap();
            };

            struct FDSelect_format0 {
                Card8 format; // Should be 0
                // Card8 fds[nGlyphs]; // nGlyphs is the count field of the CharStrings INDEX
            };

            struct FDSelect_range3 {
                Card16 first;   // First glyph in range
                Card8 fd;       // Which index of FDArray the range maps to.
            };
            static_assert(sizeof(FDSelect_range3) == 3);

            struct FDSelect_format3 {
                Card8 format; // Should be 3
                Card16 nRanges;
                // FDSelect_range3 range[nRanges];
                // Card16 sentinel; // Used to delimit the last range element
                void EndianSwap();
            };
            static_assert(sizeof(FDSelect_format3) == 3);

            /*  struct: dict
                Author: Philip Haynes
                Information parsed from DICT charstrings with appropriate defaults.       */
            struct dict {
                // NOTE: Should we use f64 for all the real numbers since they're
                //       not explicitly integers in any cases, at least per the spec?
                SID version;
                SID Notice;
                SID Copyright;
                SID FullName;
                SID FamilyName;
                SID Weight;
                bool isFixedPitch = false;
                i32 ItalicAngle = 0;
                i32 UnderlinePosition = -100;
                i32 UnderlineThickness = 50;
                i32 PaintType = 0;
                i32 CharstringType = 2;
                Array<f32> FontMatrix = {0.001, 0.0, 0.0, 0.001, 0.0, 0.0};
                i32 UniqueID;
                Array<i32> FontBBox = {0, 0, 0, 0};
                f32 StrokeWidth = 0.0;
                Array<i32> XUID;
                i32 charset = 0;
                i32 Encoding = 0;
                i32 CharStrings = -1;
                struct {
                    i32 offset;
                    i32 size;
                } Private;
                i32 SyntheticBase;
                SID PostScript;
                SID BaseFontName;
                Array<i32> BaseFontBlend; // Delta
                //
                //          Private DICT values
                //
                Array<i32> BlueValues; // Delta
                Array<i32> OtherBlues; // Delta
                Array<i32> FamilyBlues; // Delta
                Array<i32> FamilyOtherBlues; // Delta
                f32 BlueScale = 0.039625;
                f32 BlueShift = 7;
                f32 BlueFuzz = 1;
                f32 StdHW;
                f32 StdVW;
                Array<f32> StemSnapH; // Delta
                Array<f32> StemSnapV; // Delta
                bool ForceBold = false;
                i32 LanguageGroup = 0;
                f32 ExpansionFactor = 0.06;
                i32 initialRandomSeed = 0;
                i32 Subrs; // Offset to local subrs, relative to start of Private DICT data
                i32 defaultWidthX = 0;
                i32 nominalWidthX = 0;
                //
                //          CIDFont Operator Extensions
                //
                struct {
                    SID registry;
                    SID ordering;
                    i32 supplement;
                } ROS;
                f32 CIDFontVersion = 0;
                f32 CIDFontRevision = 0;
                i32 CIDFontType = 0;
                i32 CIDCount = 8720;
                i32 UIDBase;
                i32 FDArray = -1;
                i32 FDSelect = -1;
                SID FontName;
                void ParseCharString(u8 *data, u32 size);
                void ResolveOperator(u8 **data, u8 *firstOperand);
            };

            struct index {
                Card16 count;               // Number of objects stored in this index
                // If count is 0, then the entire struct is 2 bytes exactly, and everything past this point isn't there.
                OffSize offSize;
                /*  The rest of the data varies in size and offset as follows:
                OffsetXX offset[count+1];   // Data type depend on offSize.
                    Offsets are relative to the byte preceding data.
                Card8 data[...];
                */
                // Returns an Array of offsets into data
                Array<u32> EndianSwap(char **ptr, char** dataStart);
            };
            static_assert(sizeof(index) == 3);

            struct header {
                Card8 versionMajor; // Should be at least 1
                Card8 versionMinor; // We don't care about this unless we want to support extensions
                Card8 size;         // Size of this header, used to locate the Name INDEX since it may vary between versions
                OffSize offSize;    // Specifies the size of all offsets into the CFF data.
                // At least that's what the spec says. It looks like actual fonts don't care about this value.
            };
            static_assert(sizeof(header) == 4);

        };

        /*  struct: cff
            Author: Philip Haynes
            Compact Font Format table.     */
        struct cff {
            cffs::header header; // We probably only need version 1.0???
            /* All data beyond this point is of variable offset
                Starting at an offset of header.size
            cffs::index nameIndex;
            cffs::index dictIndex;
            cffs::index stringsIndex;
            cffs::index gsubrIndex; // Stands for Global Subroutines
            */
            void EndianSwap();
        };

        #pragma pack()
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
            Array<u32> cmaps; // Offset from beginning of tableData for chosen encodings
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

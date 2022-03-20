/*
	File: CFF.hpp
	Author: Philip Haynes
	Structures and utilities for parsing glyphs and whatnot from the CFF table.
*/
#ifndef AZCORE_FONT_CFF_HPP
#define  AZCORE_FONT_CFF_HPP

#include "../common.hpp"

/*
	Information on the CFF table is courtesy of Adobe:
		http://wwwimages.adobe.com/www.adobe.com/content/dam/acom/en/devnet/font/pdfs/5176.CFF.pdf
	Information about PostScript is also courtesy of Adobe:
		https://www.adobe.com/content/dam/acom/en/devnet/actionscript/articles/PLRM.pdf
	Information about Type 1 and Type 2 Charstrings are also from Adobe:
		https://www.adobe.com/content/dam/acom/en/devnet/font/pdfs/T1_SPEC.pdf
		https://www.adobe.com/content/dam/acom/en/devnet/font/pdfs/5177.Type2.pdf
*/

namespace AzCore {
namespace font {
namespace tables {
namespace cffs {

	#pragma pack(1)

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
	inline SID stdCharset0(const SID& in) {
		return in > 228 ? 0 : in;
	}
	extern const SID stdCharset1[166];
	extern const SID stdCharset2[87];

	/*  struct: Operand
		Author: Philip Haynes
		For coherency, operands are parsed from the charstrings and then put into this union.   */
	struct Operand {
		enum Type {
			// Some of these are not relevant to fonts
			INVALID=0,
			// Simple objects
			// FONT_ID, // I'm not sure if this is an SID or an index
			INTEGER,
			BOOLEAN=INTEGER,
			MARK=INTEGER, // Denotes a position on the operand stack
			NAME,
			NULL_OP,
			OPERATOR,
			REAL,
			// Composite objects
			ARRAY,
			DICTIONARY,
			// FILE,
			// GSTATE,
			// PACKEDARRAY,
			// SAVE,
			STRING
		} type;
		enum Dict {
			// The first three here are PostScript dictionaries.
			// DICT_SYSTEM,
			// DICT_GLOBAL,
			// DICT_USER,
			// CFF dictionaries
			DICT_TOP,
			DICT_FONT, // For CID-keyed fonts
			DICT_PRIVATE,
		};
		struct _Data {
			u64 _data[2];
			_Data() = default;
			_Data(u64 in) : _data{in, in} {}
			inline bool operator==(const _Data &other) const {
				return _data[0] == other._data[0] && _data[1] == other._data[1];
			}
		};
		union {
			_Data data;
			bool boolean;
			// i32 fontID;
			i32 integer;
			i32 mark;
			char name[128]; // Max length is 127, so last character must be 0
			struct {
				u8 byte[2];
			} op;
			f32 real;
			// composite
			i32 array; // Index of the Array<Operand> in virtual memory
			Dict dictionary;
			i32 string; // Index of the String in virtual memory
		};

		Operand() = default;
		Operand(f32 in) : type(REAL), real(in) {}
		Operand(i32 in) : type(INTEGER), integer(in) {}
		Operand(bool in) : type(BOOLEAN), boolean(in) {}

		inline i32 ToI32() {
			switch (type) {
				case INTEGER:
					return integer;
				case REAL:
					return (i32)real;
				default:
					return 0;
			}
		}
		inline f32 ToF32() {
			switch (type) {
				case INTEGER:
					return (f32)integer;
				case REAL:
					return real;
				default:
					return 0.0f;
			}
		}
		Operand operator+(const Operand &other) const;
		Operand operator-(const Operand &other) const;
		Operand operator*(const Operand &other) const;
		Operand operator/(const Operand &other) const;
		bool operator==(const Operand &other) const;
		bool operator>(const Operand &other) const;
		Operand operator-() const;
	};

	/*  struct: OperandStack
		Author: Philip Haynes
		Execution of charstrings involves PostScript-style stacks of operands.     */
	struct OperandStack {
		Array<Operand> data{};
		inline void Push(Operand op) {
			data.Append(op);
		}
		inline Operand Pop() {
			Operand op = data.Back();
			data.size--;
			return op;
		}
		// This behavior is unique to CFF dicts
		inline Array<i32> DictArrayI32() {
			Array<i32> array(data.size);
			for (i32 i = 0; i < array.size; i++) {
				array[i] = data[i].ToI32();
			}
			data.size = 0;
			return array;
		}
		inline Array<i32> DictDeltaI32() {
			Array<i32> array(data.size);
			for (i32 i = 0; i < array.size; i++) {
				array[i] = data[i].ToI32();
				if (i > 0) array[i] += array[i-1];
			}
			data.size = 0;
			return array;
		}
		inline Array<f32> DictArrayF32() {
			Array<f32> array(data.size);
			for (i32 i = 0; i < array.size; i++) {
				array[i] = data[i].ToF32();
			}
			data.size = 0;
			return array;
		}
		inline Array<f32> DictDeltaF32() {
			Array<f32> array(data.size);
			for (i32 i = 0; i < array.size; i++) {
				array[i] = data[i].ToF32();
				if (i > 0) array[i] += array[i-1];
			}
			data.size = 0;
			return array;
		}
		inline Operand& operator[](i32 i) {
			static Operand invalid = {Operand::INVALID};
			if (i >= data.size || i < 0) return invalid;
			return data[data.size-i-1];
		}
		inline void Clear() {
			data.size = 0;
		}
	};

	/*      Encodings
		I'm still trying to figure out what these do exactly.
		I'm at a loss really since the Spec is so fucking vague.
		struct: encoding_format_any
		Interface into any of the below formats.
		struct: encoding_format0
		Basic format good for randomly ordered glyphs.
		struct: encoding_forma1
		Defined ranges rather than sparse glyphs. Good for well-ordered glyphs.
		struct: encoding_supplemental
		For glyphs that have more than one encoding, this is how they would be defined.
		*/
	struct encoding_format_any {
		Card8 format; // High-order bit determines if there are any supplemental encodings
	};

	struct encoding_format0 {
		Card8 format; // should be 0x00 or 0x80
		Card8 nCodes; // number of encoded glyphs
		/* variable-sized
		Card8 code[nCodes];
		*/
	};

	struct encoding_range1 {
		Card8 first;
		Card8 nLeft; // Codes left in range excluding first
	};

	struct encoding_format1 {
		Card8 format; // should be 0x01 or 0x81
		Card8 nRanges;
		/* variable-sized
		encoding_range1 range[nRanges];
		*/
	};

	struct encoding_supplement {
		Card8 code; // encoding
		SID glyph; // name
	};

	struct encoding_supplemental {
		Card8 nSups;
		/* variable-sized
		encoding_supplement supplement[nSups];
		*/
	};

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
		bool EndianSwap(Card16 nGlyphs);
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
		bool EndianSwap();
		u32 GetFD(char32 glyphIndex, u32 charStringsCount) const;
	};

	struct FDSelect_format0 {
		Card8 format; // Should be 0
		// Card8 fds[nGlyphs]; // nGlyphs is the count field of the CharStrings INDEX
		u32 GetFD(char32 glyphIndex, u32 charStringsCount) const;
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
		u32 GetFD(char32 glyphIndex, u32 charStringsCount) const;
	};
	static_assert(sizeof(FDSelect_format3) == 3);

	/*  struct: dict
		Author: Philip Haynes
		Information parsed from DICT charstrings with appropriate defaults.       */
	struct dict {
		// NOTE: Should we use f64 for all the real numbers since they're
		//	   not explicitly integers in any cases, at least per the spec?
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
		Array<f32> FontMatrix = {0.001f, 0.0f, 0.0f, 0.001f, 0.0f, 0.0f};
		i32 UniqueID;
		Array<i32> FontBBox = {0, 0, 0, 0};
		f32 StrokeWidth = 0.0;
		Array<i32> XUID;
		i32 charset = 0;
		i32 Encoding = 0;
		i32 CharStrings = -1;
		struct {
			i32 size;
			i32 offset;
		} Private;
		i32 SyntheticBase;
		SID PostScript;
		SID BaseFontName;
		Array<i32> BaseFontBlend; // Delta
		// NOTE: The Top DICT and Private/Font DICTs should probably be separate
		//
		//		  Private DICT values
		//
		/*  BlueValues contains an even number of values in pairs.
			The first pair indicate the baseline overshoot, and the baseline, respectively.
			All other pairs indicate alignment zones for the tops of characters.
			There may be up to 7 pairs.
			Different pairs must be at least 3 units apart from each other and from OtherBlues pairs.
				This distance can be modified by BlueFuzz
			The max distance between values in a pair is constrained by BlueScale. */
		Array<i32> BlueValues; // Delta
		/*  OtherBlues is like BlueValues except for the following differences:
			It only describes bottom pairs.
			There may be up to 5 pairs. */
		Array<i32> OtherBlues; // Delta
		/*  FamilyBlues are pairs like BlueValues that represent alignment zones to use when the
			difference between the individual font zones and family zones are less than 1 pixel
			apart. (This depends on the size of the text being rasterized) */
		Array<i32> FamilyBlues; // Delta
		//  FamilyOtherBlues is a Ditto
		Array<i32> FamilyOtherBlues; // Delta
		// BlueScale determines the size at which overshoot suppression kicks in
		f32 BlueScale = 0.039625f;
		f32 BlueShift = 7;
		/*  BlueFuzz determines how many units away from an alignment zone a top or bottom can be
			before it's no longer considered to be in that alignment zone.
			Distance is BlueFuzz*2 + 1, meaning by default it's 3 units.    */
		f32 BlueFuzz = 1;
		f32 StdHW;
		f32 StdVW;
		Array<f32> StemSnapH; // Delta
		Array<f32> StemSnapV; // Delta
		bool ForceBold = false;
		i32 LanguageGroup = 0;
		// lenIV determines the number of random bytes at the beginning of Type 1 charstrings for encryption
		// i32 lenIV = 4;
		f32 ExpansionFactor = 0.06f;
		i32 initialRandomSeed = 0;
		i32 Subrs=0; // Offset to local subrs, relative to start of Private DICT data
		i32 defaultWidthX = 0;
		i32 nominalWidthX = 0;
		//
		//		  CIDFont Operator Extensions
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
		// Returns the operator's length in bytes
		i32 ResolveOperator(u8 *data, OperandStack &stack);
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
		bool Parse(char **ptr, u8** dataStart, Array<u32> *dstOffsets, bool swapEndian);
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

	#pragma pack()

} // namespace cffs

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
	bool Parse(struct cffParsed *parsed, bool swapEndian);
};

} // namespace tables

} // namespace font

} // namespace AzCore

inline AzCore::font::tables::cffs::Operand abs(AzCore::font::tables::cffs::Operand in) {
	switch (in.type) {
		case AzCore::font::tables::cffs::Operand::INTEGER:
			in.integer = abs(in.integer);
			return in;
		case AzCore::font::tables::cffs::Operand::REAL:
			in.real = abs(in.real);
			return in;
		default:
			return in;
	}
}

#endif // AZCORE_FONT_CFF_HPP

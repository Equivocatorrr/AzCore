/*
    File: font_tables.cpp
    Author: Philip Haynes
*/
#include "font.hpp"

namespace font {
f32 ToF32(const F2Dot14_t& in) {
    f32 out;
    if (in & 0x8000) {
        if (in & 0x4000) {
            out = -2.0;
        } else {
            out = -1.0;
        }
    } else {
        if (in & 0x4000) {
            out = 1.0;
        } else {
            out = 0.0;
        }
    }
    const u16 dot14 = in & 0x3fff;
    out += (f32)dot14 / 16384.0;
    return out;
}

constexpr Tag_t::Tag_t(const u32 in) : data(in) {}

constexpr Tag_t::Tag_t(const char *in) : name{in[0],in[1],in[2],in[3]} {}

constexpr Tag_t operator "" _Tag(const char *name, const size_t size) {
    return Tag_t(name);
}

Tag_t operator "" _Tag(const u64 in) {
    // We keep tags in Big Endian no matter what because they're usually expressed as a string.
    return Tag_t(endianToB((u32)in));
}

Tag_t readTag(std::ifstream &file) {
    Tag_t buffer;
    file.read(buffer.name, 4);
    return buffer;
}

Tag_t bytesToTag(char *buffer) {
    return Tag_t(*reinterpret_cast<const u32*>(buffer));
}

Fixed_t bytesToFixed(char *buffer, const bool swapEndian) {
    return {
        bytesToI16(&buffer[0], swapEndian),
        bytesToU16(&buffer[2], swapEndian)
    };
}

String error = "No Error";
io::logStream cout("font.log");

inline bool operator==(const Tag_t &a, const Tag_t &b) {
    return a.data == b.data;
}

namespace tables {

namespace cffs {

#include "font_cff_std_data.c"

u32 Offset24::value() const {
    return (u32)bytes[0] + ((u32)bytes[1] << 8) + ((u32)bytes[2] << 12);
}

void Offset24::set(u32 val) {
    bytes[0] = val;
    bytes[1] = val >> 8;
    bytes[2] = val >> 12;
}

const char* boolString[2] = {
    "false",
    "true"
};

void OperandPassover(u8 **data) {
    const u8 b0 = **data;
    if (b0 >= 32 && b0 <= 246) {
        (*data)++;
    } else if (b0 >= 247 && b0 <= 254) {
        *data += 2;
    } else if (b0 == 28) {
        *data += 3;
    } else if (b0 == 29) {
        *data += 5;
    } else if (b0 == 30) {
        u8 nibbles[2];
        do {
            (*data)++;
            nibbles[0] = (**data) >> 4;
            nibbles[1] = (**data) & 0x0f;
            for (i32 i = 0; i < 2; i++) {
                if (nibbles[i] == 0xf) {
                    break;
                }
            }
        } while (nibbles[0] != 0xf && nibbles[1] != 0xf);
        (*data)++;
    } else {
        cout << "Operand ERROR " << (u16)b0;
        (*data)++;
    }
}

String OperandString(u8 **data) {
    String out;
    const u8 b0 = **data;
    if (b0 >= 32 && b0 <= 246) {
        out += ToString((i32)b0 - 139);
        (*data)++;
    } else if (b0 >= 247 && b0 <= 254) {
        const u8 b1 = *((*data)+1);
        if (b0 < 251) {
            out += ToString(((i32)b0 - 247)*256 + (i32)b1 + 108);
        } else {
            out += ToString(-((i32)b0 - 251)*256 - (i32)b1 - 108);
        }
        *data += 2;
    } else if (b0 == 28) {
        const u8 b1 = *((*data)+1);
        const u8 b2 = *((*data)+2);
        out += ToString(((i16)b1<<8) | ((i32)b2));
        *data += 3;
    } else if (b0 == 29) {
        const u8 b1 = *((*data)+1);
        const u8 b2 = *((*data)+2);
        const u8 b3 = *((*data)+3);
        const u8 b4 = *((*data)+4);
        out += ToString(((i32)b1<<24) | ((i32)b2<<16) | ((i32)b3<<8) | ((i32)b4));
        *data += 5;
    } else if (b0 == 30) {
        u8 nibbles[2];
        do {
            (*data)++;
            nibbles[0] = (**data) >> 4;
            nibbles[1] = (**data) & 0x0f;
            for (i32 i = 0; i < 2; i++) {
                if (nibbles[i] < 0xa) {
                    out.Append(nibbles[i] + '0');
                } else if (nibbles[i] == 0xa) {
                    out.Append('.');
                } else if (nibbles[i] == 0xb) {
                    out.Append('E');
                } else if (nibbles[i] == 0xc) {
                    out.Append("E-");
                } else if (nibbles[i] == 0xe) {
                    out.Append('-');
                } else {
                    break;
                }
            }
        } while (nibbles[0] != 0xf && nibbles[1] != 0xf);
        (*data)++;
    } else {
        out += "Operand ERROR " + ToString((u16)b0);
        (*data)++;
    }
    return out;
}

i32 OperandI32(u8 **data) {
    i64 out = 0;
    const u8 b0 = **data;
    if (b0 >= 32 && b0 <= 246) {
        out = (i32)b0 - 139;
        (*data)++;
    } else if (b0 >= 247 && b0 <= 254) {
        const u8 b1 = *((*data)+1);
        if (b0 < 251) {
            out = ((i32)b0 - 247)*256 + (i32)b1 + 108;
        } else {
            out = -((i32)b0 - 251)*256 - (i32)b1 - 108;
        }
        *data += 2;
    } else if (b0 == 28) {
        const u8 b1 = *((*data)+1);
        const u8 b2 = *((*data)+2);
        out = ((i32)b1<<8) | ((i32)b2);
        *data += 3;
    } else if (b0 == 29) {
        const u8 b1 = *((*data)+1);
        const u8 b2 = *((*data)+2);
        const u8 b3 = *((*data)+3);
        const u8 b4 = *((*data)+4);
        out = ((i32)b1<<24) | ((i32)b2<<16) | ((i32)b3<<8) | ((i32)b4);
        *data += 5;
    } else if (b0 == 30) {
        u8 nibbles[2];
        i32 dec = -1;
        bool expPos = false;
        bool expNeg = false;
        i32 exponent = 0;
        bool negative = false;
        do {
            (*data)++;
            nibbles[0] = (**data) >> 4;
            nibbles[1] = (**data) & 0x0f;
            for (i32 i = 0; i < 2; i++) {
                if (nibbles[i] < 0xa) {
                    if (expPos) {
                        exponent *= 10;
                        exponent += (i32)nibbles[i];
                    } else if (expNeg) {
                        exponent *= 10;
                        exponent -= (i32)nibbles[i];
                    } else {
                        if (dec > -1) {
                            dec++;
                        }
                        out *= 10;
                        out += (i32)nibbles[i];
                    }
                } else if (nibbles[i] == 0xa) {
                    // Decimal Point
                    dec = 0;
                } else if (nibbles[i] == 0xb) {
                    expPos = true;
                } else if (nibbles[i] == 0xc) {
                    expNeg = true;
                } else if (nibbles[i] == 0xe) {
                    negative = true;
                } else {
                    break;
                }
            }
        } while (nibbles[0] != 0xf && nibbles[1] != 0xf);
        if (dec >= 0) {
            exponent -= dec;
        }
        if (exponent < 0) {
            for (i32 i = exponent; i < 0; i++) {
                out /= 10;
            }
        } else if (exponent > 0) {
            for (i32 i = exponent; i > 0; i--) {
                out *= 10;
            }
        }
        if (negative) {
            out *= -1;
        }
        (*data)++;
    } else {
        cout << "Operand ERROR " << (u16)b0;
        (*data)++;
    }
    return out;
}

f32 OperandF32(u8 **data) {
    f64 out = 0; // We use greater precision for parsing the real values
    const u8 b0 = **data;
    if (b0 >= 32 && b0 <= 246) {
        out = (i32)b0 - 139;
        (*data)++;
    } else if (b0 >= 247 && b0 <= 254) {
        const u8 b1 = *((*data)+1);
        if (b0 < 251) {
            out = ((i32)b0 - 247)*256 + (i32)b1 + 108;
        } else {
            out = -((i32)b0 - 251)*256 - (i32)b1 - 108;
        }
        *data += 2;
    } else if (b0 == 28) {
        const u8 b1 = *((*data)+1);
        const u8 b2 = *((*data)+2);
        out = ((i32)b1<<8) | ((i32)b2);
        *data += 3;
    } else if (b0 == 29) {
        const u8 b1 = *((*data)+1);
        const u8 b2 = *((*data)+2);
        const u8 b3 = *((*data)+3);
        const u8 b4 = *((*data)+4);
        out = ((i32)b1<<24) | ((i32)b2<<16) | ((i32)b3<<8) | ((i32)b4);
        *data += 5;
    } else if (b0 == 30) {
        u8 nibbles[2];
        i32 dec = -1;
        bool expPos = false;
        bool expNeg = false;
        i32 exponent = 0;
        bool negative = false;
        do {
            (*data)++;
            nibbles[0] = (**data) >> 4;
            nibbles[1] = (**data) & 0x0f;
            for (i32 i = 0; i < 2; i++) {
                if (nibbles[i] < 0xa) {
                    if (expPos) {
                        exponent *= 10;
                        exponent += (i32)nibbles[i];
                    } else if (expNeg) {
                        exponent *= 10;
                        exponent -= (i32)nibbles[i];
                    } else {
                        if (dec > -1) {
                            dec++;
                        }
                        out *= 10;
                        out += (i32)nibbles[i];
                    }
                } else if (nibbles[i] == 0xa) {
                    // Decimal Point
                    dec = 0;
                } else if (nibbles[i] == 0xb) {
                    expPos = true;
                } else if (nibbles[i] == 0xc) {
                    expNeg = true;
                } else if (nibbles[i] == 0xe) {
                    negative = true;
                } else {
                    break;
                }
            }
        } while (nibbles[0] != 0xf && nibbles[1] != 0xf);
        if (dec >= 0) {
            exponent -= dec;
        }
        if (exponent < 0) {
            for (i32 i = exponent; i < 0; i++) {
                out /= 10.0d;
            }
        } else if (exponent > 0) {
            for (i32 i = exponent; i > 0; i--) {
                out *= 10.0d;
            }
        }
        if (negative) {
            out *= -1.0;
        }
        (*data)++;
    } else {
        cout << "Operand ERROR " << (u16)b0;
        (*data)++;
    }
    return out;
}

String DictOperatorResolution(u8 **data, u8 *firstOperand) {
    String out;
    u8 operator1 = *(*data)++;
#define GETSID() out += ToString(endianFromB(*(SID*)firstOperand)); firstOperand += 2
#define GETARR()                            \
out += '{';                                 \
for (;*firstOperand != operator1;) {        \
out += OperandString(&firstOperand);    \
if (*firstOperand != operator1) {       \
out += ", ";                        \
}                                       \
}                                           \
out += '}';
#define CASE_VAL(op, var) case op : { out += #var ": "; out += OperandString(&firstOperand); break; }
#define CASE_BOOL(op, var) case op : { out += #var ": "; out += boolString[*firstOperand!=0]; break; }
#define CASE_SID(op, var) case op : { out += #var ": "; GETSID(); break; }
#define CASE_ARR(op, var) case op : { out += #var ": "; GETARR(); break; }
    switch(operator1) {
        case 12: {
            u8 operator2 = *(*data)++;
            switch(operator2) {
                // Private DICT
                CASE_VAL(        9, BlueScale);
                CASE_VAL(       10, BlueShift);
                CASE_VAL(       11, BlueFuzz);
                CASE_ARR(       12, StemSnapH);
                CASE_ARR(       13, StemSnapV);
                CASE_BOOL(      14, ForceBold);
                CASE_VAL(       17, LanguageGroup);
                CASE_VAL(       18, ExpansionFactor);
                CASE_VAL(       19, initialRandomSeed);
                // Top DICT
                CASE_SID(        0, Copyright);
                CASE_BOOL(       1, isFixedPitch);
                CASE_VAL(        2, ItalicAngle);
                CASE_VAL(        3, UnderlinePosition);
                CASE_VAL(        4, UnderlineThickness);
                CASE_VAL(        5, PaintType);
                CASE_VAL(        6, CharstringType);
                CASE_ARR(        7, FontMatrix);
                CASE_VAL(        8, StrokeWidth);
                CASE_VAL(       20, SyntheticBase);
                CASE_SID(       21, PostScript);
                CASE_SID(       22, BaseFontName);
                CASE_ARR(       23, BaseFontBlend);
                // CIDFont-only Operators
                case 30: { out += "Registry: "; GETSID(); out += " Ordering: "; GETSID();
                    out += " Supplement: " + OperandString(&firstOperand); break; }
                CASE_VAL(       31, CIDFontVersion);
                CASE_VAL(       32, CIDFontRevision);
                CASE_VAL(       33, CIDFontType);
                CASE_VAL(       34, CIDCount);
                CASE_VAL(       35, UIDBase);
                CASE_VAL(       36, FDArray);
                CASE_VAL(       37, FDSelect);
                CASE_SID(       38, FontName);
                default: {
                    cout << "Operator Error 12:" << operator2 << std::endl;
                }
            }
            break;
        }
        // Private DICT
        CASE_ARR(        6, BlueValues);
        CASE_ARR(        7, OtherBlues);
        CASE_ARR(        8, FamilyBlues);
        CASE_ARR(        9, FamilyOtherBlues);
        CASE_VAL(       10, StdHW);
        CASE_VAL(       11, StdVW);
        CASE_VAL(       13, UniqueID);
        CASE_VAL(       19, Subrs);
        CASE_VAL(       20, defaultWidthX);
        CASE_VAL(       21, nominalWidthX);
        // Top DICT
        CASE_SID(        0, version);
        CASE_SID(        1, Notice);
        CASE_SID(        2, FullName);
        CASE_SID(        3, FamilyName);
        CASE_SID(        4, Weight);
        CASE_ARR(        5, FontBBox);
        CASE_ARR(       14, XUID);
        CASE_VAL(       15, charset);
        CASE_VAL(       16, Encoding);
        CASE_VAL(       17, CharStrings);
        case 18: { out += "Private: offset: " + OperandString(&firstOperand)
            + ", size: " + OperandString(&firstOperand); break; }
        default: {
            cout << "Operator Error " << operator1 << std::endl;
            break;
        }
    }
#undef CASE_VAL
#undef CASE_BOOL
#undef CASE_SID
#undef CASE_ARR
#undef GETSID
#undef GETARR
    return out;
}

String CharString(u8 *start, u8 *end) {
    String out;
    out.Reserve(i32(end-start));
    u8 *firstOperand = start;
    while (start < end) {
        const u8 b0 = *start;
        if (b0 <= 21) {
            out += DictOperatorResolution(&start, firstOperand);
            out += "\n";
            firstOperand = start;
        } else if (!(b0 == 31 || b0 == 255 || (b0 <= 27 && b0 >= 22))) {
            OperandPassover(&start);
        } else {
            out += "ERROR #" + ToString((u16)b0);
            start++;
        }
    }
    return out;
}

void dict::ParseCharString(u8 *data, u32 size) {
    u8 *firstOperand = data;
    for (u8 *end = data+size; end > data;) {
        const u8 b0 = *data;
        if (b0 <= 21) {
            ResolveOperator(&data, firstOperand);
            firstOperand = data;
        } else if (!(b0 == 31 || b0 == 255 || (b0 <= 27 && b0 >= 22))) {
            OperandPassover(&data);
        } else {
            cout << "ERROR #" << (u16)b0;
            data++;
        }
    }
}

void dict::ResolveOperator(u8 **data, u8 *firstOperand) {
    u8 operator1 = *(*data)++;
#define GETSID(var) (var) = endianFromB(*(SID*)firstOperand); firstOperand += 2
#define GETARR(arr, ophandler)              \
arr.Clear();                                \
for (;*firstOperand != operator1;) {        \
arr.Append(ophandler(&firstOperand));   \
}
#define CASE_F32(op, var) case op : { var = OperandF32(&firstOperand); break; }
#define CASE_I32(op, var) case op : { var = OperandI32(&firstOperand); break; }
#define CASE_BOOL(op, var) case op : { var = *firstOperand != 0; break; }
#define CASE_SID(op, var) case op : { GETSID(var); break; }
#define CASE_ARR_F32(op, var) case op : { GETARR(var, OperandF32); break; }
#define CASE_ARR_I32(op, var) case op : { GETARR(var, OperandI32); break; }
    switch(operator1) {
        case 12: {
            u8 operator2 = *(*data)++;
            switch(operator2) {
                // Private DICT
                CASE_F32(        9, BlueScale);
                CASE_F32(       10, BlueShift);
                CASE_F32(       11, BlueFuzz);
                CASE_ARR_F32(   12, StemSnapH);
                CASE_ARR_F32(   13, StemSnapV);
                CASE_BOOL(      14, ForceBold);
                CASE_I32(       17, LanguageGroup);
                CASE_F32(       18, ExpansionFactor);
                CASE_I32(       19, initialRandomSeed);
                // Top DICT
                CASE_SID(        0, Copyright);
                CASE_BOOL(       1, isFixedPitch);
                CASE_I32(        2, ItalicAngle);
                CASE_I32(        3, UnderlinePosition);
                CASE_I32(        4, UnderlineThickness);
                CASE_I32(        5, PaintType);
                CASE_I32(        6, CharstringType);
                CASE_ARR_F32(    7, FontMatrix);
                CASE_F32(        8, StrokeWidth);
                CASE_I32(       20, SyntheticBase);
                CASE_SID(       21, PostScript);
                CASE_SID(       22, BaseFontName);
                CASE_ARR_I32(   23, BaseFontBlend);
                // CIDFont-only Operators
                case 30: { GETSID(ROS.registry); GETSID(ROS.ordering);
                    ROS.supplement = OperandI32(&firstOperand); break; }
                CASE_F32(       31, CIDFontVersion);
                CASE_F32(       32, CIDFontRevision);
                CASE_I32(       33, CIDFontType);
                CASE_I32(       34, CIDCount);
                CASE_I32(       35, UIDBase);
                CASE_I32(       36, FDArray);
                CASE_I32(       37, FDSelect);
                CASE_SID(       38, FontName);
                default: {
                    cout << "Operator Error 12:" << operator2 << std::endl;
                }
            }
            break;
        }
        // Private DICT
        CASE_ARR_I32(    6, BlueValues);
        CASE_ARR_I32(    7, OtherBlues);
        CASE_ARR_I32(    8, FamilyBlues);
        CASE_ARR_I32(    9, FamilyOtherBlues);
        CASE_F32(       10, StdHW);
        CASE_F32(       11, StdVW);
        CASE_I32(       13, UniqueID);
        CASE_I32(       19, Subrs);
        CASE_I32(       20, defaultWidthX);
        CASE_I32(       21, nominalWidthX);
        // Top DICT
        CASE_SID(        0, version);
        CASE_SID(        1, Notice);
        CASE_SID(        2, FullName);
        CASE_SID(        3, FamilyName);
        CASE_SID(        4, Weight);
        CASE_ARR_I32(    5, FontBBox);
        CASE_ARR_I32(   14, XUID);
        CASE_I32(       15, charset);
        CASE_I32(       16, Encoding);
        CASE_I32(       17, CharStrings);
        case 18: { Private.offset = OperandI32(&firstOperand);
            Private.size = OperandI32(&firstOperand); break; }
        default: {
            cout << "Operator Error " << operator1 << std::endl;
            break;
        }
    }
#undef CASE_F32
#undef CASE_I32
#undef CASE_BOOL
#undef CASE_SID
#undef CASE_ARR_F32
#undef CASE_ARR_I32
#undef GETSID
#undef GETARR
}

} // namespace cffs

// When doing checksums, the data has to still be in big endian or this won't work.
u32 Checksum(u32 *table, const u32 length) {
    u32 sum = 0;
    u32 *end = table + ((length+3) & ~3) / sizeof(u32);
    while (table < end) {
        sum += endianFromB(*table++);
    }
    return sum;
}

void Offset::Read(std::ifstream &file) {
    char buffer[12];
    file.read(buffer, 12);
    sfntVersion.data = *(u32*)buffer;
    numTables = bytesToU16(&buffer[4], SysEndian.little);
    searchRange = bytesToU16(&buffer[6], SysEndian.little);
    entrySelector = bytesToU16(&buffer[8], SysEndian.little);
    rangeShift = bytesToU16(&buffer[10], SysEndian.little);
    tables.Resize(numTables);
    for (u32 i = 0; i < numTables; i++) {
        tables[i].Read(file);
    }
}

bool TTCHeader::Read(std::ifstream &file) {
    ttcTag = readTag(file);
    if (ttcTag == "ttcf"_Tag) {
        {
            char buffer[8];
            file.read(buffer, 8);
            version = bytesToFixed(&buffer[0], SysEndian.little);
            numFonts = bytesToU32(&buffer[4], SysEndian.little);
        }
        offsetTables.Resize(numFonts);
        file.read((char*)offsetTables.data, numFonts * 4);
        if (SysEndian.little) {
            for (u32 i = 0; i < numFonts; i++) {
                offsetTables[i] = endianSwap(offsetTables[i]);
            }
        }
        if (version.major == 2) {
            char buffer[12];
            file.read(buffer, 12);
            dsigTag.data = bytesToU32(&buffer[0], false);
            dsigLength = bytesToU32(&buffer[4], SysEndian.little);
            dsigOffset = bytesToU32(&buffer[8], SysEndian.little);
        } else if (version.major != 1) {
            error = "Unknown TTC file version: " + ToString(version.major) + "." + ToString(version.minor);
            return false;
        }
    } else {
        version.major = 0;
        numFonts = 1;
        offsetTables.Resize(1);
        offsetTables[0] = 0;
    }
    return true;
}

void Record::Read(std::ifstream &file) {
    char buffer[sizeof(Record)];
    file.read(buffer, sizeof(Record));
    tableTag.data = *(u32*)buffer;
    checkSum = bytesToU32(&buffer[4], SysEndian.little);
    offset = bytesToU32(&buffer[8], SysEndian.little);
    length = bytesToU32(&buffer[12], SysEndian.little);
}

#define ENDIAN_SWAP(in) in = endianSwap((in))
#define ENDIAN_SWAP_FIXED(in) in.major = endianSwap(in.major); in.minor = endianSwap(in.minor)

void cmap_encoding::EndianSwap() {
    ENDIAN_SWAP(platformID);
    ENDIAN_SWAP(platformSpecificID);
    ENDIAN_SWAP(offset);
}

void cmap_index::EndianSwap() {
    ENDIAN_SWAP(version);
    ENDIAN_SWAP(numberSubtables);
}

bool cmap_format_any::EndianSwap() {
    ENDIAN_SWAP(format);
    if (format == 0) {
        ((cmap_format0*)this)->EndianSwap();
    } else if (format == 4) {
        ((cmap_format4*)this)->EndianSwap();
    } else if (format == 12) {
        ((cmap_format12*)this)->EndianSwap();
    } else {
        error = "cmap format " + ToString(format) + " is not supported.";
        return false;
    }
    return true;
}

u32 cmap_format_any::GetGlyphIndex(char32 glyph) {
    if (format == 0) {
        return ((cmap_format0*)this)->GetGlyphIndex(glyph);
    } else if (format == 4) {
        return ((cmap_format4*)this)->GetGlyphIndex(glyph);
    } else if (format == 12) {
        return ((cmap_format12*)this)->GetGlyphIndex(glyph);
    }
    return 0;
}

void cmap_format0::EndianSwap() {
    // format is already done.
    // NOTE: In this case it may be ideal to just ignore these values entirely
    //       but I'd rather be a completionist first and remove stuff later.
    ENDIAN_SWAP(length);
    ENDIAN_SWAP(language);
}

u32 cmap_format0::GetGlyphIndex(char32 glyph) {
    if (glyph >= 256) {
        return 0;
    }
    return (u32)glyphIndexArray[glyph];
}

void cmap_format4::EndianSwap() {
    // format is already done.
    ENDIAN_SWAP(length);
    ENDIAN_SWAP(language);
    ENDIAN_SWAP(segCountX2);
    ENDIAN_SWAP(searchRange);
    ENDIAN_SWAP(entrySelector);
    ENDIAN_SWAP(rangeShift);
    const u16 segCount = segCountX2 / 2;
    u16 *ptr = (u16*)(((char*)this) + sizeof(cmap_format4));
    // 4 arrays of size segCount and 1 scalar
    for (u16 i = 0; i < segCount*4+1; i++) {
        ENDIAN_SWAP(*ptr);
        ptr++;
    }
}

u32 cmap_format4::GetGlyphIndex(char32 glyph) {
    u32 segment = 0;
    for (u32 i = 0; i*2 < segCountX2; i++) {
        if (endCode(i) >= glyph) { // Find the first endCode >= glyph
            if (startCode(i) <= glyph) { // We're in the range
                segment = i;
                break;
            } else { // We're not in the range
                return 0;
            }
        }
    }
    // If we got this far, we have a mapped segment.
    if (idRangeOffset(segment) == 0) {
        // cout << "idDelta(" << segment << ") = " << idDelta(segment) << std::endl;
        return ((u32)idDelta(segment) + glyph) % 65536;
    } else {
        // idRangeOffset is in bytes, and we're working with an array of u16's, so divide offset by two.
        u32 glyphIndex = endianFromB( // We keep the glyphIndexArray in big-endian
            *(&idRangeOffset(segment) + idRangeOffset(segment)/2 + (glyph - startCode(segment)))
        );
        if (glyphIndex == 0) {
            return 0;
        } else {
            return (glyphIndex + (u32)idDelta(segment)) % 65536;
        }
    }
}

void cmap_format12_group::EndianSwap() {
    ENDIAN_SWAP(startCharCode);
    ENDIAN_SWAP(endCharCode);
    ENDIAN_SWAP(startGlyphCode);
}

void cmap_format12::EndianSwap() {
    // format.major is already done.
    ENDIAN_SWAP(format.minor);
    ENDIAN_SWAP(length);
    ENDIAN_SWAP(language);
    ENDIAN_SWAP(nGroups);
    cmap_format12_group *ptr = (cmap_format12_group*)(((char*)this) + sizeof(cmap_format12));
    for (u32 i = 0; i < nGroups; i++) {
        ptr->EndianSwap();
        ptr++;
    }
}

u32 cmap_format12::GetGlyphIndex(char32 glyph) {
    cmap_format12_group *group = &groups(0);
    for (u32 i = 0; i < nGroups; i++) {
        if (group->endCharCode >= glyph) {
            if (group->startCharCode <= glyph) {
                return group->startGlyphCode + (glyph - group->startCharCode);
            } else {
                return 0;
            }
        }
        group++;
    }
    return 0;
}

void head::EndianSwap() {
    ENDIAN_SWAP_FIXED(version);
    ENDIAN_SWAP_FIXED(fontRevision);
    ENDIAN_SWAP(checkSumAdjustment);
    ENDIAN_SWAP(magicNumber);
    ENDIAN_SWAP(flags);
    ENDIAN_SWAP(unitsPerEm);
    ENDIAN_SWAP(created);
    ENDIAN_SWAP(modified);
    ENDIAN_SWAP(xMin);
    ENDIAN_SWAP(yMin);
    ENDIAN_SWAP(xMax);
    ENDIAN_SWAP(yMax);
    ENDIAN_SWAP(macStyle);
    ENDIAN_SWAP(lowestRecPPEM);
    ENDIAN_SWAP(fontDirectionHint);
    ENDIAN_SWAP(indexToLocFormat);
    ENDIAN_SWAP(glyphDataFormat);
}

void maxp::EndianSwap() {
    ENDIAN_SWAP_FIXED(version);
    ENDIAN_SWAP(numGlyphs);
    // Version 0.5 only needs the above
    if (version.major == 1 && version.minor == 0) {
        ENDIAN_SWAP(maxPoints);
        ENDIAN_SWAP(maxContours);
        ENDIAN_SWAP(maxCompositePoints);
        ENDIAN_SWAP(maxCompositeContours);
        ENDIAN_SWAP(maxZones);
        ENDIAN_SWAP(maxTwilightPoints);
        ENDIAN_SWAP(maxStorage);
        ENDIAN_SWAP(maxFunctionDefs);
        ENDIAN_SWAP(maxInstructionDefs);
        ENDIAN_SWAP(maxStackElements);
        ENDIAN_SWAP(maxSizeOfInstructions);
        ENDIAN_SWAP(maxComponentElements);
        ENDIAN_SWAP(maxComponentDepth);
    }
}

void loca::EndianSwap(u16 numGlyphs, bool longOffsets) {
    if (longOffsets) {
        for (u16 i = 0; i <= numGlyphs; i++) {
            u32& offset = offsets32(i);
            ENDIAN_SWAP(offset);
        }
    } else {
        for (u16 i = 0; i <= numGlyphs; i++) {
            u16& offset = offsets16(i);
            ENDIAN_SWAP(offset);
        }
    }
}

void glyf_header::EndianSwap() {
    ENDIAN_SWAP(numberOfContours);
    ENDIAN_SWAP(xMin);
    ENDIAN_SWAP(yMin);
    ENDIAN_SWAP(xMax);
    ENDIAN_SWAP(yMax);
}

void glyf::EndianSwap(loca *loc, u16 numGlyphs, bool longOffsets) {
    #define DO_SWAP()   header->EndianSwap();                   \
                        if (header->numberOfContours >= 0) {    \
                            EndianSwapSimple(header);           \
                        } else {                                \
                            EndianSwapCompound(header);         \
                        }
    if (longOffsets) {
        Set<u32> offsetsDone;
        for (u16 i = 0; i < numGlyphs; i++) {
            if (offsetsDone.count(loc->offsets32(i)) == 0) {
                glyf_header *header = (glyf_header*)((char*)this + loc->offsets32(i));
                DO_SWAP();
                offsetsDone.insert(loc->offsets32(i));
            }
        }
    } else {
        Set<u16> offsetsDone;
        for (u16 i = 0; i < numGlyphs; i++) {
            if (offsetsDone.count(loc->offsets16(i)) == 0) {
                glyf_header *header = (glyf_header*)(((char*)this) + loc->offsets16(i) * 2);
                DO_SWAP();
                offsetsDone.insert(loc->offsets16(i));
            }
        }
    }
    #undef DO_SWAP
}

void glyf::EndianSwapSimple(glyf_header *header) {
    char *ptr = (char*)(header+1);
    u16* endPtsOfContours = (u16*)ptr;
    for (i16 i = 0; i < header->numberOfContours; i++) {
        ENDIAN_SWAP(*(u16*)ptr);
        ptr += 2;
    }
    u16& instructionLength = ENDIAN_SWAP(*(u16*)ptr);
    ptr += instructionLength + 2;
    // Now it gets weird
    u16 nPoints;
    if (header->numberOfContours > 0) {
        nPoints = endPtsOfContours[header->numberOfContours-1] + 1;
    } else {
        nPoints = 0;
    }
    // Now figure out how many flag bytes there actually are
    u8 *flags = (u8*)ptr;
    u16 nFlags = nPoints;
    for (u16 i = 0; i < nFlags; i++) {
        if (flags[i] & 0x08) { // Bit 3
            nFlags -= flags[++i] - 1;
        }
    }
    ptr += nFlags; // We should be at the beginning of the xCoord array
    // We only need to swap endians for xCoords that are 2-bytes in width
    u16 repeatCount = 0;
    for (u16 i = 0; i < nFlags; i++) {
        if (flags[i] & 0x02) {
            // cout << "xCoord is u8" << std::endl;
            ptr++;
        } else {
            if (!(flags[i] & 0x10)) {
                // cout << "xCoord is u16" << std::endl;
                ENDIAN_SWAP(*(i16*)ptr);
                ptr += 2;
            } else {
                // cout << "xCoord is repeated" << std::endl;
            }
        }
        if (repeatCount) {
            repeatCount--;
            if (repeatCount) {
                i--; // Stay on same flag
            } else {
                i++; // Skip byte that held repeatCount
            }
        } else if (flags[i] & 0x08 && i+1 < nFlags) {
            repeatCount = (u16)flags[i+1];
            // cout << "Repeat " << repeatCount << " times" << std::endl;
            if (repeatCount != 0) {
                i--; // Stay on same flag
            } else {
                i++;
            }
        }
    }
    // Same thing for yCoord array
    for (u16 i = 0; i < nFlags; i++) {
        if (flags[i] & 0x04) {
            // cout << "yCoord is u8" << std::endl;
            ptr++;
        } else {
            if (!(flags[i] & 0x20)) {
                // cout << "yCoord is u16" << std::endl;
                ENDIAN_SWAP(*(i16*)ptr);
                ptr += 2;
            } else {
                // cout << "yCoord is repeated" << std::endl;
            }
        }
        if (repeatCount) {
            repeatCount--;
            if (repeatCount) {
                i--; // Stay on same flag
            } else {
                i++; // Skip byte that held repeatCount
            }
        } else if (flags[i] & 0x08 && i+1 < nFlags) {
            repeatCount = (u16)flags[i+1];
            // cout << "Repeat " << repeatCount << " times" << std::endl;
            if (repeatCount != 0) {
                i--; // Stay on same flag
            } else {
                i++;
            }
        }
    }
    // Simple my ASS
}

void glyf::EndianSwapCompound(glyf_header *header) {
    char *ptr = (char*)(header+1);
    u16 *flags;
    u16 components = 0;
    do {
        components++;
        flags = (u16*)ptr;
        ENDIAN_SWAP(*flags);
        ptr += 2;
        u16& glyphIndex = *(u16*)ptr;
        ENDIAN_SWAP(glyphIndex);
        ptr += 2;
        if (*flags & 0x01) { // Bit 0
            u16 *arguments = (u16*)ptr;
            ENDIAN_SWAP(arguments[0]);
            ENDIAN_SWAP(arguments[1]);
            ptr += 4;
        } else {
            ptr += 2;
        }
        if (*flags & 0x08) { // Bit 3
            F2Dot14_t& scale = *(F2Dot14_t*)ptr;
            ENDIAN_SWAP(scale);
            ptr += 2;
        }
        if (*flags & 0x40) { // Bit 6
            F2Dot14_t *scale = (F2Dot14_t*)ptr;
            ENDIAN_SWAP(scale[0]);
            ENDIAN_SWAP(scale[1]);
            ptr += 4;
        }
        if (*flags & 0x80) { // Bit 7
            F2Dot14_t *scale = (F2Dot14_t*)ptr;
            ENDIAN_SWAP(scale[0]);
            ENDIAN_SWAP(scale[1]);
            ENDIAN_SWAP(scale[2]);
            ENDIAN_SWAP(scale[3]);
            ptr += 8;
        }
    } while (*flags & 0x20); // Bit 5
    if (*flags & 0x0100) { // Bit 8
        u16& numInstructions = *(u16*)ptr;
        ENDIAN_SWAP(numInstructions);
        ptr += 2 + numInstructions;
    }
    // That was somehow simpler than the "Simple" Glyph... tsk
}

bool cffs::charset_format_any::EndianSwap(Card16 nGlyphs) {
    if (format == 0) {
        ((cffs::charset_format0*)this)->EndianSwap(nGlyphs);
    } else if (format == 1) {
        ((cffs::charset_format1*)this)->EndianSwap(nGlyphs);
    } else if (format == 2) {
        ((cffs::charset_format2*)this)->EndianSwap(nGlyphs);
    } else {
        error = "Unsupported charset format " + ToString((u16)format);
        return false;
    }
    return true;
}

void cffs::charset_format0::EndianSwap(Card16 nGlyphs) {
    SID *glyph = (SID*)(&format + 1);
    for (Card16 i = 0; i < nGlyphs-1; i++) {
        ENDIAN_SWAP(*glyph);
        glyph++;
    }
}

void cffs::charset_range1::EndianSwap() {
    ENDIAN_SWAP(first);
}

void cffs::charset_range2::EndianSwap() {
    ENDIAN_SWAP(first);
    ENDIAN_SWAP(nLeft);
#ifdef LOG_VERBOSE
    cout << "charset_range2: first = " << (u32)first << ", nLeft = " << nLeft << std::endl;
#endif
}

void cffs::charset_format1::EndianSwap(Card16 nGlyphs) {
    i32 glyphsRemaining = nGlyphs-1;
    cffs::charset_range1 *range = (cffs::charset_range1*)(&format + 1);
    while (glyphsRemaining > 0) {
        range->EndianSwap();
        glyphsRemaining -= range->nLeft+1;
        range++;
    }
}

void cffs::charset_format2::EndianSwap(Card16 nGlyphs) {
    i32 glyphsRemaining = nGlyphs-1;
    cffs::charset_range2 *range = (cffs::charset_range2*)(&format + 1);
    while (glyphsRemaining > 0) {
        range->EndianSwap();
        glyphsRemaining -= range->nLeft+1;
        range++;
    }
}

bool cffs::FDSelect_any::EndianSwap() {
    if (format == 0) {
#ifdef LOG_VERBOSE
        cout << "Format 0" << std::endl;
#endif
    } else if (format == 3) {
#ifdef LOG_VERBOSE
        cout << "Format 3" << std::endl;
#endif
        ((cffs::FDSelect_format3*)this)->EndianSwap();
    } else {
        error = "Unsupported FDSelect format " + ToString((u16)format);
        return false;
    }
    return true;
}

void cffs::FDSelect_format3::EndianSwap() {
    ENDIAN_SWAP(nRanges);
#ifdef LOG_VERBOSE
    cout << "nRanges = " << nRanges << std::endl;
#endif
    cffs::FDSelect_range3 *range = (cffs::FDSelect_range3*)(&format + 3);
    for (u32 i = 0; i < (u32)nRanges; i++) {
        ENDIAN_SWAP(range->first);
        // cout << "Range[" << i << "].first = " << range->first << ", fd = " << (u32)range->fd << std::endl;
        range++;
    }
    ENDIAN_SWAP(range->first);
    // cout << "Sentinel = " << range->first << std::endl;
}

bool cffs::index::Parse(char **ptr, u8 **dataStart, Array<u32> *dstOffsets, bool swapEndian) {
    if (swapEndian) {
        ENDIAN_SWAP(count);
    }
    *ptr += 2;
    u32 lastOffset = 1;
    Array<u32> offsets;
    if (count != 0) {
        offsets.Resize(count+1);
#ifdef LOG_VERBOSE
        cout << "count = " << count << ", offSize = " << (u32)offSize << std::endl;
#endif
        (*ptr)++; // Getting over offSize
        for (u32 i = 0; i < (u32)count+1; i++) {
            u32 offset;
            if (offSize == 1) {
                offset = *((u8*)(*ptr)++);
            } else if (offSize == 2) {
                cffs::Offset16 *off = (cffs::Offset16*)(u8*)*ptr;
                if (swapEndian) {
                    ENDIAN_SWAP(*off);
                }
                offset = *off;
                *ptr += 2;
            } else if (offSize == 3) {
                cffs::Offset24 *off = (cffs::Offset24*)(u8*)*ptr;
                offset = off->value();
                *ptr += 3;
            } else if (offSize == 4) {
                cffs::Offset32 *off = (cffs::Offset32*)(u8*)*ptr;
                if (swapEndian) {
                    ENDIAN_SWAP(*off);
                }
                offset = *off;
                *ptr += 4;
            } else {
                error = "Unsupported offSize: " + ToString((u32)offSize);
                return false;
            }
            if (i == count) {
                lastOffset = offset;
            }
            offsets[i] = offset;
        }
    }
    *dataStart = (u8*)(*ptr - 1);
    *ptr += lastOffset - 1;
    *dstOffsets = std::move(offsets);
    return true;
}

bool cff::Parse(cffParsed *parsed, bool swapEndian) {
    parsed->active = true;
    char *ptr = (char*)this + header.size;

    //
    //                  nameIndex
    //

    parsed->nameIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
    cout << "nameIndex:\n";
#endif
    if (!parsed->nameIndex->Parse(&ptr, &parsed->nameIndexData, &parsed->nameIndexOffsets, swapEndian)) {
        error = "nameIndex: " + error;
        return false;
    }
#ifdef LOG_VERBOSE
    cout << "nameIndex data:\n";
    for (i32 i = 0; i < parsed->nameIndexOffsets.size-1; i++) {
        String string(parsed->nameIndexOffsets[i+1] - parsed->nameIndexOffsets[i]);
        memcpy(string.data, parsed->nameIndexData + parsed->nameIndexOffsets[i], string.size);
        cout << "[" << i << "]=\"" << string << "\" ";
    }
    cout << std::endl;
#endif
    if (parsed->nameIndexOffsets.size > 2) {
        error = "We only support CFF tables with 1 Name entry (1 font).";
        return false;
    }

    //
    //                  dictIndex
    //

    parsed->dictIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
    cout << "dictIndex:\n";
#endif
    if (!parsed->dictIndex->Parse(&ptr, &parsed->dictIndexData, &parsed->dictIndexOffsets, swapEndian)) {
        error = "dictIndex: " + error;
        return false;
    }
#ifdef LOG_VERBOSE
    cout << "dictIndex charstrings:\n" << cffs::CharString(
        parsed->dictIndexData + parsed->dictIndexOffsets[0],
        parsed->dictIndexData + parsed->dictIndexOffsets[parsed->dictIndexOffsets.size-1]
    ) << std::endl;
#endif
    parsed->dictIndexValues.ParseCharString(
        parsed->dictIndexData + parsed->dictIndexOffsets[0],
        parsed->dictIndexOffsets[1] - parsed->dictIndexOffsets[0]
    );

    if (parsed->dictIndexValues.CharstringType != 2) {
        error = "Unsupported CharstringType " + ToString((u16)parsed->dictIndexValues.CharstringType);
        return false;
    }

    //
    //                  stringsIndex
    //

    parsed->stringsIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
    cout << "stringsIndex:\n";
#endif
    if (!parsed->stringsIndex->Parse(&ptr, &parsed->stringsIndexData,
                                     &parsed->stringsIndexOffsets, swapEndian)) {
        error = "stringsIndex: " + error;
        return false;
    }
#ifdef LOG_VERBOSE
    cout << "stringsIndex data:\n";
    for (i32 i = 0; i < parsed->stringsIndexOffsets.size-1; i++) {
        String string(parsed->stringsIndexOffsets[i+1] - parsed->stringsIndexOffsets[i]);
        memcpy(string.data, parsed->stringsIndexData + parsed->stringsIndexOffsets[i], string.size);
        cout << "\n[" << i << "]=\"" << string << "\" ";
    }
    cout << std::endl;
#endif

    //
    //                  gsubrIndex
    //

    parsed->gsubrIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
    cout << "gsubrIndex:\n";
#endif
    if (!parsed->gsubrIndex->Parse(&ptr, &parsed->gsubrIndexData, &parsed->gsubrIndexOffsets, swapEndian)) {
        error = "gsubrIndex: " + error;
        return false;
    }
    // cout << "gsubrIndex data dump:\n" << std::hex;
    // for (i32 i = 0; i < gsubrIndexOffsets.size-1; i++) {
    //     String string(gsubrIndexOffsets[i+1] - gsubrIndexOffsets[i]);
    //     memcpy(string.data, gsubrIndexData + gsubrIndexOffsets[i], string.size);
    //     cout << "\n[" << i << "]=\"";
    //     for (i32 i = 0; i < string.size; i++) {
    //         cout << "0x" << (u32)(u8)string[i];
    //         if (i < string.size-1) {
    //             cout << ", ";
    //         }
    //     }
    //     cout << "\" ";
    // }
    // cout << std::endl;

    //
    //                  charStringsIndex
    //

    if (parsed->dictIndexValues.CharStrings == -1) {
        error = "CFF data has no CharStrings offset!";
        return false;
    }

#ifdef LOG_VERBOSE
    cout << "charStringsIndex:\n";
#endif
    ptr = (char*)this + parsed->dictIndexValues.CharStrings;
    parsed->charStringsIndex = (cffs::index*)ptr;
    if (!parsed->charStringsIndex->Parse(&ptr, &parsed->charStringsIndexData,
                                         &parsed->charStringsIndexOffsets, swapEndian)) {
        error = "charStringsIndex: " + error;
        return false;
    }

    //
    //                  charsets
    //

    // Do we have a predefined charset or a custom one?
    if (parsed->dictIndexValues.charset == 0) {
        // ISOAdobe charset
#ifdef LOG_VERBOSE
        cout << "We are using the ISOAdobe predefined charset." << std::endl;
#endif
    } else if (parsed->dictIndexValues.charset == 1) {
        // Expert charset
#ifdef LOG_VERBOSE
        cout << "We are using the Expert predefined charset." << std::endl;
#endif
    } else if (parsed->dictIndexValues.charset == 2) {
        // ExpertSubset charset
#ifdef LOG_VERBOSE
        cout << "We are using the ExpertSubset predefined charset." << std::endl;
#endif
    } else {
        // Custom charset
        cffs::charset_format_any *charset = (cffs::charset_format_any*)((char*)this + parsed->dictIndexValues.charset);
#ifdef LOG_VERBOSE
        cout << "We are using a custom charset with format " << (i32)charset->format << std::endl;
#endif
        if (swapEndian) {
            charset->EndianSwap(parsed->charStringsIndex->count);
        }
    }

    //
    //                  CIDFont
    //

    if (parsed->dictIndexValues.FDSelect != -1) {
        parsed->CIDFont = true;
        if (parsed->dictIndexValues.FDArray == -1) {
            error = "CIDFonts must have an FDArray!";
            return false;
        }
#ifdef LOG_VERBOSE
        cout << "FDSelect:\n";
#endif
        parsed->fdSelect = (cffs::FDSelect_any*)((char*)this + parsed->dictIndexValues.FDSelect);
        if (swapEndian) {
            parsed->fdSelect->EndianSwap();
        }

#ifdef LOG_VERBOSE
        cout << "FDArray:\n";
#endif
        ptr = (char*)this + parsed->dictIndexValues.FDArray;
        parsed->fdArray = (cffs::index*)ptr;
        if (!parsed->fdArray->Parse(&ptr, &parsed->fdArrayData, &parsed->fdArrayOffsets, swapEndian)) {
            error = "FDArray: " + error;
            return false;
        }
#ifdef LOG_VERBOSE
        for (i32 i = 0; i < parsed->fdArrayOffsets.size-1; i++) {
            cout << "fontDictIndex[" << i << "] charstrings: " << cffs::CharString(
                parsed->fdArrayData + parsed->fdArrayOffsets[i],
                parsed->fdArrayData + parsed->fdArrayOffsets[i+1]
            ) << std::endl;
        }
#endif
    }

    // My, that was quite a lot of stuff... mostly logging ain't it?
    return true;
}

void hhea::EndianSwap() {
    ENDIAN_SWAP_FIXED(version);
    ENDIAN_SWAP(ascent);
    ENDIAN_SWAP(descent);
    ENDIAN_SWAP(lineGap);
    ENDIAN_SWAP(advanceWidthMax);
    ENDIAN_SWAP(minLeftSideBearing);
    ENDIAN_SWAP(minRightSideBearing);
    ENDIAN_SWAP(xMaxExtent);
    ENDIAN_SWAP(caretSlopeRise);
    ENDIAN_SWAP(caretSlopeRun);
    ENDIAN_SWAP(caretOffset);
    // Who cares about reserved garbage anyway???
    ENDIAN_SWAP(metricDataFormat);
    ENDIAN_SWAP(numOfLongHorMetrics);
#ifdef LOG_VERBOSE
    cout << "numOfLongHorMetrics = " << numOfLongHorMetrics << std::endl;
#endif
}

void hmtx::EndianSwap(u16 numOfLongHorMetrics, u16 numGlyphs) {
    longHorMetric *metrics = (longHorMetric*)this;
    for (u32 i = 0; i < numOfLongHorMetrics; i++) {
        ENDIAN_SWAP(metrics->advanceWidth);
        ENDIAN_SWAP(metrics->leftSideBearing);
        metrics++;
    }
    FWord_t *leftSideBearing = (FWord_t*)metrics;
    for (u32 i = numOfLongHorMetrics; i < numGlyphs; i++) {
        ENDIAN_SWAP(*leftSideBearing);
        leftSideBearing++;
    }
}

#undef ENDIAN_SWAP
#undef ENDIAN_SWAP_FIXED

Glyph glyfParsed::GetGlyph(u32 glyphIndex) const {
    Glyph out;
    glyf_header *gheader = (glyf_header*)((char*)glyphData + glyfOffsets[glyphIndex]);
    if (gheader->numberOfContours >= 0) {
        out = ParseSimple(gheader);
    } else {
        out = ParseCompound(gheader);
    }
    vec2 minBounds(1000.0), maxBounds(-1000.0);
    for (Curve& curve : out.curves) {
        if (curve.p1.x < minBounds.x) { minBounds.x = curve.p1.x; }
        if (curve.p2.x < minBounds.x) { minBounds.x = curve.p2.x; }
        if (curve.p3.x < minBounds.x) { minBounds.x = curve.p3.x; }
        if (curve.p1.y < minBounds.y) { minBounds.y = curve.p1.y; }
        if (curve.p2.y < minBounds.y) { minBounds.y = curve.p2.y; }
        if (curve.p3.y < minBounds.y) { minBounds.y = curve.p3.y; }
        if (curve.p1.x > maxBounds.x) { maxBounds.x = curve.p1.x; }
        if (curve.p2.x > maxBounds.x) { maxBounds.x = curve.p2.x; }
        if (curve.p3.x > maxBounds.x) { maxBounds.x = curve.p3.x; }
        if (curve.p1.y > maxBounds.y) { maxBounds.y = curve.p1.y; }
        if (curve.p2.y > maxBounds.y) { maxBounds.y = curve.p2.y; }
        if (curve.p3.y > maxBounds.y) { maxBounds.y = curve.p3.y; }
    }
    for (Line& line : out.lines) {
        if (line.p1.x < minBounds.x) { minBounds.x = line.p1.x; }
        if (line.p2.x < minBounds.x) { minBounds.x = line.p2.x; }
        if (line.p1.y < minBounds.y) { minBounds.y = line.p1.y; }
        if (line.p2.y < minBounds.y) { minBounds.y = line.p2.y; }
        if (line.p1.x > maxBounds.x) { maxBounds.x = line.p1.x; }
        if (line.p2.x > maxBounds.x) { maxBounds.x = line.p2.x; }
        if (line.p1.y > maxBounds.y) { maxBounds.y = line.p1.y; }
        if (line.p2.y > maxBounds.y) { maxBounds.y = line.p2.y; }
    }
    for (Curve& curve : out.curves) {
        curve.p1 -= minBounds;
        curve.p2 -= minBounds;
        curve.p3 -= minBounds;
    }
    for (Line& line : out.lines) {
        line.p1 -= minBounds;
        line.p2 -= minBounds;
    }
    out.info.size = maxBounds - minBounds;
    out.info.offset += minBounds;
    longHorMetric metric = horMetrics->Metric(glyphIndex, horHeader->numOfLongHorMetrics);
    f32 lsb = (f32)metric.leftSideBearing / (f32)header->unitsPerEm;
    out.info.offset.x -= lsb;
    out.info.advance.x = (f32)metric.advanceWidth / (f32)header->unitsPerEm;
    out.info.advance.y = 0.0;
    // cout << "Advance width: " << out.info.advance.x << std::endl;
    // cout << "offset = { " << out.offset.x << ", " << out.offset.y
    //      << " }, size = {" << out.size.x << ", " << out.size.y << " }" << std::endl;
    return out;
}

GlyphInfo glyfParsed::GetGlyphInfo(u32 glyphIndex) const {
    return GetGlyph(glyphIndex).info;
}

Glyph glyfParsed::ParseSimple(glyf_header *gheader, Array<glyfPoint> *dstArray) const {
    Glyph out;

    // cout << "xMin = " << xMin << ", xMax = " << xMax << ", yMin = " << yMin << ", yMax = " << yMax << std::endl;

    char *ptr = (char*)(gheader+1);
    u16* endPtsOfContours = (u16*)ptr;
    ptr += 2 * gheader->numberOfContours;
    u16& instructionLength = *(u16*)ptr;
    // cout << "instructionLength = " << instructionLength << std::endl;
    ptr += instructionLength + 2;
    // Now it gets weird
    u16 nPoints;
    if (gheader->numberOfContours > 0) {
        nPoints = endPtsOfContours[gheader->numberOfContours-1] + 1;
    } else {
        nPoints = 0;
    }
    // Now figure out how many flag bytes there actually are
    u8 *flags = (u8*)ptr;
    u16 nFlags = nPoints;
    for (u16 i = 0; i < nFlags; i++) {
        if (flags[i] & 0x08) { // Bit 3
            nFlags -= flags[++i] - 1;
        }
    }
    ptr += nFlags; // We should be at the beginning of the xCoord array
    // cout << "nPoints = " << nPoints << ", nFlags = " << nFlags << std::endl;
    Array<glyfPoint> points(nPoints);
    u16 repeatCount = 0;
    i32 pointIndex = 0;
    i32 prevX = 0, prevY = 0;
    for (u16 i = 0; i < nFlags; i++) {
        if (flags[i] & 0x02) {
            i32 coord = *(u8*)ptr;
            if (!(flags[i] & 0x10)) {
                coord *= -1;
            }
            // cout << "short xCoord " << coord;
            coord += prevX;
            // cout << ", rel = " << coord << std::endl;
            prevX = coord;
            points[pointIndex].coords.x = (f32)coord;
            ptr++;
        } else {
            if (!(flags[i] & 0x10)) {
                i32 coord = *(i16*)ptr;
                // cout << "long xCoord " << coord;
                coord += prevX;
                // cout << ", rel = " << coord << std::endl;
                prevX = coord;
                points[pointIndex].coords.x = (f32)coord;
                ptr += 2;
            } else {
                points[pointIndex].coords.x = (f32)prevX;
                // cout << "xCoord repeats, rel = " << prevX << std::endl;
                // repeat
            }
        }
        points[pointIndex].coords.x /= (f32)header->unitsPerEm;
        points[pointIndex].onCurve = ((flags[i] & 0x01) == 0x01);
        if (repeatCount) {
            repeatCount--;
            if (repeatCount) {
                i--; // Stay on same flag
            } else {
                i++; // Skip byte that held repeatCount
            }
        } else if (flags[i] & 0x08 && i+1 < nFlags) {
            repeatCount = (u16)flags[i+1];
            if (repeatCount != 0) {
                i--; // Stay on same flag
            } else {
                i++;
            }
        }
        pointIndex++;
    }
    // Same thing for yCoord array
    pointIndex = 0;
    for (u16 i = 0; i < nFlags; i++) {
        if (flags[i] & 0x04) {
            i32 coord = *(u8*)ptr;
            if (!(flags[i] & 0x20)) {
                coord *= -1;
            }
            // cout << "short yCoord " << coord;
            coord += prevY;
            // cout << ", rel = " << coord << std::endl;
            prevY = coord;
            points[pointIndex].coords.y = (f32)coord;
            ptr++;
        } else {
            if (!(flags[i] & 0x20)) {
                i32 coord = *(i16*)ptr;
                // cout << "long yCoord " << coord;
                coord += prevY;
                // cout << ", rel = " << coord << std::endl;
                prevY = coord;
                points[pointIndex].coords.y = (f32)coord;
                ptr += 2;
            } else {
                points[pointIndex].coords.y = (f32)prevY;
                // cout << "yCoord repeats, rel = " << prevY << std::endl;
                // repeat
            }
        }
        points[pointIndex].coords.y /= (f32)header->unitsPerEm;
        points[pointIndex].onCurve = ((flags[i] & 0x01) == 0x01);
        if (repeatCount) {
            repeatCount--;
            if (repeatCount) {
                i--; // Stay on same flag
            } else {
                i++; // Skip byte that held repeatCount
            }
        } else if (flags[i] & 0x08 && i+1 < nFlags) {
            repeatCount = (u16)flags[i+1];
            if (repeatCount != 0) {
                i--; // Stay on same flag
            } else {
                i++;
            }
        }
        pointIndex++;
    }

    // Now we expand our points to always include control points and convert to normalized coordinates.
    glyfPoint *pt = points.data;
    out.AddFromGlyfPoints(pt, endPtsOfContours[0] + 1);
    pt += endPtsOfContours[0] + 1;
    for (i32 i = 1; i < gheader->numberOfContours; i++) {
        out.AddFromGlyfPoints(pt, endPtsOfContours[i] - endPtsOfContours[i-1]);
        pt += endPtsOfContours[i] - endPtsOfContours[i-1];
    }
    if (dstArray != nullptr) {
        *dstArray = std::move(points);
    }
    return out;
}

Glyph glyfParsed::ParseCompound(glyf_header *gheader, Array<glyfPoint> *dstArray) const {
    Glyph out;
    char *ptr = (char*)(gheader+1);
    u16 *flags;
    struct ComponentParse {
        u16 glyphIndex;
        i32 arguments[2];
        bool argsAreXY;
        bool roundXY;
        bool useMyMetrics;
        bool scaledComponentOffset;
        mat2 scale;
    };
    Array<ComponentParse> componentsParse;
    do {
        ComponentParse componentParse;
        componentParse.scale = mat2(1.0);
        flags = (u16*)ptr;
        ptr += 2;
        componentParse.glyphIndex = *(u16*)ptr;
        ptr += 2;
        componentParse.argsAreXY = (*flags & 0x02) != 0;
        if (*flags & 0x01) { // Bit 0
            if (componentParse.argsAreXY) {
                i16 *arguments = (i16*)ptr;
                componentParse.arguments[0] = arguments[0];
                componentParse.arguments[1] = arguments[1];
            } else {
                u16 *arguments = (u16*)ptr;
                componentParse.arguments[0] = arguments[0];
                componentParse.arguments[1] = arguments[1];
            }
            ptr += 4;
        } else {
            if (componentParse.argsAreXY) {
                i8 *arguments = (i8*)ptr;
                componentParse.arguments[0] = (i32)arguments[0];
                componentParse.arguments[1] = (i32)arguments[1];
            } else {
                u8 *arguments = (u8*)ptr;
                componentParse.arguments[0] = (i32)arguments[0];
                componentParse.arguments[1] = (i32)arguments[1];
            }
            ptr += 2;
        }
        componentParse.roundXY = (*flags & 0x03) != 0;
        if (*flags & 0x08) { // Bit 3
            const F2Dot14_t& scale = *(F2Dot14_t*)ptr;
            componentParse.scale = mat2(ToF32(scale));
            ptr += 2;
        }
        if (*flags & 0x40) { // Bit 6
            const F2Dot14_t *scale = (F2Dot14_t*)ptr;
            componentParse.scale = mat2(ToF32(scale[0]), 0.0, 0.0, ToF32(scale[1]));
            ptr += 4;
        }
        if (*flags & 0x80) { // Bit 7
            const F2Dot14_t *scale = (F2Dot14_t*)ptr;
            componentParse.scale = mat2(ToF32(scale[0]), ToF32(scale[1]), ToF32(scale[2]), ToF32(scale[3]));
            ptr += 8;
        }
        componentParse.useMyMetrics = (*flags & 0x200) != 0; // Bit 9
        componentParse.scaledComponentOffset = (*flags & 0x800) != 0; // Bit 11
        componentsParse.Append(componentParse);
    } while (*flags & 0x20); // Bit 5
    // if (*flags & 0x0100) { // Bit 8
    //     u16& numInstructions = *(u16*)ptr;
    //     ptr += 2 + numInstructions;
    // }
    const f32 unitsPerEm = (f32)header->unitsPerEm;
    Array<glyfPoint> allPoints;
    for (ComponentParse& componentParse : componentsParse) {
        Array<glyfPoint> componentPoints;
        glyf_header *componentHeader = (glyf_header*)((char*)glyphData + glyfOffsets[componentParse.glyphIndex]);
        Glyph componentGlyph;
        bool simple = false;
        Component component;
        if (componentHeader->numberOfContours <= 0) {
            componentGlyph = ParseCompound(componentHeader, &componentPoints);
        } else {
            componentGlyph = ParseSimple(componentHeader, &componentPoints);
            simple = true;
            component.glyphIndex = componentParse.glyphIndex;
        }
        vec2 offset;
        if (componentParse.argsAreXY) {
            // cout << "argsAreXY" << std::endl;
            offset = vec2((f32)componentParse.arguments[0], (f32)componentParse.arguments[1]) / unitsPerEm;
        } else {
            // cout << "argsAre not XY" << std::endl;
            offset = componentPoints[componentParse.arguments[1]].coords - allPoints[componentParse.arguments[0]].coords;
        }
        if (componentParse.scaledComponentOffset) {
            // cout << "scaledComponentOffset" << std::endl;
            offset = componentParse.scale * offset;
        }
        if (componentParse.roundXY) {
            // cout << "roundXY" << std::endl;
            offset = vec2(round(offset.x*unitsPerEm), round(offset.y*unitsPerEm)) / unitsPerEm;
        }
        if (simple) {
            component.offset = offset;
            component.transform = componentParse.scale;
            out.components = {component};
        }
        // cout << "scale = { "
        //      << component.scale.h.x1 << ", "
        //      << component.scale.h.x2 << ", "
        //      << component.scale.h.y1 << ", "
        //      << component.scale.h.y2 << " }, offset = { "
        //      << offset.x << ", " << offset.y << " }" << std::endl;
        for (glyfPoint& point : componentPoints) {
            point.coords = componentParse.scale * point.coords;
        }
        allPoints.Append(std::move(componentPoints));
        componentGlyph.Scale(componentParse.scale);
        componentGlyph.Offset(offset);
        out.curves.Append(std::move(componentGlyph.curves));
        out.lines.Append(std::move(componentGlyph.lines));
        out.components.Append(std::move(componentGlyph.components));
    }
    if (dstArray != nullptr) {
        *dstArray = std::move(allPoints);
    }
    return out;
}

longHorMetric hmtx::Metric(u32 glyphIndex, u16 numOfLongHorMetrics) const {
    if (glyphIndex < numOfLongHorMetrics) {
        return *(((longHorMetric*)this) + glyphIndex);
    }
    longHorMetric metric;
    metric.advanceWidth = (((longHorMetric*)this) + numOfLongHorMetrics-1)->advanceWidth;
    FWord_t *leftSideBearing = (FWord_t*)(((longHorMetric*)this) + numOfLongHorMetrics);
    leftSideBearing += glyphIndex - numOfLongHorMetrics;
    metric.leftSideBearing = *leftSideBearing;
    return metric;
}

} // namespace tables
} // namespace font

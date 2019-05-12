/*
    File: font.cpp
    Author: Philip Haynes
*/

#include "font.hpp"

#define LOG_VERBOSE

String ToString(font::Tag_t tag) {
    String string(4);
    for (u32 x = 0; x < 4; x++) {
        string[x] = tag.name[x];
    }
    return string;
}

// font::tables::cffs::Offset24 endianSwap(font::tables::cffs::Offset24 in) {
//     font::tables::cffs::Offset24 out;
//     for (u32 i = 0; i < 3; i++) {
//         out.bytes[i] = in.bytes[2-i];
//     }
//     return out;
// }

namespace font {

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

            typedef String (*fpCharStringOperatorResolution)(u8**, u8*);

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
                String out(false);
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
                String out(false);
                u8 operator1 = *(*data)++;
#define GETSID() out += ToString(endianSwap(*(SID*)firstOperand)); firstOperand += 2
#define GETARR()                            \
out += '{';                                 \
for (;*firstOperand != operator1;) {        \
    out += OperandString(&firstOperand);    \
    if (*firstOperand != operator1) {       \
        out += ", ";                        \
    }                                       \
}                                           \
out += '}';
                if (operator1 == 12) {
                    u8 operator2 = *(*data)++;
                    // Start with Private DICT data since there may be more than 1?
                    if (operator2 == 9) {
                        out += "BlueScale: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 10) {
                        out += "BlueShift: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 11) {
                        out += "BlueFuzz: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 12) {
                        out += "StemSnapH: ";
                        GETARR();
                    } else if (operator2 == 13) {
                        out += "StemSnapV: ";
                        GETARR();
                    } else if (operator2 == 14) {
                        out += "ForceBold: ";
                        out += boolString[*firstOperand];
                    } else if (operator2 == 17) {
                        out += "LanguageGroup: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 18) {
                        out += "ExpansionFactor: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 19) {
                        out += "initialRandomSeed: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 0) { // Top DICT
                        out += "Copyright: ";
                        GETSID();
                    } else if (operator2 == 1) {
                        out += "isFixedPitch: ";
                        out += boolString[*firstOperand];
                    } else if (operator2 == 2) {
                        out += "italicAngle: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 3) {
                        out += "UnderlinePosition: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 4) {
                        out += "UnderlineThickness: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 5) {
                        out += "PaintType: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 6) {
                        out += "CharstringType: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 7) {
                        out += "FontMatrix: ";
                        GETARR();
                    } else if (operator2 == 8) {
                        out += "StrokeWidth: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 20) {
                        out += "SyntheticBase: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 21) {
                        out += "PostScript: ";
                        GETSID();
                    } else if (operator2 == 22) {
                        out += "BaseFontName: ";
                        GETSID();
                    } else if (operator2 == 23) {
                        out += "BaseFontBlend(delta): ";
                        GETARR();
                    } else if (operator2 == 30) { // Here there be CIDFont Operators
                        out += "Registry: ";
                        GETSID();
                        out += " Ordering: ";
                        GETSID();
                        out += " Supplement: " + OperandString(&firstOperand);
                    } else if (operator2 == 31) {
                        out += "CIDFontVersion: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 32) {
                        out += "CIDFontRevision: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 33) {
                        out += "CIDFontType: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 34) {
                        out += "CIDCount: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 35) {
                        out += "UIDBase: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 36) {
                        out += "FDArray: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 37) {
                        out += "FDSelect: ";
                        out += OperandString(&firstOperand);
                    } else if (operator2 == 38) {
                        out += "CIDFontName: ";
                        GETSID();
                    } else {
                        out += "Operator ERROR 12 " + ToString((u32)operator2);
                    }
                } else if (operator1 == 6) { // Private DICT
                    out += "BlueValues: ";
                    GETARR();
                } else if (operator1 == 7) {
                    out += "OtherBlues: ";
                    GETARR();
                } else if (operator1 == 8) {
                    out += "FamilyBlues: ";
                    GETARR();
                } else if (operator1 == 9) {
                    out += "FamilyOtherBlues: ";
                    GETARR();
                } else if (operator1 == 10) {
                    out += "StdHW: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 11) {
                    out += "StdVW: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 19) {
                    out += "Subrs: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 20) {
                    out += "defaultWidthX: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 21) {
                    out += "nominalWidthX: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 0) { // Top DICT
                    out += "Version: ";
                    GETSID();
                } else if (operator1 == 1) {
                    out += "Notice: ";
                    GETSID();
                } else if (operator1 == 2) {
                    out += "FullName: ";
                    GETSID();
                } else if (operator1 == 3) {
                    out += "FamilyName: ";
                    GETSID();
                } else if (operator1 == 4) {
                    out += "Weight: ";
                    GETSID();
                } else if (operator1 == 5) {
                    out += "FontBBox: ";
                    GETARR();
                } else if (operator1 == 13) {
                    out += "UniqueID: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 14) {
                    out += "XUID: ";
                    GETARR();
                } else if (operator1 == 15) {
                    out += "charset: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 16) {
                    out += "Encoding: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 17) {
                    out += "CharStrings: ";
                    out += OperandString(&firstOperand);
                } else if (operator1 == 18) {
                    out += "Private: offset: ";
                    out += OperandString(&firstOperand) + ", size: " + OperandString(&firstOperand);
                } else {
                    out += "Operator ERROR " + ToString((u32)operator1);

                }
#undef GETSID
#undef GETARR
                return out;
            }

            String CharString(u8 *start, u8 *end) {
                String out(false);
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
#define GETSID(var) (var) = endianSwap(*(SID*)firstOperand); firstOperand += 2
#define GETARR(arr, ophandler)              \
arr.Clear();                                \
for (;*firstOperand != operator1;) {        \
    arr.Append(ophandler(&firstOperand));   \
}
                if (operator1 == 12) {
                    u8 operator2 = *(*data)++;
                    if (operator2 == 9) { // Private DICT
                        BlueScale = OperandF32(&firstOperand);
                    } else if (operator2 == 10) {
                        BlueShift = OperandF32(&firstOperand);
                    } else if (operator2 == 11) {
                        BlueFuzz = OperandF32(&firstOperand);
                    } else if (operator2 == 12) {
                        GETARR(StemSnapH, OperandF32);
                    } else if (operator2 == 13) {
                        GETARR(StemSnapV, OperandF32);
                    } else if (operator2 == 14) {
                        ForceBold = *firstOperand != 0;
                    } else if (operator2 == 17) {
                        LanguageGroup = OperandI32(&firstOperand);
                    } else if (operator2 == 18) {
                        ExpansionFactor = OperandF32(&firstOperand);
                    } else if (operator2 == 19) {
                        initialRandomSeed = OperandI32(&firstOperand);
                    } else if (operator2 == 0) { // Top DICT
                        GETSID(Copyright);
                    } else if (operator2 == 1) {
                        isFixedPitch = *firstOperand != 0;
                    } else if (operator2 == 2) {
                        ItalicAngle = OperandI32(&firstOperand);
                    } else if (operator2 == 3) {
                        UnderlinePosition = OperandI32(&firstOperand);
                    } else if (operator2 == 4) {
                        UnderlineThickness = OperandI32(&firstOperand);
                    } else if (operator2 == 5) {
                        PaintType = OperandI32(&firstOperand);
                    } else if (operator2 == 6) {
                        CharstringType = OperandI32(&firstOperand);
                    } else if (operator2 == 7) {
                        GETARR(FontMatrix, OperandF32);
                    } else if (operator2 == 8) {
                        StrokeWidth = OperandF32(&firstOperand);
                    } else if (operator2 == 20) {
                        SyntheticBase = OperandI32(&firstOperand);
                    } else if (operator2 == 21) {
                        GETSID(PostScript);
                    } else if (operator2 == 22) {
                        GETSID(BaseFontName);
                    } else if (operator2 == 23) {
                        GETARR(BaseFontBlend, OperandI32);
                    } else if (operator2 == 30) { // Here there be CIDFont Operators
                        GETSID(ROS.registry);
                        GETSID(ROS.ordering);
                        ROS.supplement = OperandI32(&firstOperand);
                    } else if (operator2 == 31) {
                        CIDFontVersion = OperandF32(&firstOperand);
                    } else if (operator2 == 32) {
                        CIDFontRevision = OperandF32(&firstOperand);
                    } else if (operator2 == 33) {
                        CIDFontType = OperandI32(&firstOperand);
                    } else if (operator2 == 34) {
                        CIDCount = OperandI32(&firstOperand);
                    } else if (operator2 == 35) {
                        UIDBase = OperandI32(&firstOperand);
                    } else if (operator2 == 36) {
                        FDArray = OperandI32(&firstOperand);
                    } else if (operator2 == 37) {
                        FDSelect = OperandI32(&firstOperand);
                    } else if (operator2 == 38) {
                        GETSID(FontName);
                    }
                } else if (operator1 == 6) { // Private DICT
                    GETARR(BlueValues, OperandI32);
                } else if (operator1 == 7) {
                    GETARR(OtherBlues, OperandI32);
                } else if (operator1 == 8) {
                    GETARR(FamilyBlues, OperandI32);
                } else if (operator1 == 9) {
                    GETARR(FamilyOtherBlues, OperandI32);
                } else if (operator1 == 10) {
                    StdHW = OperandF32(&firstOperand);
                } else if (operator1 == 11) {
                    StdVW = OperandF32(&firstOperand);
                } else if (operator1 == 19) {
                    Subrs = OperandI32(&firstOperand);
                } else if (operator1 == 20) {
                    defaultWidthX = OperandI32(&firstOperand);
                } else if (operator1 == 21) {
                    nominalWidthX = OperandI32(&firstOperand);
                } else if (operator1 == 0) { // Top DICT
                    GETSID(version);
                } else if (operator1 == 1) {
                    GETSID(Notice);
                } else if (operator1 == 2) {
                    GETSID(FullName);
                } else if (operator1 == 3) {
                    GETSID(FamilyName);
                } else if (operator1 == 4) {
                    GETSID(Weight);
                } else if (operator1 == 5) {
                    GETARR(FontBBox, OperandI32);
                } else if (operator1 == 14) {
                    GETARR(XUID, OperandI32);
                } else if (operator1 == 15) {
                    charset = OperandI32(&firstOperand);
                } else if (operator1 == 16) {
                    Encoding = OperandI32(&firstOperand);
                } else if (operator1 == 17) {
                    CharStrings = OperandI32(&firstOperand);
                } else if (operator1 == 18) {
                    Private.offset = OperandI32(&firstOperand);
                    Private.size = OperandI32(&firstOperand);
                } else {
                    cout << "Operator Error" << std::endl;
                }
#undef GETSID
#undef GETARR
            }

        }

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

        void cmap_format0::EndianSwap() {
            // format is already done.
            // NOTE: In this case it may be ideal to just ignore these values entirely
            //       but I'd rather be a completionist first and remove stuff later.
            ENDIAN_SWAP(length);
            ENDIAN_SWAP(language);
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
            // 5 arrays of size segCount and 1 scalar
            for (u16 i = 0; i < segCount*5+1; i++) {
                ENDIAN_SWAP(*ptr);
                ptr++;
            }
        }

        void cmap_format12_group::EndianSwap() {
            ENDIAN_SWAP(startCharCode);
            ENDIAN_SWAP(endCharCode);
            ENDIAN_SWAP(startGlpyhCode);
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
                        cout << "Long offset: " << loc->offsets32(i) << std::endl;
                        DO_SWAP();
                        offsetsDone.insert(loc->offsets32(i));
                    }
                }
            } else {
                Set<u16> offsetsDone;
                for (u16 i = 0; i < numGlyphs; i++) {
                    if (offsetsDone.count(loc->offsets16(i)) == 0) {
                        glyf_header *header = (glyf_header*)(((char*)this) + loc->offsets16(i) * 2);
                        cout << "Short offset[" << i << "]: " << loc->offsets16(i) * 2 << std::endl;
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
            cout << "Simple Glyph with " << header->numberOfContours << " contours..." << std::endl;
            for (i16 i = 0; i < header->numberOfContours; i++) {
                ENDIAN_SWAP(*(u16*)ptr);
                ptr += 2;
            }
            u16& instructionLength = ENDIAN_SWAP(*(u16*)ptr);
            cout << "instructionLength = " << instructionLength << std::endl;
            ptr += instructionLength + 2;
            // Now it gets weird
            u16 nPoints;
            if (header->numberOfContours > 0) {
                nPoints = endPtsOfContours[header->numberOfContours-1];
            } else {
                nPoints = 0;
            }
            cout << "We have " << nPoints << " points..." << std::endl;
            // Now figure out how many flag bytes there actually are
            u8 *flags = (u8*)ptr;
            u16 nFlags = nPoints;
            for (u16 i = 0; i < nFlags; i++) {
                if (flags[i] & 0x08) { // Bit 3
                    nFlags -= flags[++i] - 1;
                }
            }
            cout << "We have " << nFlags << " flags..." << std::endl;
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
                        ENDIAN_SWAP(*(u16*)ptr);
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
                        ENDIAN_SWAP(*(u16*)ptr);
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
            cout << "Completed! Total size: " << u32(ptr-(char*)header) << "\n" << std::endl;
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
            cout << "Compound Glyph with " << components << " component glyphs..." << std::endl;
            // That was somehow simpler than the "Simple" Glyph... tsk
            cout << "Completed! Total size: " << u32(ptr-(char*)header) << "\n" << std::endl;
        }

        void cffs::charset_format_any::EndianSwap(Card16 nGlyphs) {
            if (format == 0) {
                ((cffs::charset_format0*)this)->EndianSwap(nGlyphs);
            } else if (format == 1) {
                ((cffs::charset_format1*)this)->EndianSwap(nGlyphs);
            } else if (format == 2) {
                ((cffs::charset_format2*)this)->EndianSwap(nGlyphs);
            } else {
                cout << "Unsupported charset format " << (u32)format << std::endl;
            }
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
            cout << "charset_range2: first = " << (u32)first << ", nLeft = " << nLeft << std::endl;
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

        void cffs::FDSelect_any::EndianSwap() {
            if (format == 0) {
                cout << "Format 0" << std::endl;
            } else if (format == 3) {
                cout << "Format 3" << std::endl;
                ((cffs::FDSelect_format3*)this)->EndianSwap();
            } else {
                cout << "Unsupported FDSelect format " << format << std::endl;
            }
        }

        void cffs::FDSelect_format3::EndianSwap() {
            ENDIAN_SWAP(nRanges);
            cout << "nRanges = " << nRanges << std::endl;
            cffs::FDSelect_range3 *range = (cffs::FDSelect_range3*)(&format + 3);
            for (u32 i = 0; i < (u32)nRanges; i++) {
                ENDIAN_SWAP(range->first);
                // cout << "Range[" << i << "].first = " << range->first << ", fd = " << (u32)range->fd << std::endl;
                range++;
            }
            ENDIAN_SWAP(range->first);
            // cout << "Sentinel = " << range->first << std::endl;
        }

        Array<u32> cffs::index::EndianSwap(char **ptr, char **dataStart) {
            ENDIAN_SWAP(count);
            *ptr += 2;
            u32 lastOffset = 1;
            Array<u32> offsets;
            if (count != 0) {
                offsets.Resize(count+1);
                cout << "count = " << count << ", offSize = " << (u32)offSize << std::endl;
                (*ptr)++; // Getting over offSize
                for (u32 i = 0; i < (u32)count+1; i++) {
                    u32 offset;
                    if (offSize == 1) {
                        offset = *((u8*)(*ptr)++);
                    } else if (offSize == 2) {
                        cffs::Offset16 *off = (cffs::Offset16*)(u8*)*ptr;
                        ENDIAN_SWAP(*off);
                        offset = *off;
                        *ptr += 2;
                    } else if (offSize == 3) {
                        cffs::Offset24 *off = (cffs::Offset24*)(u8*)*ptr;
                        offset = off->value();
                        *ptr += 3;
                    } else if (offSize == 4) {
                        cffs::Offset32 *off = (cffs::Offset32*)(u8*)*ptr;
                        ENDIAN_SWAP(*off);
                        offset = *off;
                        *ptr += 4;
                    } else {
                        cout << "Unsupported offSize: " << (u32)offSize << std::endl;
                        return Array<u32>();
                    }
                    if (i == count) {
                        lastOffset = offset;
                    }
                    offsets[i] = offset;
                }
            }
            *dataStart = *ptr - 1;
            *ptr += lastOffset - 1;
            return offsets;
        }

        void cff::EndianSwap() {
            char *ptr = (char*)this + header.size;
            cffs::index *nameIndex = (cffs::index*)ptr;
            char *nameIndexData;
            cout << "nameIndex:\n";
            Array<u32> nameIndexOffsets = nameIndex->EndianSwap(&ptr, &nameIndexData);
            cout << "nameIndex data:\n";
            for (i32 i = 0; i < nameIndexOffsets.size-1; i++) {
                String string(nameIndexOffsets[i+1] - nameIndexOffsets[i]);
                memcpy(string.data, nameIndexData + nameIndexOffsets[i], string.size);
                cout << "[" << i << "]=\"" << string << "\" ";
            }
            cout << std::endl;
            if (nameIndexOffsets.size > 2) {
                cout << "We only support CFF tables with 1 Name entry." << std::endl;
                return;
            }

            cffs::index *dictIndex = (cffs::index*)ptr;
            char *dictIndexData;
            cout << "dictIndex:\n";
            Array<u32> dictIndexOffsets = dictIndex->EndianSwap(&ptr, &dictIndexData);
            cout << "dictIndex charstrings:\n" << cffs::CharString((u8*)dictIndexData+dictIndexOffsets[0], (u8*)dictIndexData + dictIndexOffsets[dictIndexOffsets.size-1]) << std::endl;
            cffs::dict dictIndexValues;
            dictIndexValues.ParseCharString((u8*)dictIndexData+dictIndexOffsets[0], dictIndexOffsets[1]-dictIndexOffsets[0]);

            if (dictIndexValues.CharstringType != 2) {
                cout << "Unsupported CharstringType " << dictIndexValues.CharstringType << std::endl;
                return;
            }

            cffs::index *stringsIndex = (cffs::index*)ptr;
            char *stringsIndexData;
            cout << "stringsIndex:\n";
            Array<u32> stringsIndexOffsets = stringsIndex->EndianSwap(&ptr, &stringsIndexData);
            cout << "stringsIndex data:\n";
            for (i32 i = 0; i < stringsIndexOffsets.size-1; i++) {
                String string(stringsIndexOffsets[i+1] - stringsIndexOffsets[i]);
                memcpy(string.data, stringsIndexData + stringsIndexOffsets[i], string.size);
                cout << "\n[" << i << "]=\"" << string << "\" ";
            }
            cout << std::endl;

            cffs::index *gsubrIndex = (cffs::index*)ptr;
            char *gsubrIndexData;
            cout << "gsubrIndex:\n";
            Array<u32> gsubrIndexOffsets = gsubrIndex->EndianSwap(&ptr, &gsubrIndexData);
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

            if (dictIndexValues.CharStrings == -1) {
                cout << "WHAAAT NO CHARSTRINGS???" << std::endl;
                return;
            }

            cout << "charStringsIndex:\n";
            ptr = (char*)this + dictIndexValues.CharStrings;
            cffs::index *charStringsIndex = (cffs::index*)ptr;
            char *charStringsIndexData;
            Array<u32> charStringsIndexOffsets = charStringsIndex->EndianSwap(&ptr, &charStringsIndexData);

            // Do we have a predefined charset or a custom one?
            if (dictIndexValues.charset == 0) {
                // ISOAdobe charset
                cout << "We are using the ISOAdobe predefined charset." << std::endl;
            } else if (dictIndexValues.charset == 1) {
                // Expert charset
                cout << "We are using the Expert predefined charset." << std::endl;
            } else if (dictIndexValues.charset == 2) {
                // ExpertSubset charset
                cout << "We are using the ExpertSubset predefined charset." << std::endl;
            } else {
                // Custom charset
                cffs::charset_format_any *charset = (cffs::charset_format_any*)((char*)this + dictIndexValues.charset);
                cout << "We are using a custom charset with format " << (i32)charset->format << std::endl;
                charset->EndianSwap(charStringsIndex->count);
            }

            if (dictIndexValues.FDSelect != -1) {
                if (dictIndexValues.FDArray == -1) {
                    cout << "CIDFonts must have an FDArray!" << std::endl;
                    return;
                }
                cout << "FDSelect:\n";
                cffs::FDSelect_any *fdSelect = (cffs::FDSelect_any*)((char*)this + dictIndexValues.FDSelect);
                fdSelect->EndianSwap();

                cout << "FDArray:\n";
                ptr = (char*)this + dictIndexValues.FDArray;
                cffs::index *fdArray = (cffs::index*)ptr;
                char *fdArrayData;
                Array<u32> fdArrayOffsets = fdArray->EndianSwap(&ptr, &fdArrayData);
                for (i32 i = 0; i < fdArrayOffsets.size-1; i++) {
                    cout << "fontDictIndex[" << i << "] charstrings: "
                         << cffs::CharString((u8*)fdArrayData+fdArrayOffsets[i], (u8*)fdArrayData+fdArrayOffsets[i+1])
                         << std::endl;
                }
            }


        }

#undef ENDIAN_SWAP
#undef ENDIAN_SWAP_FIXED

    }

    bool Font::Load() {
        if (filename.size == 0) {
            error = "No filename specified!";
            return false;
        }
        cout << "Attempting to load \"" << filename << "\"" << std::endl;

        data.file.open(filename.data, std::ios::in | std::ios::binary);
        if (!data.file.is_open()) {
            error = "Failed to open font with filename: \"" + filename + "\"";
            return false;
        }

        data.ttcHeader.Read(data.file);

        if (data.ttcHeader.ttcTag == 0x10000_Tag) {
            cout << "TrueType outline" << std::endl;
        } else if (data.ttcHeader.ttcTag == "true"_Tag) {
            cout << "TrueType" << std::endl;
        } else if (data.ttcHeader.ttcTag == "OTTO"_Tag) {
            cout << "OpenType with CFF" << std::endl;
        } else if (data.ttcHeader.ttcTag == "typ1"_Tag) {
            cout << "Old-style PostScript" << std::endl;
        } else if (data.ttcHeader.ttcTag == "ttcf"_Tag) {
            cout << "TrueType Collection" << std::endl;
        } else {
            error = "Unknown font type for file: \"" + filename + "\"";
            return false;
        }

        data.offsetTables.Resize(data.ttcHeader.numFonts);
        Set<u32> uniqueOffsets;
        for (u32 i = 0; i < data.ttcHeader.numFonts; i++) {
            data.file.seekg(data.ttcHeader.offsetTables[i]);
            tables::Offset &offsetTable = data.offsetTables[i];
            offsetTable.Read(data.file);
#ifdef LOG_VERBOSE
            cout << "Font[" << i << "]\n\tnumTables: " << offsetTable.numTables
                 << "\n\tsearchRange: " << offsetTable.searchRange
                 << "\n\tentrySelector: " << offsetTable.entrySelector
                 << "\n\trangeShift: " << offsetTable.rangeShift << std::endl;
#endif
            for (u32 ii = 0; ii < offsetTable.numTables; ii++) {
                tables::Record &record = offsetTable.tables[ii];
                if (record.offset < data.offsetMin) {
                    data.offsetMin = record.offset;
                }
                if (record.offset + record.length > data.offsetMax) {
                    data.offsetMax = record.offset + record.length;
                }
                // Keep track of unique tables
                if (uniqueOffsets.count(record.offset) == 0) {
                    data.uniqueTables.Append(record);
                    uniqueOffsets.insert(record.offset);
                }
#ifdef LOG_VERBOSE
                cout << "\tTable: \"" << ToString(record.tableTag) << "\"\n\t\toffset = " << record.offset << ", length = " << record.length << std::endl;
#endif
            }
        }

#ifdef LOG_VERBOSE
        cout << "offsetMin = " << data.offsetMin << ", offsetMax = " << data.offsetMax << std::endl;
#endif
        data.tableData.Resize(align(data.offsetMax-data.offsetMin, 4));

        data.file.seekg(data.offsetMin); // Should probably already be here but you never know ;)
        data.file.read(data.tableData.data, data.offsetMax-data.offsetMin);

        data.file.close();

        // Checksum verifications

        u32 checksumsCompleted = 0;
        u32 checksumsCorrect = 0;

        for (i32 i = 0; i < data.uniqueTables.size; i++) {
            tables::Record &record = data.uniqueTables[i];
            char *ptr = data.tableData.data + record.offset - data.offsetMin;
            if (record.tableTag == "head"_Tag || record.tableTag == "bhed"_Tag) {
                ((tables::head*)ptr)->checkSumAdjustment = 0;
            }
            u32 checksum = tables::Checksum((u32*)ptr, record.length);
            if (checksum != record.checkSum) {
                cout << "Checksum for table " << ToString(record.tableTag) << " didn't match!" << std::endl;
            } else {
                checksumsCorrect++;
            }
            checksumsCompleted++;
        }
        cout << "Checksums completed. " << checksumsCorrect
             << "/" << checksumsCompleted << " correct.\n" << std::endl;

        // File is in big-endian, so we may need to swap the data before we can use it.

        if (SysEndian.little) {
            u16 numGlyphs = 0;
            bool longOffsets = false;
            for (i32 i = 0; i < data.uniqueTables.size; i++) {
                tables::Record &record = data.uniqueTables[i];
                char *ptr = data.tableData.data + record.offset - data.offsetMin;
                const Tag_t &tag = record.tableTag;
                Array<u32> uniqueEncodingOffsets;
                if (tag == "head"_Tag || tag == "bhed"_Tag) {
                    tables::head *head = (tables::head*)ptr;
                    head->EndianSwap();
                    if (head->indexToLocFormat == 1) {
                        longOffsets = true;
                    }
                } else if (tag == "cmap"_Tag) {
                    tables::cmap_index *index = (tables::cmap_index*)ptr;
                    index->EndianSwap();
                    for (u32 enc = 0; enc < index->numberSubtables; enc++) {
                        tables::cmap_encoding *encoding = (tables::cmap_encoding*)(ptr + 4 + enc * sizeof(tables::cmap_encoding));
                        encoding->EndianSwap();
                        if (!uniqueEncodingOffsets.Contains(encoding->offset)) {
                            uniqueEncodingOffsets.Append(encoding->offset);
                            tables::cmap_format_any *cmap = (tables::cmap_format_any*)(ptr + encoding->offset);
                            if (!cmap->EndianSwap()) {
                                cout << "Unsupported cmap table format " << cmap->format << std::endl;
                            }
                        }
                    }
                } else if (tag == "maxp"_Tag) {
                    tables::maxp *maxp = (tables::maxp*)ptr;
                    maxp->EndianSwap();
                    numGlyphs = maxp->numGlyphs;
                } else if (tag == "CFF "_Tag) {
                    tables::cff *cff = (tables::cff*)ptr;
                    cff->EndianSwap();
                }
            }
            // To parse the 'loca' table correctly, head needs to be parsed first
            tables::loca *loca = nullptr;
            for (i32 i = 0; i < data.uniqueTables.size; i++) {
                tables::Record &record = data.uniqueTables[i];
                char *ptr = data.tableData.data + record.offset - data.offsetMin;
                const Tag_t &tag = record.tableTag;
                if (tag == "loca"_Tag) {
                    loca = (tables::loca*)ptr;
                    loca->EndianSwap(numGlyphs, longOffsets);
                    u32 locaGlyphs = record.length / (2 << (u32)longOffsets)-1;
                    if (numGlyphs != locaGlyphs) {
                        error = "Glyph counts don't match between maxp(" + ToString(numGlyphs)
                              + ") and loca(" + ToString(locaGlyphs) + ")";
                        return false;
                    }
                }
            }
            // To parse the 'glyf' table correctly, loca needs to be parsed first
            for (i32 i = 0; i < data.uniqueTables.size; i++) {
                tables::Record &record = data.uniqueTables[i];
                char *ptr = data.tableData.data + record.offset - data.offsetMin;
                const Tag_t &tag = record.tableTag;
                if (tag == "glyf"_Tag) {
                    if (loca == nullptr) {
                        error = "Cannot parse glyf table without a loca table!";
                        return false;
                    }
                    tables::glyf *glyf = (tables::glyf*)ptr;
                    glyf->EndianSwap(loca, numGlyphs, longOffsets);
                }
            }
        }

        // Figuring out what data to actually use

        data.cmaps.Resize(data.ttcHeader.numFonts); // One per font

        for (i32 i = 0; i < data.offsetTables.size; i++) {
#ifdef LOG_VERBOSE
            cout << "\nFont[" << i << "]\n\n";
#endif
            tables::Offset &offsetTable = data.offsetTables[i];
            i16 chosenCmap = -1; // -1 is not found, bigger is better
            for (i32 ii = 0; ii < offsetTable.numTables; ii++) {
                tables::Record &record = offsetTable.tables[ii];
                char *ptr = data.tableData.data + record.offset - data.offsetMin;
                Tag_t &tag = record.tableTag;
#ifdef LOG_VERBOSE
                if (tag == "head"_Tag || tag == "bhed"_Tag) {
                    tables::head *head = (tables::head*)ptr;
                    cout << "\thead table:\nVersion " << head->version.major << "." << head->version.minor
                         << " Revision: " << head->fontRevision.major << "." << head->fontRevision.minor
                         << "\nFlags: 0x" << std::hex << head->flags << " MacStyle: 0x" << head->macStyle
                         << std::dec << " unitsPerEm: " << head->unitsPerEm
                         << "\nxMin: " << head->xMin << " xMax: " << head->xMax
                         << " yMin: " << head->yMin << " yMax " << head->yMax << "\n" << std::endl;
                } else if (tag == "CFF "_Tag) {
                    tables::cff *cff = (tables::cff*)ptr;
                    cout << "\tCFF version: " << (u32)cff->header.versionMajor << "." << (u32)cff->header.versionMinor
                         << "\nheader.size: " << (u32)cff->header.size
                         << ", header.offSize: " << (u32)cff->header.offSize << std::endl;
                } else if (tag == "maxp"_Tag) {
                    tables::maxp *maxp = (tables::maxp*)ptr;
                    cout << "\tmaxp table:\nnumGlyphs: " << maxp->numGlyphs << "\n" << std::endl;
                }
#endif
                if (tag == "cmap"_Tag) {
                    tables::cmap_index *index = (tables::cmap_index*)ptr;
#ifdef LOG_VERBOSE
                    cout << "\tcmap table:\nVersion " << index->version
                         << " numSubtables: " << index->numberSubtables << "\n";
#endif
                    for (u32 enc = 0; enc < index->numberSubtables; enc++) {
                        tables::cmap_encoding *encoding = (tables::cmap_encoding*)(ptr + 4 + enc * sizeof(tables::cmap_encoding));
// #ifdef LOG_VERBOSE
//                         cout << "\tEncoding[" << enc << "]:\nPlatformID: " << encoding->platformID
//                              << " PlatformSpecificID: " << encoding->platformSpecificID
//                              << "\nOffset: " << encoding->offset
//                              << "\nFormat: " << *((u16*)(ptr+encoding->offset)) << "\n";
// #endif
                        #define CHOOSE(in) chosenCmap = in; \
                                data.cmaps[i] = u32((char*)index - data.tableData.data) + encoding->offset
                        if (encoding->platformID == 0 && encoding->platformSpecificID == 4) {
                            CHOOSE(4);
                        } else if (encoding->platformID == 0 && encoding->platformSpecificID == 3 && chosenCmap < 4) {
                            CHOOSE(3);
                        } else if (encoding->platformID == 3 && encoding->platformSpecificID == 10 && chosenCmap < 3) {
                            CHOOSE(2);
                        } else if (encoding->platformID == 3 && encoding->platformSpecificID == 1 && chosenCmap < 2) {
                            CHOOSE(1);
                        } else if (encoding->platformID == 3 && encoding->platformSpecificID == 0 && chosenCmap < 1) {
                            CHOOSE(0);
                        }
                        #undef CHOOSE
                    }
#ifdef LOG_VERBOSE
                    cout << std::endl;
#endif
                }
            }
            if (chosenCmap == -1) {
                cout << "Font[" << i << "] doesn't have a supported cmap table." << std::endl;
                data.offsetTables.Erase(i--);
            } else {
#ifdef LOG_VERBOSE
                cout << "chosenCmap = " << chosenCmap << " offset = " << data.cmaps[i]
                     << " format = " << *((u16*)(data.tableData.data+data.cmaps[i])) << "\n" << std::endl;
#endif
            }
        }
        if (data.offsetTables.size == 0) {
            error = "Font file not supported.";
            return false;
        }

        return true;
    }

}

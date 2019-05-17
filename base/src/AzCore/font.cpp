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

    inline f32 BezierDerivative(f32 t, f32 p1, f32 p2, f32 p3) {
        return 2.0 * ((1.0-t) * (p2-p1) + t * (p3-p2));
    }

    i32 Curve::Intersection(vec2 point) {
        i32 winding = 0;
        // Bezier(t) = (1-t)*((1-t)*p1 + t*p2) + t*((1-t)*p2 + t*p3)
        // Bezier(t) = (1-t)*(1-t)*p1 + (1-t)t*(2*p2) + t^2*p3
        // Bezier(t) = p1 - t(2*p1) + t^2(p1) + t(2*p2) - t^2(2*p2) + t^2(p3)
        // Bezier(t) = t^2(p3 - 2*p2 + p1) + t(2*p2 - 2*p1) + p1
        // Bezier'(t) = 2(1-t)(p2-p1) + 2t(p3-p2);

        // Bezier(t) = (1-t)*p1 + t*p3
        // Bezier(t) = t(p3 - p1) + p1
        // t = -p1 / (p3 - p1)
        f32 a, b, c; // coefficients of the curve: y = ax^2 + bx + c
        a = p3.y - 2.0*p2.y + p1.y;
        if (a == 0.0) {
            // Straight line
            if (p3.x == p1.x) {
                // Vertical line
                if (p3.x > point.x) {
                    if (point.y == median(p3.y, point.y, p1.y)) {
                        return p1.y < p3.y ? 1 : -1;
                    }
                }
            } else {
                // Now represents the line: y = ax + b
                a = p3.y - p1.y;
                if (a == 0.0) {
                    // Horizontal line
                    return 0;
                } else {
                    b = p1.y - point.y;
                    f32 t = -b / a;
                    if (t == median(0.0, t, 1.0)) {
                        f32 x = (p3.x - p1.x) * t + p1.x;
                        if (x > point.x) {
                            return a > 0.0 ? 1 : -1;
                        }
                    }
                }
            }
            return 0;
        }
        b = 2.0*(p2.y - p1.y);
        c = p1.y - point.y;
        const f32 bb = square(b);
        const f32 ac4 = 4.0*a*c;
        if (bb <= ac4) {
            // We don't care about complex answers or tangent lines.
            return 0;
        }
        const f32 squareRoot = sqrt(bb - ac4);
        // cout << "squareRoot = " << squareRoot << std::endl;
        f32 t1 = (-b + squareRoot) / (2.0 * a);
        if (t1 == median(0.0, t1, 1.0)) {
            f32 x = (p3.x - 2.0*p2.x + p1.x) * square(t1) + 2.0*(p2.x - p1.x) * t1 + p1.x;
            if (x > point.x) {
                // We have an intersection
                winding += BezierDerivative(t1, p1.y, p2.y, p3.y) > 0.0 ? 1 : -1;
            }
        }
        f32 t2 = (-b - squareRoot) / (2.0 * a);
        if (t2 == median(0.0, t2, 1.0)) {
            f32 x = (p3.x - 2.0*p2.x + p1.x) * square(t2) + 2.0*(p2.x - p1.x) * t2 + p1.x;
            if (x > point.x) {
                // We have an intersection
                winding += BezierDerivative(t2, p1.y, p2.y, p3.y) > 0.0 ? 1 : -1;
            }
        }
        return winding;
    }

    i32 Contour::Intersection(vec2 point) {
        i32 winding = 0;
        for (i32 i = 0; i < points.size-1; i += 2) {
            Curve *curve = (Curve*)&points[i];
            winding += curve->Intersection(point);
        }
        return winding;
    }

    void Contour::FromGlyfPoints(glyfPoint *glyfPoints, i32 count) {
        points.Reserve(count);
        bool lastOnCurve = glyfPoints[0].onCurve;
        if (!lastOnCurve) {
            points.Append((glyfPoints[count-1].coords + glyfPoints[0].coords) * 0.5);
        }
        points.Append(glyfPoints[0].coords);
        for (i32 i = 1; i < count; i++) {
            if (lastOnCurve == glyfPoints[i].onCurve) {
                points.Append((glyfPoints[(i-1)%count].coords + glyfPoints[i].coords) * 0.5);
            }
            lastOnCurve = glyfPoints[i].onCurve;
            points.Append(glyfPoints[i].coords);
        }
        if (lastOnCurve == glyfPoints[0].onCurve) {
            points.Append((glyfPoints[count-1].coords + glyfPoints[0].coords) * 0.5);
        }
        points.Append(glyfPoints[0].coords);
        // for (i32 i = 0; i < points.size; i++) {
        //     cout << "point[" << i << "] = (" << points[i].x << "," << points[i].y << ")\n";
        // }
        // cout << std::endl;
    }

    bool Glyph::Inside(vec2 point) {
        i32 winding = 0;
        for (Contour& contour : contours) {
            winding += contour.Intersection(point);
        }
        return winding != 0;
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
            u32 segment;
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

        Glyph glyf_header::Parse(u16 unitsPerEm) {
            Glyph out;
            if (numberOfContours >= 0) {
                out = ParseSimple(unitsPerEm);
            } else {
                out = ParseCompound(unitsPerEm);
            }
            return out;
        }

        Glyph glyf_header::ParseSimple(u16 unitsPerEm) {
            Glyph out;
            out.contours.Resize(numberOfContours);

            // cout << "xMin = " << xMin << ", xMax = " << xMax << ", yMin = " << yMin << ", yMax = " << yMax << std::endl;

            char *ptr = (char*)(this+1);
            u16* endPtsOfContours = (u16*)ptr;
            ptr += 2 * numberOfContours;
            u16& instructionLength = *(u16*)ptr;
            // cout << "instructionLength = " << instructionLength << std::endl;
            ptr += instructionLength + 2;
            // Now it gets weird
            u16 nPoints;
            if (numberOfContours > 0) {
                nPoints = endPtsOfContours[numberOfContours-1] + 1;
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
                points[pointIndex].coords.x /= (f32)unitsPerEm;
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
                points[pointIndex].coords.y /= (f32)unitsPerEm;
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
            out.contours[0].FromGlyfPoints(pt, endPtsOfContours[0] + 1);
            pt += endPtsOfContours[0] + 1;
            for (i32 i = 1; i < out.contours.size; i++) {
                out.contours[i].FromGlyfPoints(pt, endPtsOfContours[i] - endPtsOfContours[i-1]);
                pt += endPtsOfContours[i] - endPtsOfContours[i-1];
            }
            return out;
        }

        Glyph glyf_header::ParseCompound(u16 unitsPerEm) {
            Glyph out;
            return out;
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
                nPoints = endPtsOfContours[header->numberOfContours-1] + 1;
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
#ifdef LOG_VERBOSE
                                cout << "Unsupported cmap table format " << cmap->format << std::endl;
#endif
                            }
                        }
                    }
                } else if (tag == "maxp"_Tag) {
                    tables::maxp *maxp = (tables::maxp*)ptr;
                    maxp->EndianSwap();
                    numGlyphs = maxp->numGlyphs;
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
                } else if (tag == "CFF "_Tag && !data.cffParsed.active) {
                    tables::cff *cff = (tables::cff*)ptr;
                    if (!cff->Parse(&data.cffParsed, SysEndian.little)) {
                        return false;
                    }
                } else if (tag == "glyf"_Tag && !data.glyfParsed.active) {
                    data.glyfParsed.active = true;
                    data.glyfParsed.glyphData = (tables::glyf*)ptr;
                } else if (tag == "loca"_Tag) {
                    data.glyfParsed.indexToLoc = (tables::loca*)ptr;
                } else if (tag == "maxp"_Tag) {
                    data.glyfParsed.maxProfile = (tables::maxp*)ptr;
                } else if (tag == "head"_Tag || tag == "bhed"_Tag) {
                    data.glyfParsed.header = (tables::head*)ptr;
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

        if (data.glyfParsed.active) {
            // This at least guarantees glyphData is valid
            // NOTE: These tests are probably overkill, but then again if we're a big-endian system
            //       they wouldn't have been done in the EndianSwap madness. Better safe than sorry???
            if (data.glyfParsed.header == nullptr) {
                error = "Can't use glyf without head!";
                return false;
            }
            if (data.glyfParsed.maxProfile == nullptr) {
                error = "Can't use glyf without maxp!";
                return false;
            }
            if (data.glyfParsed.indexToLoc == nullptr) {
                error = "Can't use glyf without loca!";
                return false;
            }
            data.glyfParsed.glyfOffsets.Resize((u32)data.glyfParsed.maxProfile->numGlyphs+1);
            if (data.glyfParsed.header->indexToLocFormat == 1) { // Long Offsets
                for (u32 i = 0; i < (u32)data.glyfParsed.maxProfile->numGlyphs + 1; i++) {
                    u32 *offsets = (u32*)data.glyfParsed.indexToLoc;
                    data.glyfParsed.glyfOffsets[i] = offsets[i];
                }
            } else { // Short Offsets
                for (u32 i = 0; i < (u32)data.glyfParsed.maxProfile->numGlyphs + 1; i++) {
                    u16 *offsets = (u16*)data.glyfParsed.indexToLoc;
                    data.glyfParsed.glyfOffsets[i] = offsets[i] * 2;
                }
            }
        }

        cout << "Successfully prepared \"" << filename << "\" for usage." << std::endl;

        return true;
    }

    void Font::PrintGlyph(char32 glyph) {
        u32 glyphIndex = 0;
        for (i32 i = 0; i < data.cmaps.size; i++) {
            tables::cmap_format_any *cmap = (tables::cmap_format_any*)(data.tableData.data + data.cmaps[i]);
            glyphIndex = cmap->GetGlyphIndex(glyph);
            if (glyphIndex) {
                break;
            }
        }
        cout << "glyph " << glyph << " has a glyph index of " << glyphIndex << std::endl;
        if (glyphIndex == 0) {
            return;
        }
        if (data.cffParsed.active) {
            cout << "Not yet implemented." << std::endl;
        } else if (data.glyfParsed.active) {
            tables::glyf_header *header = (tables::glyf_header*)((char*)data.glyfParsed.glyphData + data.glyfParsed.glyfOffsets[glyphIndex]);
            if (header->numberOfContours <= 0) {
                return;
            }
            // cout << "numberOfContours = " << header->numberOfContours << std::endl;
            Glyph glyph = header->Parse(data.glyfParsed.header->unitsPerEm);
            for (f32 y = 0.0; y <= 1.0; y += 1.0/60.0) {
                for (f32 x = 0.0; x <= 1.0; x += 1.0/100.0) {
                    vec2 point(x, 1.0-y);
                    if (glyph.Inside(point)) {
                        cout << 'X';
                    } else {
                        cout << ' ';
                    }
                }
                cout << "\n";
            }
            cout << std::endl;
        } else {
            cout << "We don't have any supported glyph data!" << std::endl;
        }
    }

}

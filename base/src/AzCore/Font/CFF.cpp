/*
	File: CFF.cpp
	Author: Philip Haynes
*/

#include "CFF.hpp"
#include "../math.hpp"
#include "../font.hpp"

namespace AzCore {

template <typename T>
String ToString(Array<T> values) {
	String out = "{";
	for (i32 i = 0; i < values.size-1; i++) {
		out += ToString(values[i]);
		out += ", ";
	}
	out += ToString(values.Back());
	out += "}";
	return out;
}

namespace font {
namespace tables {
namespace cffs {

#include "font_cff_std_data.c"

#define ENDIAN_SWAP(in) in = endianSwap((in))

u32 Offset24::value() const {
	return (u32)bytes[2] + ((u32)bytes[1] << 8) + ((u32)bytes[0] << 16);
}

void Offset24::set(u32 val) {
	bytes[2] = val;
	bytes[1] = val >> 8;
	bytes[0] = val >> 16;
}

const char* boolString[2] = {
	"false",
	"true"
};

#define OPERAND_ARITHMETIC(operator) Operand out;               \
switch (type) {                                                 \
	case INTEGER: {                                             \
		switch (other.type) {                                   \
			case INTEGER:                                       \
				out.type = INTEGER;                             \
				out.integer = integer operator other.integer;   \
				break;                                          \
			case REAL:                                          \
				out.type = REAL;                                \
				out.real = (f32)integer operator other.real;    \
				break;                                          \
			default:                                            \
				out.type = INVALID;                             \
		}                                                       \
	} break;                                                    \
	case REAL: {                                                \
		switch (other.type) {                                   \
			case INTEGER:                                       \
				out.type = REAL;                                \
				out.real = real operator (f32)other.integer;    \
				break;                                          \
			case REAL:                                          \
				out.type = REAL;                                \
				out.real = real operator other.real;            \
				break;                                          \
			default:                                            \
				out.type = INVALID;                             \
		}                                                       \
	} break;                                                    \
	default:                                                    \
		out.type = INVALID;                                     \
}                                                               \
return out;                                                     \

Operand Operand::operator+(const Operand &other) const {
	OPERAND_ARITHMETIC(+)
}
Operand Operand::operator-(const Operand &other) const {
	OPERAND_ARITHMETIC(-)
}
Operand Operand::operator*(const Operand &other) const {
	OPERAND_ARITHMETIC(*)
}
Operand Operand::operator/(const Operand &other) const {
	OPERAND_ARITHMETIC(/)
}
#undef OPERAND_ARITHMETIC

bool Operand::operator==(const Operand &other) const {
	switch (type) {
		case INTEGER: {
			switch (other.type) {
				case INTEGER:
					return integer == other.integer;
				case REAL:
					return (f32)integer == other.real;
				default:
					return type == other.type && data == other.data;
			}
		}
		case REAL: {
			switch (other.type) {
				case INTEGER:
					return real == (f32)other.integer;
				case REAL:
					return real == other.real;
				default:
					return type == other.type && data == other.data;
			}
		}
		default:
			return type == other.type && data == other.data;
	}
}

bool Operand::operator>(const Operand &other) const {
	switch (type) {
		case INTEGER: {
			switch (other.type) {
				case INTEGER:
					return integer > other.integer;
				case REAL:
					return (f32)integer > other.real;
				default:
					return false;
			}
		}
		case REAL: {
			switch (other.type) {
				case INTEGER:
					return real > (f32)other.integer;
				case REAL:
					return real > other.real;
				default:
					return false;
			}
		}
		default:
			return false;
	}
}

Operand Operand::operator-() const {
	Operand out;
	switch (type) {
		case INTEGER:
			out.type = INTEGER;
			out.integer = -integer;
			break;
		case REAL:
			out.type = REAL;
			out.real = -real;
			break;
		default:
			out.type = INVALID;
	}
	return out;
}

bool charset_format_any::EndianSwap(Card16 nGlyphs) {
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

void charset_format0::EndianSwap(Card16 nGlyphs) {
	SID *glyph = (SID*)(&format + 1);
	for (Card16 i = 0; i < nGlyphs-1; i++) {
		ENDIAN_SWAP(*glyph);
		glyph++;
	}
}

void charset_range1::EndianSwap() {
	ENDIAN_SWAP(first);
}

void charset_range2::EndianSwap() {
	ENDIAN_SWAP(first);
	ENDIAN_SWAP(nLeft);
#ifdef LOG_VERBOSE
	cout.PrintLn("charset_range2: first = ", (u32)first, ", nLeft = ", nLeft);
#endif
}

void charset_format1::EndianSwap(Card16 nGlyphs) {
	i32 glyphsRemaining = nGlyphs-1;
	cffs::charset_range1 *range = (cffs::charset_range1*)(&format + 1);
	while (glyphsRemaining > 0) {
		range->EndianSwap();
		glyphsRemaining -= range->nLeft+1;
		range++;
	}
}

void charset_format2::EndianSwap(Card16 nGlyphs) {
	i32 glyphsRemaining = nGlyphs-1;
	cffs::charset_range2 *range = (cffs::charset_range2*)(&format + 1);
	while (glyphsRemaining > 0) {
		range->EndianSwap();
		glyphsRemaining -= range->nLeft+1;
		range++;
	}
}

bool FDSelect_any::EndianSwap() {
	if (format == 0) {
#ifdef LOG_VERBOSE
		cout.PrintLn("Format 0");
#endif
	} else if (format == 3) {
#ifdef LOG_VERBOSE
		cout.PrintLn("Format 3");
#endif
		((cffs::FDSelect_format3*)this)->EndianSwap();
	} else {
		error = "Unsupported FDSelect format " + ToString((u16)format);
		return false;
	}
	return true;
}

u32 FDSelect_any::GetFD(char32 glyphIndex, u32 charStringsCount) const {
	if (format == 0) {
		return ((FDSelect_format0*)this)->GetFD(glyphIndex, charStringsCount);
	} else if (format == 3) {
		return ((FDSelect_format3*)this)->GetFD(glyphIndex, charStringsCount);
	} else {
		return 0;
	}
}

u32 FDSelect_format0::GetFD(char32 glyphIndex, u32 nGlyphs) const {
	if (glyphIndex >= nGlyphs) return 0;
	Card8 *fds = (Card8*)this+1;
	return fds[glyphIndex];
}

void FDSelect_format3::EndianSwap() {
	ENDIAN_SWAP(nRanges);
#ifdef LOG_VERBOSE
	cout.PrintLn("nRanges = ", nRanges);
#endif
	cffs::FDSelect_range3 *range = (cffs::FDSelect_range3*)(&format + 3);
	for (u32 i = 0; i < (u32)nRanges; i++) {
		ENDIAN_SWAP(range->first);
		// cout.PrintLn("Range[", i, "].first = ", range->first, ", fd = ", (u32)range->fd);
		range++;
	}
	ENDIAN_SWAP(range->first);
	// cout.PrintLn("Sentinel = ", range->first);
}

u32 FDSelect_format3::GetFD(char32 glyphIndex, u32 nGlyphs) const {
	if (glyphIndex >= nGlyphs) return 0;
	FDSelect_range3 *range = (FDSelect_range3*)(this+1);
	for (i32 i = 0; i < nRanges; i++) {
		if (range[i].first > glyphIndex) return 0;
		if (range[i+1].first > glyphIndex) {
			return range[i].fd;
		}
	}
	return 0;
}

bool index::Parse(char **ptr, u8 **dataStart, Array<u32> *dstOffsets, bool swapEndian) {
	if (swapEndian) {
		ENDIAN_SWAP(count);
	}
	*ptr += 2;
	u32 lastOffset = 1;
	Array<u32> offsets;
	if (count != 0) {
		offsets.Resize(count+1);
#ifdef LOG_VERBOSE
		cout.PrintLn("count = ", count, ", offSize = ", (u32)offSize);
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

//
// void OperandPassover(u8 **data) {
//	 const u8 b0 = **data;
//	 if (b0 >= 32 && b0 <= 246) {
//		 (*data)++;
//	 } else if (b0 >= 247 && b0 <= 254) {
//		 *data += 2;
//	 } else if (b0 == 28) {
//		 *data += 3;
//	 } else if (b0 == 29) {
//		 *data += 5;
//	 } else if (b0 == 30) {
//		 u8 nibbles[2];
//		 do {
//			 (*data)++;
//			 nibbles[0] = (**data) >> 4;
//			 nibbles[1] = (**data) & 0x0f;
//			 for (i32 i = 0; i < 2; i++) {
//				 if (nibbles[i] == 0xf) {
//					 break;
//				 }
//			 }
//		 } while (nibbles[0] != 0xf && nibbles[1] != 0xf);
//		 (*data)++;
//	 } else {
//		 cout.Print("Operand ERROR ", (u16)b0);
//		 (*data)++;
//	 }
// }
//
// String OperandString(u8 **data) {
//	 String out;
//	 const u8 b0 = **data;
//	 if (b0 >= 32 && b0 <= 246) {
//		 out += ToString((i32)b0 - 139);
//		 (*data)++;
//	 } else if (b0 >= 247 && b0 <= 254) {
//		 const u8 b1 = *((*data)+1);
//		 if (b0 < 251) {
//			 out += ToString(((i32)b0 - 247)*256 + (i32)b1 + 108);
//		 } else {
//			 out += ToString(-((i32)b0 - 251)*256 - (i32)b1 - 108);
//		 }
//		 *data += 2;
//	 } else if (b0 == 28) {
//		 const u8 b1 = *((*data)+1);
//		 const u8 b2 = *((*data)+2);
//		 out += ToString(((i16)b1<<8) | ((i32)b2));
//		 *data += 3;
//	 } else if (b0 == 29) {
//		 const u8 b1 = *((*data)+1);
//		 const u8 b2 = *((*data)+2);
//		 const u8 b3 = *((*data)+3);
//		 const u8 b4 = *((*data)+4);
//		 out += ToString(((i32)b1<<24) | ((i32)b2<<16) | ((i32)b3<<8) | ((i32)b4));
//		 *data += 5;
//	 } else if (b0 == 30) {
//		 u8 nibbles[2];
//		 do {
//			 (*data)++;
//			 nibbles[0] = (**data) >> 4;
//			 nibbles[1] = (**data) & 0x0f;
//			 for (i32 i = 0; i < 2; i++) {
//				 if (nibbles[i] < 0xa) {
//					 out.Append(nibbles[i] + '0');
//				 } else if (nibbles[i] == 0xa) {
//					 out.Append('.');
//				 } else if (nibbles[i] == 0xb) {
//					 out.Append('E');
//				 } else if (nibbles[i] == 0xc) {
//					 out.Append("E-");
//				 } else if (nibbles[i] == 0xe) {
//					 out.Append('-');
//				 } else {
//					 break;
//				 }
//			 }
//		 } while (nibbles[0] != 0xf && nibbles[1] != 0xf);
//		 (*data)++;
//	 } else {
//		 out += "Operand ERROR " + ToString((u16)b0);
//		 (*data)++;
//	 }
//	 return out;
// }

i32 GetDictOperand(u8 *data, Operand *dst) {
	u8 b0 = data[0];
	if (b0 >= 32 && b0 <= 246) {
		dst->type = Operand::INTEGER;
		dst->integer = (i32)b0 - 139;
		return 1;
	} else if (b0 >= 247 && b0 <= 254) {
		const u8 b1 = data[1];
		dst->type = Operand::INTEGER;
		if (b0 < 251) {
			dst->integer = ((i32)b0 - 247)*256 + (i32)b1 + 108;
		} else {
			dst->integer = -((i32)b0 - 251)*256 - (i32)b1 - 108;
		}
		return 2;
	} else if (b0 == 28) {
		const u8 b1 = data[1];
		const u8 b2 = data[2];
		dst->type = Operand::INTEGER;
		dst->integer = ((i32)b1<<8) | ((i32)b2);
		return 3;
	} else if (b0 == 29) {
		const u8 b1 = data[1];
		const u8 b2 = data[2];
		const u8 b3 = data[3];
		const u8 b4 = data[4];
		dst->type = Operand::INTEGER;
		dst->integer = ((i32)b1<<24) | ((i32)b2<<16) | ((i32)b3<<8) | ((i32)b4);
		return 5;
	} else if (b0 == 30) {
		u8 nibbles[2];
		i32 dec = -1;
		bool expPos = false;
		bool expNeg = false;
		i32 exponent = 0;
		bool negative = false;
		i32 i = 0;
		f64 out = 0.0;
		do {
			i++;
			nibbles[0] = data[i] >> 4;
			nibbles[1] = data[i] & 0x0f;
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
				out /= 10.0;
			}
		} else if (exponent > 0) {
			for (i32 i = exponent; i > 0; i--) {
				out *= 10.0;
			}
		}
		if (negative) {
			out *= -1.0;
		}
		dst->type = Operand::REAL;
		dst->real = (f32)out;
		return i+1;
	} else {
		cout.Print("Operand ERROR ", (u16)b0);
		return 1;
	}
}

i32 GetType2Operand(u8 *data, Operand *dst) {
	u8 b0 = data[0];
	if (b0 >= 32 && b0 <= 246) {
		dst->type = Operand::INTEGER;
		dst->integer = (i32)b0 - 139;
		return 1;
	} else if (b0 >= 247 && b0 <= 254) {
		const u8 b1 = data[1];
		dst->type = Operand::INTEGER;
		if (b0 < 251) {
			dst->integer = ((i32)b0 - 247)*256 + (i32)b1 + 108;
		} else {
			dst->integer = -((i32)b0 - 251)*256 - (i32)b1 - 108;
		}
		return 2;
	} else if (b0 == 255) {
		Fixed_t fixed = bytesToFixed((char*)data+1, SysEndian.little);
		dst->type = Operand::REAL;
		dst->real = ToF32(fixed);
		return 5;
	} else if (b0 == 28) {
		const u8 b1 = data[1];
		const u8 b2 = data[2];
		dst->type = Operand::INTEGER;
		dst->integer = ((i32)b1<<8) | ((i32)b2);
		return 3;
	} else {
		cout.Print("Operand ERROR ", (u16)b0);
		return 1;
	}
}

i32 DictOperatorResolution(u8 *data, OperandStack &stack, String &out) {
	u8 op1 = data[0];
	if (op1 <= 21) {
		// We're an operator
		switch (op1) {
			case 12: {
				// We're a two-byte operator
				u8 op2 = data[1];
				switch (op2) {
					// Private DICT
					case 9: {
						out += "BlueScale = " + ToString(stack.Pop().ToF32());
					} break;
					case 10: {
						out += "BlueShift = " + ToString(stack.Pop().ToF32());
					} break;
					case 11: {
						out += "BlueFuzz = " + ToString(stack.Pop().ToF32());
					} break;
					case 12: {
						out += "StemSnapH = " + ToString(stack.DictDeltaF32());
					} break;
					case 13: {
						out += "StemSnapV = " + ToString(stack.DictDeltaF32());
					} break;
					case 14: {
						// This should come in as an integer, which translates directly to bool
						out += "ForceBold = " + String(boolString[stack.Pop().boolean]);
					} break;
					case 17: {
						out += "LanguageGroup = " + ToString(stack.Pop().ToI32());
					} break;
					case 18: {
						out += "ExpansionFactor = " + ToString(stack.Pop().ToF32());
					} break;
					case 19: {
						out += "initialRandomSeed = " + ToString(stack.Pop().ToI32());
					} break;

					// Top DICT

					case 0: {
						out += "Copyright = " + ToString((SID)stack.Pop().ToI32());
					} break;
					case 1: {
						out += "isFixedPitch = " + String(boolString[stack.Pop().boolean]);
					} break;
					case 2: {
						out += "ItalicAngle = " + ToString(stack.Pop().ToI32());
					} break;
					case 3: {
						out += "UnderlinePosition = " + ToString(stack.Pop().ToI32());
					} break;
					case 4: {
						out += "UnderlineThickness = " + ToString(stack.Pop().ToI32());
					} break;
					case 5: {
						out += "PaintType = " + ToString(stack.Pop().ToI32());
					} break;
					case 6: {
						out += "CharstringType = " + ToString(stack.Pop().ToI32());
					} break;
					case 7: {
						out += "FontMatrix = " + ToString(stack.DictArrayF32());
					} break;
					case 8: {
						out += "StrokeWidth = " + ToString(stack.Pop().ToF32());
					} break;
					case 20: {
						out += "SyntheticBase = " + ToString(stack.Pop().ToI32());
					} break;
					case 21: {
						out += "PostScript = " + ToString((SID)stack.Pop().ToI32());
					} break;
					case 22: {
						out += "BaseFontName = " + ToString((SID)stack.Pop().ToI32());
					} break;
					case 23: {
						out += "BaseFontBlend = " + ToString(stack.DictDeltaI32());
					} break;

					// CIDFont-only Operators

					case 30: {
						out += "ROS.supplement = " + ToString(stack.Pop().ToI32());
						out += "\nROS.ordering = " + ToString((SID)stack.Pop().ToI32());
						out += "\nROS.registry = " + ToString((SID)stack.Pop().ToI32());
					} break;
					case 31: {
						out += "CIDFontVersion = " + ToString(stack.Pop().ToF32());
					} break;
					case 32: {
						out += "CIDFontRevision = " + ToString(stack.Pop().ToF32());
					} break;
					case 33: {
						out += "CIDFontType = " + ToString(stack.Pop().ToI32());
					} break;
					case 34: {
						out += "CIDCount = " + ToString(stack.Pop().ToI32());
					} break;
					case 35: {
						out += "UIDBase = " + ToString(stack.Pop().ToI32());
					} break;
					case 36: {
						out += "FDArray = " + ToString(stack.Pop().ToI32());
					} break;
					case 37: {
						out += "FDSelect = " + ToString(stack.Pop().ToI32());
					} break;
					case 38: {
						out += "FontName = " + ToString((SID)stack.Pop().ToI32());
					} break;
					default: {
						cout.PrintLn("Operator Error (12 ", op2, ")");
					}
				}
				return 2;
			} break;

			// Private DICT

			case 6: {
				out += "BlueValues = " + ToString(stack.DictDeltaI32());
			} break;
			case 7: {
				out += "OtherBlues = " + ToString(stack.DictDeltaI32());
			} break;
			case 8: {
				out += "FamilyBlues = " + ToString(stack.DictDeltaI32());
			} break;
			case 9: {
				out += "FamilyOtherBlues = " + ToString(stack.DictDeltaI32());
			} break;
			case 10: {
				out += "StdHW = " + ToString(stack.Pop().ToF32());
			} break;
			case 11: {
				out += "StdVW = " + ToString(stack.Pop().ToF32());
			} break;
			case 19: {
				out += "Subrs = " + ToString(stack.Pop().ToI32());
			} break;
			case 20: {
				out += "defaultWidthX = " + ToString(stack.Pop().ToI32());
			} break;
			case 21: {
				out += "nominalWidthX = " + ToString(stack.Pop().ToI32());
			} break;

			// Top DICT

			case 0: {
				out += "version = " + ToString((SID)stack.Pop().ToI32());
			} break;
			case 1: {
				out += "Notice = " + ToString((SID)stack.Pop().ToI32());
			} break;
			case 2: {
				out += "FullName = " + ToString((SID)stack.Pop().ToI32());
			} break;
			case 3: {
				out += "FamilyName = " + ToString((SID)stack.Pop().ToI32());
			} break;
			case 4: {
				out += "Weight = " + ToString((SID)stack.Pop().ToI32());
			} break;
			case 13: {
				out += "UniqueID = " + ToString(stack.Pop().ToI32());
			} break;
			case 5: {
				out += "FontBBox = " + ToString(stack.DictArrayI32());
			} break;
			case 14: {
				out += "XUID = " + ToString(stack.DictArrayI32());
			} break;
			case 15: {
				out += "charset = " + ToString(stack.Pop().ToI32());
			} break;
			case 16: {
				out += "Encoding = " + ToString(stack.Pop().ToI32());
			} break;
			case 17: {
				out += "CharStrings = " + ToString(stack.Pop().ToI32());
			} break;
			case 18: {
				out += "Private.offset = " + ToString(stack.Pop().ToI32());
				out += "\nPrivate.size = " + ToString(stack.Pop().ToI32());
			} break;
		}
		return 1;
	} else if (!(op1 == 31 || op1 == 255 || (op1 <= 27 && op1 >= 22))) {
		// We're an operand
		Operand operand;
		i32 count = GetDictOperand(data, &operand);
		stack.Push(operand);
		return count;
	} else {
		// We're an invalid operator
		cout.PrintLn("Invalid Operator (", (i32)op1, ")");
		return 1;
	}
}

String DictCharString(u8 *start, u8 *end) {
	String out;
	out.Reserve(i32(end-start));
	OperandStack stack;
	while (start < end) {
		const u8 b0 = *start;
		if (b0 <= 21) {
			start += DictOperatorResolution(start, stack, out);
			out += "\n";
		} else if (!(b0 == 31 || b0 == 255 || (b0 <= 27 && b0 >= 22))) {
			Operand operand;
			i32 count = GetDictOperand(start, &operand);
			stack.Push(operand);
			start += count;
		} else {
			out += "ERROR #" + ToString((u16)b0);
			start++;
		}
	}
	return out;
}

struct Type2ParsingInfo {
	OperandStack stack;
	vec2 point = 0.0f;
	vec2 pathStart;
	i32 numHints = 0;
	RandomNumberGenerator rng;
	Array<Operand> transientArray;
	dict dictValues;
	u8 *subrData;
	Array<u32> subrOffsets;
	u8 *gsubrData;
	const Array<u32> *gsubrOffsets;
	bool ret = false;
	bool path = false;
	bool first = true;
};

i32 ResolveType2Operator(u8 *data, Type2ParsingInfo &info, Glyph &out) {
	u8 op1 = data[0];
	if (op1 <= 31 && op1 != 28) {
		// Operator
		if (info.first) {
			switch (op1) {
				case 1:
				case 3:
				case 18:
				case 23:
				case 19:
				case 20:
				case 21:
				case 14:
					// even arguments = no width
					if (info.stack.data.size & 1) {
						info.stack.data.Erase(0);
					}
					break;
				case 22:
				case 4:
					// odd arguments = no width
					if (!(info.stack.data.size & 1) && info.stack.data.size > 0) {
						info.stack.data.Erase(0);
					}
					break;
			}
			info.first = false;
		}
		switch (op1) {

			// We don't care about hints

			case  1: // |- y dy {dya dyb}* hstem |-
			case  3: // |- x dx {dxa dxb}* vstem |-
			case 18: // |- y dy {dya dyb}* hstemhm |-
			case 23: // |- x dx {dxa dxb}* vstemhm |-
				info.numHints += info.stack.data.size / 2;
				info.stack.Clear();
				return 1;
			case 19: // |- hintmask |-
			case 20: { // |- cntrmask |-
				info.numHints += info.stack.data.size / 2; // In case there's an implicit stem hint.
				info.stack.Clear();
				i32 maskBytes = (info.numHints+7)/8;
				return maskBytes+1;
			}
			// Two-byte operators
			case 12: {
				u8 op2 = data[1];
				switch (op2) {

					// Path construction

					case 35: { // |- dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 dx6 dy6 fd flex |-
						// Operand fd = info.stack.Pop();
						for (i32 i = 0; i < info.stack.data.size-5; i += 6) {
							Operand dx1 = info.stack.data[i];
							Operand dy1 = info.stack.data[i+1];
							Operand dx2 = info.stack.data[i+2];
							Operand dy2 = info.stack.data[i+3];
							Operand dx3 = info.stack.data[i+4];
							Operand dy3 = info.stack.data[i+5];
							vec2 point2 = info.point + vec2(dx1.ToF32(), dy1.ToF32());
							vec2 point3 = point2 + vec2(dx2.ToF32(), dy2.ToF32());
							vec2 point4 = point3 + vec2(dx3.ToF32(), dy3.ToF32());
							out.curve2s.Append({info.point, point2, point3, point4});
							info.point = point4;
						}
						info.stack.Clear();
						return 2;
					}
					case 34: { // |- dx1 dx2 dy2 dx3 dx4 dx5 dx6 hflex |-
						Operand dx1 = info.stack.data[0];
						Operand dx2 = info.stack.data[1];
						Operand dy2 = info.stack.data[2];
						Operand dx3 = info.stack.data[3];

						Operand dx4 = info.stack.data[4];
						Operand dx5 = info.stack.data[5];
						Operand dx6 = info.stack.data[6];
						vec2 point2 = info.point + vec2(dx1.ToF32(), 0.0f);
						vec2 point3 = point2 + vec2(dx2.ToF32(), dy2.ToF32());
						vec2 point4 = point3 + vec2(dx3.ToF32(), 0.0f);
						out.curve2s.Append({info.point, point2, point3, point4});
						info.point = point4;

						point2 = info.point + vec2(dx4.ToF32(), 0.0f);
						point3 = point2 + vec2(dx5.ToF32(), -dy2.ToF32());
						point4 = point3 + vec2(dx6.ToF32(), 0.0f);
						out.curve2s.Append({info.point, point2, point3, point4});
						info.point = point4;

						info.stack.Clear();
						return 2;
					}
					case 36: { // |- dx1 dy1 dx2 dy2 dx3 dx4 dx5 dy5 dx6 hflex1 |-
						Operand dx1 = info.stack.data[0];
						Operand dy1 = info.stack.data[1];
						Operand dx2 = info.stack.data[2];
						Operand dy2 = info.stack.data[3];
						Operand dx3 = info.stack.data[4];
						//	  dy3 = 0.0f
						Operand dx4 = info.stack.data[5];
						//	  dy4 = 0.0f
						Operand dx5 = info.stack.data[6];
						Operand dy5 = info.stack.data[7];
						Operand dx6 = info.stack.data[8];

						f32 lastY = info.point.y;

						vec2 point2 = info.point + vec2(dx1.ToF32(), dy1.ToF32());
						vec2 point3 = point2 + vec2(dx2.ToF32(), dy2.ToF32());
						vec2 point4 = point3 + vec2(dx3.ToF32(), 0.0f);
						out.curve2s.Append({info.point, point2, point3, point4});
						info.point = point4;

						point2 = info.point + vec2(dx4.ToF32(), 0.0f);
						point3 = point2 + vec2(dx5.ToF32(), dy5.ToF32());
						point4 = vec2(point3.x + dx6.ToF32(), lastY);
						out.curve2s.Append({info.point, point2, point3, point4});
						info.point = point4;

						info.stack.Clear();
						return 2;
					}
					case 37: { // |- dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 d6 flex1 |-
						Operand dx1 = info.stack.data[0];
						Operand dy1 = info.stack.data[1];
						Operand dx2 = info.stack.data[2];
						Operand dy2 = info.stack.data[3];
						Operand dx3 = info.stack.data[4];
						Operand dy3 = info.stack.data[5];
						Operand dx4 = info.stack.data[6];
						Operand dy4 = info.stack.data[7];
						Operand dx5 = info.stack.data[8];
						Operand dy5 = info.stack.data[9];
						Operand d6 = info.stack.data[10];

						vec2 start = info.point;

						vec2 point2 = start + vec2(dx1.ToF32(), dy1.ToF32());
						vec2 point3 = point2 + vec2(dx2.ToF32(), dy2.ToF32());
						vec2 point4 = point3 + vec2(dx3.ToF32(), dy3.ToF32());
						out.curve2s.Append({start, point2, point3, point4});
						info.point = point4;

						point2 = info.point + vec2(dx4.ToF32(), dy4.ToF32());
						point3 = point2 + vec2(dx5.ToF32(), dy5.ToF32());
						vec2 dxdy = point3 - start;
						if (abs(dxdy.x) > abs(dxdy.y)) {
							point4 = vec2(point3.x + d6.ToF32(), start.y);
						} else {
							point4 = vec2(start.x, point3.y + d6.ToF32());
						}
						out.curve2s.Append({info.point, point2, point3, point4});
						info.point = point4;
					}

					// Arithmetic

					case 9: { // num abs num2
						info.stack.Push(abs(info.stack.Pop()));
						return 2;
					}
					case 10: { // num1 num2 add sum
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(num1 + num2);
						return 2;
					}
					case 11: { // num1 num2 sub difference
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(num1 - num2);
						return 2;
					}
					case 12: { // num1 num2 div quotient
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(num1 / num2);
						return 2;
					}
					case 14: { // num neg num2
						info.stack.Push(-info.stack.Pop());
						return 2;
					}
					case 23: { // random num2
						f32 val;
						do {
							val = random(0.0f, 1.0f, &info.rng);
						} while (val == 0.0f);
						info.stack.Push(Operand(val));
						return 2;
					}
					case 24: { // num1 num2 mul product
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(num1 * num2);
						return 2;
					}
					case 26: { // num sqrt num2
						Operand num = info.stack.Pop();
						info.stack.Push(Operand(sqrt(num.ToF32())));
						return 2;
					}
					case 18: // num drop
						info.stack.Pop();
						return 2;
					case 28: { // num1 num2 exch num2 num1
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(num2);
						info.stack.Push(num1);
						return 2;
					}
					case 29: { // numX ... num0 i index numX ... num0 numi
						Operand i = info.stack.Pop();
						if (i.type != Operand::INTEGER) {
							i.type = Operand::INVALID;
							info.stack.Push(i);
						} else {
							if (i.integer < 0) i.integer = 0;
							info.stack.Push(info.stack[i.integer]);
						}
						return 2;
					}
					case 30: { // num(N-1) ... num0 N J roll num((J-1) mod N) ... num0 num(N-1) ... num(J mod N)
						Operand J = info.stack.Pop();
						Operand N = info.stack.Pop();
						// NOTE: I'm sure there's a better way to do this but does it really matter?
						Array<Operand> nums(N.integer);
						for (i32 i = 0; i < nums.size; i++) {
							nums[i] = info.stack.Pop();
						}
						for (i32 i = (J.integer-1) % N.integer; i >= 0; i--) {
							info.stack.Push(nums[i]);
						}
						for (i32 i = (N.integer-1); i >= (J.integer % N.integer); i--) {
							info.stack.Push(nums[i]);
						}
						return 2;
					}
					case 27: // any dup any any
						info.stack.Push(info.stack[0]);
						return 2;

					// Storage

					case 20: { // val i put
						Operand i = info.stack.Pop();
						Operand val = info.stack.Pop();
						if (i.integer >= info.transientArray.size) {
							info.transientArray.Resize(i.integer);
						}
						info.transientArray[i.integer] = val;
						return 2;
					}
					case 21: { // i get val
						Operand i = info.stack.Pop();
						info.stack.Push(info.transientArray[i.integer]);
						return 2;
					}

					// Conditional

					case 3: { // num1 num2 and 1_or_0
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(Operand(num1.integer && num2.integer));
						return 2;
					}
					case 4: { // num1 num2 or 1_or_0
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(Operand(num1.integer || num2.integer));
						return 2;
					}
					case 5: { // num1 not 1_or_0
						Operand num1 = info.stack.Pop();
						info.stack.Push(Operand(!num1.integer));
						return 2;
					}
					case 15: { // num1 num2 eq 1_or_0
						Operand num2 = info.stack.Pop();
						Operand num1 = info.stack.Pop();
						info.stack.Push(Operand(num1 == num2));
						return 2;
					}
					case 22: { // s1 s2 v1 v2 ifelse s1_or_s2
						Operand v2 = info.stack.Pop();
						Operand v1 = info.stack.Pop();
						Operand s2 = info.stack.Pop();
						Operand s1 = info.stack.Pop();
						info.stack.Push(v1 > v2 ? s2 : s1);
						return 2;
					}
					default:
						cout.PrintLn("Type2 operator error(12 ", (i32)op2, ")");
						return 2;
				}
			} break;

			// Path construction

			case 21: { // |- dx1 dy1 rmoveto |-
				if (info.path) {
					if (info.point != info.pathStart) {
						out.lines.Append({info.point, info.pathStart});
					}
				}
				Operand dx1 = info.stack.data[0];
				Operand dy1 = info.stack.data[1];
				info.point += vec2(dx1.ToF32(), dy1.ToF32());
				info.pathStart = info.point;
				info.path = true;
				info.stack.Clear();
				return 1;
			}
			case 22: { // |- dx1 hmoveto |-
				if (info.path) {
					if (info.point != info.pathStart) {
						out.lines.Append({info.point, info.pathStart});
					}
				}
				Operand dx1 = info.stack.data[0];
				info.point.x += dx1.ToF32();
				info.pathStart = info.point;
				info.path = true;
				info.stack.Clear();
				return 1;
			}
			case 4: { // |- dy1 vmoveto |-
				if (info.path) {
					if (info.point != info.pathStart) {
						out.lines.Append({info.point, info.pathStart});
					}
				}
				Operand dy1 = info.stack.data[0];
				info.point.y += dy1.ToF32();
				info.pathStart = info.point;
				info.path = true;
				info.stack.Clear();
				return 1;
			}
			case 25:  // |- {dxa dya}+ dxb dyb dxc dyc dxd dyd rlinecurve |-
			case 5: { // |- {dxa dya}+ rlineto |-
				i32 i = 0;
				for (; i < info.stack.data.size-(op1 == 25 ? 7 : 1); i+=2) {
					Operand dxa = info.stack.data[i];
					Operand dya = info.stack.data[i+1];
					vec2 point2 = info.point + vec2(dxa.ToF32(), dya.ToF32());
					out.lines.Append({info.point, point2});
					info.point = point2;
				}
				if (op1 == 25) {
					Operand dxb = info.stack.data[i];
					Operand dyb = info.stack.data[i+1];
					Operand dxc = info.stack.data[i+2];
					Operand dyc = info.stack.data[i+3];
					Operand dxd = info.stack.data[i+4];
					Operand dyd = info.stack.data[i+5];
					vec2 point2 = info.point + vec2(dxb.ToF32(), dyb.ToF32());
					vec2 point3 = point2 + vec2(dxc.ToF32(), dyc.ToF32());
					vec2 point4 = point3 + vec2(dxd.ToF32(), dyd.ToF32());
					out.curve2s.Append({info.point, point2, point3, point4});
					info.point = point4;
				}
				info.stack.Clear();
				return 1;
			}
			case 6: {
				// |- dx1 {dya dxb}* hlineto |-
				// |- {dxa dyb}+ hlineto |-
				for (i32 i = 0; i < info.stack.data.size; i++) {
					Operand d = info.stack.data[i];
					vec2 point2 = info.point;
					if (i & 1) {
						point2.y += d.ToF32();
					} else {
						point2.x += d.ToF32();
					}
					out.lines.Append({info.point, point2});
					info.point = point2;
				}
				info.stack.Clear();
				return 1;
			}
			case 7: {
				// |- dy1 {dxa dyb}* vlineto |-
				// |- {dya dxb}+ vlineto |-
				for (i32 i = 0; i < info.stack.data.size; i++) {
					Operand d = info.stack.data[i];
					vec2 point2 = info.point;
					if (i & 1) {
						point2.x += d.ToF32();
					} else {
						point2.y += d.ToF32();
					}
					out.lines.Append({info.point, point2});
					info.point = point2;
				}
				info.stack.Clear();
				return 1;
			}
			case 24:  // |- {dxa dya dxb dyb dxc dyc}+ dxd dyd rcurveline |-
			case 8: { // |- {dxa dya dxb dyb dxc dyc}+ rrcurveto |-
				i32 i = 0;
				for (; i < info.stack.data.size-5; i+=6) {
					Operand dxa = info.stack.data[i];
					Operand dya = info.stack.data[i+1];
					Operand dxb = info.stack.data[i+2];
					Operand dyb = info.stack.data[i+3];
					Operand dxc = info.stack.data[i+4];
					Operand dyc = info.stack.data[i+5];
					vec2 point2 = info.point + vec2(dxa.ToF32(), dya.ToF32());
					vec2 point3 = point2 + vec2(dxb.ToF32(), dyb.ToF32());
					vec2 point4 = point3 + vec2(dxc.ToF32(), dyc.ToF32());
					out.curve2s.Append({info.point, point2, point3, point4});
					info.point = point4;
				}
				if (op1 == 24) {
					Operand dxd = info.stack.data[i];
					Operand dyd = info.stack.data[i+1];
					vec2 point2 = info.point + vec2(dxd.ToF32(), dyd.ToF32());
					out.lines.Append({info.point, point2});
					info.point = point2;
				}
				info.stack.Clear();
				return 1;
			}
			case 26: { // |- dx1? {dya dxb dyb dyc}+ vvcurveto |-
				Operand dx1;
				i32 i;
				if (info.stack.data.size & 1) {
					dx1 = info.stack.data[0];
					i = 1;
				} else {
					dx1 = Operand(0.0f);
					i = 0;
				}
				for (; i < info.stack.data.size-3; i+=4) {
					Operand dya = info.stack.data[i];
					// dxa = dx1 or 0.0f
					Operand dxb = info.stack.data[i+1];
					Operand dyb = info.stack.data[i+2];
					Operand dyc = info.stack.data[i+3];
					// dxc = 0.0f
					vec2 point2 = info.point + vec2(dx1.ToF32(), dya.ToF32());
					vec2 point3 = point2 + vec2(dxb.ToF32(), dyb.ToF32());
					vec2 point4 = point3 + vec2(0.0f, dyc.ToF32());
					out.curve2s.Append({info.point, point2, point3, point4});
					info.point = point4;
					dx1 = Operand(0.0f);
				}
				info.stack.Clear();
				return 1;
			}
			case 27: { // |- dy1? {dxa dxb dyb dxc}+ hhcurveto |-
				Operand dy1;
				i32 i;
				if (info.stack.data.size & 1) {
					dy1 = info.stack.data[0];
					i = 1;
				} else {
					dy1 = Operand(0.0f);
					i = 0;
				}
				for (; i < info.stack.data.size-3; i+=4) {
					Operand dxa = info.stack.data[i];
					// dya = dy1 or 0.0f
					Operand dxb = info.stack.data[i+1];
					Operand dyb = info.stack.data[i+2];
					Operand dxc = info.stack.data[i+3];
					// dyc = 0.0f
					vec2 point2 = info.point + vec2(dxa.ToF32(), dy1.ToF32());
					vec2 point3 = point2 + vec2(dxb.ToF32(), dyb.ToF32());
					vec2 point4 = point3 + vec2(dxc.ToF32(), 0.0f);
					out.curve2s.Append({info.point, point2, point3, point4});
					info.point = point4;
					dy1 = Operand(0.0f);
				}
				info.stack.Clear();
				return 1;
			}
			case 30:  // |- same as below, but starting vertical vhcurveto |-
			case 31: {
				// |- dx1 dx2 dy2 dy3 {dya dxb dyb dxc dxd dxe dye dyf}* dxf? hvcurveto |-
				// |- {dxa dxb dyb dyc dyd dxe dye dxf}+ dyf? hvcurveto |-
				Operand end;
				if (info.stack.data.size & 1) {
					end = info.stack.Pop();
				} else {
					end = Operand(0.0f);
				}
				bool startHorizontal = op1 == 31;
				for (i32 i = 0; i < info.stack.data.size-3; i+=4) {
					Operand d1 = info.stack.data[i];
					Operand d2 = info.stack.data[i+1];
					Operand d3 = info.stack.data[i+2];
					Operand d4 = info.stack.data[i+3];
					vec2 point2, point3, point4;
					if (startHorizontal) {
						point2 = info.point + vec2(d1.ToF32(), 0.0f);
						point3 = point2 + vec2(d2.ToF32(), d3.ToF32());
						point4 = point3 + vec2(0.0f, d4.ToF32());
						if (i+4 >= info.stack.data.size) {
							point4.x += end.ToF32();
						}
						startHorizontal = false;
					} else {
						point2 = info.point + vec2(0.0f, d1.ToF32());
						point3 = point2 + vec2(d2.ToF32(), d3.ToF32());
						point4 = point3 + vec2(d4.ToF32(), 0.0f);
						if (i+4 >= info.stack.data.size) {
							point4.y += end.ToF32();
						}
						startHorizontal = true;
					}
					out.curve2s.Append({info.point, point2, point3, point4});
					info.point = point4;
				}
				info.stack.Clear();
				return 1;
			}

			case 14: { // - endchar |-
				if (info.path) {
					if (info.point != info.pathStart) {
						out.lines.Append({info.point, info.pathStart});
					}
				}
				info.stack.Clear();
				return 1;
			}

			// Subroutines

			case 10: { // subr# callsubr -
				Operand num = info.stack.Pop();
				i32 bias;
				if (info.subrOffsets.size < 1240) {
					bias = 107;
				} else if (info.subrOffsets.size < 33900) {
					bias = 1131;
				} else {
					bias = 32768;
				}
				num.integer += bias;
				u8 *start = info.subrData + info.subrOffsets[num.integer];
				u8 *end = info.subrData + info.subrOffsets[num.integer+1];
				while (start < end) {
					start += ResolveType2Operator(start, info, out);
					if (info.ret) {
						info.ret = false;
						break;
					}
				}
				return 1;
			}
			case 29: { // globalsubr# callgsubr -
				Operand num = info.stack.Pop();
				i32 bias;
				if (info.gsubrOffsets->size < 1240) {
					bias = 107;
				} else if (info.gsubrOffsets->size < 33900) {
					bias = 1131;
				} else {
					bias = 32768;
				}
				num.integer += bias;
				u8 *start = info.gsubrData + (*info.gsubrOffsets)[num.integer];
				u8 *end = info.gsubrData + (*info.gsubrOffsets)[num.integer+1];
				while (start < end) {
					start += ResolveType2Operator(start, info, out);
					if (info.ret) {
						info.ret = false;
						break;
					}
				}
				return 1;
			}
			case 11: // - return -
				info.ret = true;
				return 1;
			default:
				cout.PrintLn("Type2 operator error (", (i32)op1, ")");
				return 1;
		}
	} else {
		// Operand
		Operand operand;
		i32 count = GetType2Operand(data, &operand);
		info.stack.Push(operand);
		return count;
	}
}

Glyph GlyphFromType2CharString(u8 *data, u32 size, Type2ParsingInfo &info) {
	Glyph out;
	for (u8 *end = data+size; end > data;) {
		data += ResolveType2Operator(data, info, out);
	}
	return out;
}

void dict::ParseCharString(u8 *data, u32 size) {
	OperandStack stack;

	for (u8 *end = data+size; end > data;) {
		data += ResolveOperator(data, stack);
	}
}

i32 dict::ResolveOperator(u8 *data, OperandStack &stack) {
	u8 op1 = data[0];
	if (op1 <= 21) {
		// We're an operator
		switch (op1) {
			case 12: {
				// We're a two-byte operator
				u8 op2 = data[1];
				switch (op2) {
					// Private DICT
					case 9: {
						BlueScale = stack.Pop().ToF32();
					} break;
					case 10: {
						BlueShift = stack.Pop().ToF32();
					} break;
					case 11: {
						BlueFuzz = stack.Pop().ToF32();
					} break;
					case 12: {
						StemSnapH = stack.DictDeltaF32();
					} break;
					case 13: {
						StemSnapV = stack.DictDeltaF32();
					} break;
					case 14: {
						// This should come in as an integer, which translates directly to bool
						ForceBold = stack.Pop().boolean;
					} break;
					case 17: {
						LanguageGroup = stack.Pop().ToI32();
					} break;
					case 18: {
						ExpansionFactor = stack.Pop().ToF32();
					} break;
					case 19: {
						initialRandomSeed = stack.Pop().ToI32();
					} break;

					// Top DICT

					case 0: {
						Copyright = (SID)stack.Pop().ToI32();
					} break;
					case 1: {
						isFixedPitch = stack.Pop().boolean;
					} break;
					case 2: {
						ItalicAngle = stack.Pop().ToI32();
					} break;
					case 3: {
						UnderlinePosition = stack.Pop().ToI32();
					} break;
					case 4: {
						UnderlineThickness = stack.Pop().ToI32();
					} break;
					case 5: {
						PaintType = stack.Pop().ToI32();
					} break;
					case 6: {
						CharstringType = stack.Pop().ToI32();
					} break;
					case 7: {
						FontMatrix = stack.DictArrayF32();
					} break;
					case 8: {
						StrokeWidth = stack.Pop().ToF32();
					} break;
					case 20: {
						SyntheticBase = stack.Pop().ToI32();
					} break;
					case 21: {
						PostScript = (SID)stack.Pop().ToI32();
					} break;
					case 22: {
						BaseFontName = (SID)stack.Pop().ToI32();
					} break;
					case 23: {
						BaseFontBlend = stack.DictDeltaI32();
					} break;

					// CIDFont-only Operators

					case 30: {
						ROS.supplement = stack.Pop().ToI32();
						ROS.ordering = (SID)stack.Pop().ToI32();
						ROS.registry = (SID)stack.Pop().ToI32();
					} break;
					case 31: {
						CIDFontVersion = stack.Pop().ToF32();
					} break;
					case 32: {
						CIDFontRevision = stack.Pop().ToF32();
					} break;
					case 33: {
						CIDFontType = stack.Pop().ToI32();
					} break;
					case 34: {
						CIDCount = stack.Pop().ToI32();
					} break;
					case 35: {
						UIDBase = stack.Pop().ToI32();
					} break;
					case 36: {
						FDArray = stack.Pop().ToI32();
					} break;
					case 37: {
						FDSelect = stack.Pop().ToI32();
					} break;
					case 38: {
						FontName = (SID)stack.Pop().ToI32();
					} break;
					default: {
						cout.PrintLn("Operator Error (12 ", op2, ")");
					}
				}
				return 2;
			} break;

			// Private DICT

			case 6: {
				BlueValues = stack.DictDeltaI32();
			} break;
			case 7: {
				OtherBlues = stack.DictDeltaI32();
			} break;
			case 8: {
				FamilyBlues = stack.DictDeltaI32();
			} break;
			case 9: {
				FamilyOtherBlues = stack.DictDeltaI32();
			} break;
			case 10: {
				StdHW = stack.Pop().ToF32();
			} break;
			case 11: {
				StdVW = stack.Pop().ToF32();
			} break;
			case 19: {
				Subrs = stack.Pop().ToI32();
			} break;
			case 20: {
				defaultWidthX = stack.Pop().ToI32();
			} break;
			case 21: {
				nominalWidthX = stack.Pop().ToI32();
			} break;

			// Top DICT

			case 0: {
				version = (SID)stack.Pop().ToI32();
			} break;
			case 1: {
				Notice = (SID)stack.Pop().ToI32();
			} break;
			case 2: {
				FullName = (SID)stack.Pop().ToI32();
			} break;
			case 3: {
				FamilyName = (SID)stack.Pop().ToI32();
			} break;
			case 4: {
				Weight = (SID)stack.Pop().ToI32();
			} break;
			case 13: {
				UniqueID = stack.Pop().ToI32();
			} break;
			case 5: {
				FontBBox = stack.DictArrayI32();
			} break;
			case 14: {
				XUID = stack.DictArrayI32();
			} break;
			case 15: {
				charset = stack.Pop().ToI32();
			} break;
			case 16: {
				Encoding = stack.Pop().ToI32();
			} break;
			case 17: {
				CharStrings = stack.Pop().ToI32();
			} break;
			case 18: {
				Private.offset = stack.Pop().ToI32();
				Private.size = stack.Pop().ToI32();
			} break;
		}
		return 1;
	} else if (!(op1 == 31 || op1 == 255 || (op1 <= 27 && op1 >= 22))) {
		// We're an operand
		Operand operand;
		i32 count = GetDictOperand(data, &operand);
		stack.Push(operand);
		return count;
	} else {
		// We're an invalid operator
		cout.PrintLn("Invalid Operator (", (i32)op1, ")");
		return 1;
	}
}

} // namespace cffs

bool cff::Parse(cffParsed *parsed, bool swapEndian) {
	parsed->cffData = this;
	parsed->active = true;
	char *ptr = (char*)this + header.size;

	//
	//				  nameIndex
	//

	parsed->nameIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
	cout.PrintLn("nameIndex:");
#endif
	if (!parsed->nameIndex->Parse(&ptr, &parsed->nameIndexData, &parsed->nameIndexOffsets, swapEndian)) {
		error = "nameIndex: " + error;
		return false;
	}
#ifdef LOG_VERBOSE
	cout.PrintLn("nameIndex data:");
	for (i32 i = 0; i < parsed->nameIndexOffsets.size-1; i++) {
		String string(parsed->nameIndexOffsets[i+1] - parsed->nameIndexOffsets[i]);
		memcpy(string.data, parsed->nameIndexData + parsed->nameIndexOffsets[i], string.size);
		cout.Print("[", i, "]=\"", string, "\" ");
	}
	cout.Print(std::endl);
#endif
	if (parsed->nameIndexOffsets.size > 2) {
		error = "We only support CFF tables with 1 Name entry (1 font).";
		return false;
	}

	//
	//				  dictIndex
	//

	parsed->dictIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
	cout.PrintLn("dictIndex:");
#endif
	if (!parsed->dictIndex->Parse(&ptr, &parsed->dictIndexData, &parsed->dictIndexOffsets, swapEndian)) {
		error = "dictIndex: " + error;
		return false;
	}
#ifdef LOG_VERBOSE
	cout << "dictIndex charstrings:\n" << cffs::DictCharString(
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
	//				  stringsIndex
	//

	parsed->stringsIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
	cout.PrintLn("stringsIndex:");
#endif
	if (!parsed->stringsIndex->Parse(&ptr, &parsed->stringsIndexData,
									 &parsed->stringsIndexOffsets, swapEndian)) {
		error = "stringsIndex: " + error;
		return false;
	}
#ifdef LOG_VERBOSE
	cout.PrintLn("stringsIndex data:");
	for (i32 i = 0; i < parsed->stringsIndexOffsets.size-1; i++) {
		String string(parsed->stringsIndexOffsets[i+1] - parsed->stringsIndexOffsets[i]);
		memcpy(string.data, parsed->stringsIndexData + parsed->stringsIndexOffsets[i], string.size);
		cout.Print("\n[", i, "]=\"", string, "\" ");
	}
	cout.Print(std::endl);
#endif

	//
	//				  gsubrIndex
	//

	parsed->gsubrIndex = (cffs::index*)ptr;
#ifdef LOG_VERBOSE
	cout.PrintLn("gsubrIndex:");
#endif
	if (!parsed->gsubrIndex->Parse(&ptr, &parsed->gsubrIndexData, &parsed->gsubrIndexOffsets, swapEndian)) {
		error = "gsubrIndex: " + error;
		return false;
	}
	// cout.Print("gsubrIndex data dump:\n", std::hex);
	// for (i32 i = 0; i < gsubrIndexOffsets.size-1; i++) {
	//	 String string(gsubrIndexOffsets[i+1] - gsubrIndexOffsets[i]);
	//	 memcpy(string.data, gsubrIndexData + gsubrIndexOffsets[i], string.size);
	//	 cout.Print("\n[", i, "]=\"");
	//	 for (i32 i = 0; i < string.size; i++) {
	//		 cout.Print("0x", (u32)(u8)string[i]);
	//		 if (i < string.size-1) {
	//			 cout.Print(", ");
	//		 }
	//	 }
	//	 cout.Print("\" ");
	// }
	// cout.Print(std::endl);

	//
	//				  charStringsIndex
	//

	if (parsed->dictIndexValues.CharStrings == -1) {
		error = "CFF data has no CharStrings offset!";
		return false;
	}

#ifdef LOG_VERBOSE
	cout.PrintLn("charStringsIndex:");
#endif
	ptr = (char*)this + parsed->dictIndexValues.CharStrings;
	parsed->charStringsIndex = (cffs::index*)ptr;
	if (!parsed->charStringsIndex->Parse(&ptr, &parsed->charStringsIndexData,
										 &parsed->charStringsIndexOffsets, swapEndian)) {
		error = "charStringsIndex: " + error;
		return false;
	}

	//
	//				  charsets
	//

	// Do we have a predefined charset or a custom one?
	if (parsed->dictIndexValues.charset == 0) {
		// ISOAdobe charset
#ifdef LOG_VERBOSE
		cout.PrintLn("We are using the ISOAdobe predefined charset.");
#endif
	} else if (parsed->dictIndexValues.charset == 1) {
		// Expert charset
#ifdef LOG_VERBOSE
		cout.PrintLn("We are using the Expert predefined charset.");
#endif
	} else if (parsed->dictIndexValues.charset == 2) {
		// ExpertSubset charset
#ifdef LOG_VERBOSE
		cout.PrintLn("We are using the ExpertSubset predefined charset.");
#endif
	} else {
		// Custom charset
		cffs::charset_format_any *charset = (cffs::charset_format_any*)((char*)this + parsed->dictIndexValues.charset);
#ifdef LOG_VERBOSE
		cout.PrintLn("We are using a custom charset with format ", (i32)charset->format);
#endif
		if (swapEndian) {
			charset->EndianSwap(parsed->charStringsIndex->count);
		}
	}

	//
	//				  CIDFont
	//

	if (parsed->dictIndexValues.FDSelect != -1) {
		parsed->CIDFont = true;
		if (parsed->dictIndexValues.FDArray == -1) {
			error = "CIDFonts must have an FDArray!";
			return false;
		}
#ifdef LOG_VERBOSE
		cout.PrintLn("FDSelect:");
#endif
		parsed->fdSelect = (cffs::FDSelect_any*)((char*)this + parsed->dictIndexValues.FDSelect);
		if (swapEndian) {
			parsed->fdSelect->EndianSwap();
		}

#ifdef LOG_VERBOSE
		cout.PrintLn("FDArray:");
#endif
		ptr = (char*)this + parsed->dictIndexValues.FDArray;
		parsed->fdArray = (cffs::index*)ptr;
		if (!parsed->fdArray->Parse(&ptr, &parsed->fdArrayData, &parsed->fdArrayOffsets, swapEndian)) {
			error = "FDArray: " + error;
			return false;
		}
#ifdef LOG_VERBOSE
		for (i32 i = 0; i < parsed->fdArrayOffsets.size-1; i++) {
			cout << "fontDict[" << i << "] charstrings: " << cffs::DictCharString(
				parsed->fdArrayData + parsed->fdArrayOffsets[i],
				parsed->fdArrayData + parsed->fdArrayOffsets[i+1]
			) << std::endl;
			cffs::dict Dict = parsed->dictIndexValues;
			Dict.ParseCharString(parsed->fdArrayData + parsed->fdArrayOffsets[i], parsed->fdArrayOffsets[i+1] - parsed->fdArrayOffsets[i]);
			cout.Print("Name: ");
			if (Dict.FontName >= cffs::nStdStrings) {
				i32 s = Dict.FontName - cffs::nStdStrings;
				String name(parsed->stringsIndexOffsets[s+1] - parsed->stringsIndexOffsets[s]);
				memcpy(name.data, parsed->stringsIndexData + parsed->stringsIndexOffsets[s], name.size);
				cout.PrintLn(name);
			} else {
				cout.PrintLn(cffs::stdStrings[Dict.FontName]);
			}
			cout << "Private DICT charstrings: " << cffs::DictCharString(
				((u8*)this) + Dict.Private.offset,
				((u8*)this) + Dict.Private.offset + Dict.Private.size
			) << std::endl;
		}
#endif
	}

	// My, that was quite a lot of stuff... mostly logging ain't it?
	return true;
}

} // namespace tables
} // namespace font
} // namespace AzCore

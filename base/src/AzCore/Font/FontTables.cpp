/*
    File: FontTables.cpp
    Author: Philip Haynes
*/
#include "../font.hpp"

#include "CFF.cpp"

namespace AzCore {

namespace font {
f32 ToF32(const F2Dot14_t& in) {
    f32 out;
    if (in & 0x8000) {
        if (in & 0x4000) {
            out = -1.0;
        } else {
            out = -2.0;
        }
    } else {
        if (in & 0x4000) {
            out = 1.0;
        } else {
            out = 0.0;
        }
    }
    const u16 dot14 = in & 0x3fff;
    out += (f32)dot14 / 16384.0f;
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
io::LogStream cout("font.log");

inline bool operator==(const Tag_t &a, const Tag_t &b) {
    return a.data == b.data;
}

namespace tables {

// When doing checksums, the data has to still be in big endian or this won't work.
u32 Checksum(u32 *table, u32 length) {
    u32 sum = 0;
    u32 *end = table + ((length+3) & ~3) / sizeof(u32);
    while (table < end) {
        sum += endianFromB(*table++);
    }
    return sum;
}

u32 ChecksumV2(u32 *table, u32 length) {
    u32 sum = 0;
    u32 *end = table + length/sizeof(u32);
    while (table < end) {
        sum += endianFromB(*table++);
    }
    if (length & 3) {
        u32 val = endianFromB(*table);
        u32 l = length & 3;
        val <<= l * 8;
        val >>= l * 8;
        sum += val;
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
    out.info.offset.x -= lsb * 2.0f;
    out.info.advance.x = (f32)metric.advanceWidth / (f32)header->unitsPerEm;
    out.info.advance.y = 0.0f;
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
    if (gheader->numberOfContours > 0) {
        out.AddFromGlyfPoints(pt, endPtsOfContours[0] + 1);
        pt += endPtsOfContours[0] + 1;
    }
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
        if (componentParse.useMyMetrics) {
            // cout << "useMyMetrics" << std::endl;
            out.info.advance = componentGlyph.info.advance;
            out.info.offset = componentGlyph.info.offset;
        }
        if (simple) {
            // cout << "simple" << std::endl;
            component.offset = offset;
            component.transform = componentParse.scale;
            out.components.Append(component);
        }
        // cout << "scale = { "
        //      << component.transform.h.x1 << ", "
        //      << component.transform.h.x2 << ", "
        //      << component.transform.h.y1 << ", "
        //      << component.transform.h.y2 << " }, offset = { "
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

Glyph cffParsed::GetGlyph(u32 glyphIndex) const {
    Glyph out;
    if (dictIndexValues.CharstringType == 2) {
        u8 *start = charStringsIndexData + charStringsIndexOffsets[glyphIndex];
        u32 size = charStringsIndexOffsets[glyphIndex+1] - charStringsIndexOffsets[glyphIndex];
        cffs::Type2ParsingInfo info;
        info.dictValues = dictIndexValues;
        if (CIDFont) {
            u32 fd = fdSelect->GetFD(glyphIndex, charStringsIndex->count);
            u8 *fontDict = fdArrayData + fdArrayOffsets[fd];
            info.dictValues.ParseCharString(fontDict, fdArrayOffsets[fd+1]-fdArrayOffsets[fd]);
        }
        u8 *privateDict = ((u8*)cffData) + info.dictValues.Private.offset;
        info.dictValues.ParseCharString(privateDict, info.dictValues.Private.size);
        if (info.dictValues.Subrs) {
            cffs::index *localSubrsIndex = (cffs::index*)(privateDict + info.dictValues.Subrs);
            char *dummy = (char*)localSubrsIndex;
            if (privateIndicesAlreadySwapped.Contains(localSubrsIndex)) {
                localSubrsIndex->Parse(&dummy, &info.subrData, &info.subrOffsets, false);
            } else {
                localSubrsIndex->Parse(&dummy, &info.subrData, &info.subrOffsets, SysEndian.little);
                privateIndicesAlreadySwapped.Append(localSubrsIndex);
            }
        }
        info.gsubrData = gsubrIndexData;
        info.gsubrOffsets = &gsubrIndexOffsets;
        out = cffs::GlyphFromType2CharString(start, size, info);
    } else {
        return out;
    }
    out.Simplify();
    out.Scale(mat2::Scaler(vec2(1.0f / (f32)header->unitsPerEm)));
    vec2 minBounds(1000.0), maxBounds(-1000.0);
    for (Curve2& curve2 : out.curve2s) {
        if (curve2.p1.x < minBounds.x) { minBounds.x = curve2.p1.x; }
        if (curve2.p2.x < minBounds.x) { minBounds.x = curve2.p2.x; }
        if (curve2.p3.x < minBounds.x) { minBounds.x = curve2.p3.x; }
        if (curve2.p4.x < minBounds.x) { minBounds.x = curve2.p4.x; }
        if (curve2.p1.y < minBounds.y) { minBounds.y = curve2.p1.y; }
        if (curve2.p2.y < minBounds.y) { minBounds.y = curve2.p2.y; }
        if (curve2.p3.y < minBounds.y) { minBounds.y = curve2.p3.y; }
        if (curve2.p4.y < minBounds.y) { minBounds.y = curve2.p4.y; }
        if (curve2.p1.x > maxBounds.x) { maxBounds.x = curve2.p1.x; }
        if (curve2.p2.x > maxBounds.x) { maxBounds.x = curve2.p2.x; }
        if (curve2.p3.x > maxBounds.x) { maxBounds.x = curve2.p3.x; }
        if (curve2.p4.x > maxBounds.x) { maxBounds.x = curve2.p4.x; }
        if (curve2.p1.y > maxBounds.y) { maxBounds.y = curve2.p1.y; }
        if (curve2.p2.y > maxBounds.y) { maxBounds.y = curve2.p2.y; }
        if (curve2.p3.y > maxBounds.y) { maxBounds.y = curve2.p3.y; }
        if (curve2.p4.y > maxBounds.y) { maxBounds.y = curve2.p4.y; }
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
    for (Curve2& curve2 : out.curve2s) {
        curve2.Offset(-minBounds);
    }
    for (Line& line : out.lines) {
        line.Offset(-minBounds);
    }
    if (out.lines.size + out.curve2s.size > 0) {
        out.info.size = maxBounds - minBounds;
        out.info.offset += minBounds;
    } else {
        out.info.size = 0.0f;
    }
    longHorMetric metric = horMetrics->Metric(glyphIndex, horHeader->numOfLongHorMetrics);
    f32 lsb = (f32)metric.leftSideBearing / (f32)header->unitsPerEm;
    out.info.offset.x -= lsb * 2.0f;
    out.info.advance.x = (f32)metric.advanceWidth / (f32)header->unitsPerEm;
    out.info.advance.y = 0.0f;
    return out;
}

GlyphInfo cffParsed::GetGlyphInfo(u32 glyphIndex) const {
    return GetGlyph(glyphIndex).info;
}

} // namespace tables
} // namespace font
} // namespace AzCore

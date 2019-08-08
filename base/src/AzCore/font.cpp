/*
    File: font.cpp
    Author: Philip Haynes
*/

#include "font.hpp"

// #define LOG_VERBOSE

#include "font_tables.cpp"

String ToString(font::Tag_t tag) {
    String string(4);
    for (u32 x = 0; x < 4; x++) {
        string[x] = tag.name[x];
    }
    return string;
}

namespace font {

const f32 sdfDistance = 0.12; // Units in the Em square

inline f32 BezierDerivative(f32 t, f32 p1, f32 p2, f32 p3) {
    return 2.0 * ((1.0-t) * (p2-p1) + t * (p3-p2));
}

inline i32 signToWinding(f32 d) {
    if (d > 0.0) {
        return 1;
    } else if (d < 0.0) {
        return -1;
    } else {
        return 0;
    }
}

inline i32 BezierDerivativeSign(f32 t, f32 p1, f32 p2, f32 p3) {
    return signToWinding((1.0-t) * (p2-p1) + t*(p3-p2));
}

i32 Line::Intersection(const vec2 &point) const {
    // Bezier(t) = (1-t)*p1 + t*p3
    // Bezier(t) = t(p3 - p1) + p1
    // t = -p1 / (p3 - p1)

    if (p2.x == p1.x) {
        // Vertical line
        if (p2.x >= point.x) {
            if (point.y >= p1.y && point.y < p2.y) {
                return 1;
            } else if (point.y >= p2.y && point.y < p1.y) {
                return -1;
            } else {
                return 0;
            }
        }
    } else {
        f32 a = p2.y - p1.y, b; // coefficients of the line: y = ax + b
        if (a == 0.0) {
            // Horizontal line
            return 0;
        } else {
            b = -p1.y + point.y;
            f32 t = b / a;
            if (a > 0.0) {
                if (t >= 0.0 && t < 1.0) {
                    f32 x = (p2.x - p1.x) * t + p1.x;
                    if (x >= point.x) {
                        return 1;
                    }
                }
            } else if (a < 0.0) {
                if (t > 0.0 && t <= 1.0) {
                    f32 x = (p2.x - p1.x) * t + p1.x;
                    if (x >= point.x) {
                        return -1;
                    }
                }
            }
        }
    }
    return 0;
};

f32 Line::DistanceLess(const vec2 &point, f32 distSquared) const {
    f32 dist = distSqrToLine<true>(p1, p2, point);
    if (dist < distSquared) {
        distSquared = dist;
    }
    return distSquared;
}

void Line::Scale(const mat2 &scale) {
    p1 = scale * p1;
    p2 = scale * p2;
}

void Line::Offset(const vec2 &offset) {
    p1 = p1 + offset;
    p2 = p2 + offset;
}

i32 Curve::Intersection(const vec2 &point) const {
    if (point.x > max(max(p1.x, p2.x), p3.x)) {
        return 0;
    }
    i32 winding = 0;
    // Bezier(t) = (1-t)*((1-t)*p1 + t*p2) + t*((1-t)*p2 + t*p3)
    // Bezier(t) = (1-t)*(1-t)*p1 + (1-t)t*(2*p2) + t^2*p3
    // Bezier(t) = p1 - t(2*p1) + t^2(p1) + t(2*p2) - t^2(2*p2) + t^2(p3)
    // Bezier(t) = t^2(p3 - 2*p2 + p1) + t(2*p2 - 2*p1) + p1
    // Bezier'(t) = 2(1-t)(p2-p1) + 2t(p3-p2);

    f32 a, b, c; // coefficients of the curve: y = ax^2 - bx + c
    a = p3.y - 2.0*p2.y + p1.y;
    if (a == 0.0) {
        Line line{p1, p3};
        return line.Intersection(point);
    }
    b = -2.0*(p2.y - p1.y);
    c = p1.y - point.y;
    const f32 bb = square(b);
    const f32 ac4 = 4.0*a*c;
    if (bb <= ac4) {
        // We don't care about complex answers or tangent lines.
        return 0;
    }
    const f32 squareRoot = sqrt(bb - ac4);
    a *= 2.0;
    const f32 t1 = (b + squareRoot) / a;
    const f32 t2 = (b - squareRoot) / a;
    a = (p3.x - 2.0*p2.x + p1.x);
    b = 2.0*(p2.x - p1.x);
    c = p1.x;
    bool t1InRange = false;
    bool t2InRange = false;
    if (p1.y < p3.y) {
        t1InRange = t1 >= 0.0 && t1 < 1.0;
        t2InRange = t2 >= 0.0 && t2 < 1.0;
    } else {
        t1InRange = t1 > 0.0 && t1 <= 1.0;
        t2InRange = t2 > 0.0 && t2 <= 1.0;
    }
    if (t1InRange) {
        f32 x = a*square(t1) + b*t1 + c;
        if (x >= point.x) {
            // We have an intersection
            winding += BezierDerivativeSign(t1, p1.y, p2.y, p3.y);
        }
    }
    if (t2InRange) {
        f32 x = a*square(t2) + b*t2 + c;
        if (x >= point.x) {
            // We have an intersection
            winding += BezierDerivativeSign(t2, p1.y, p2.y, p3.y);
        }
    }
    return winding;
}

f32 Curve::DistanceLess(const vec2 &point, f32 distSquared) const {
    // Try to do an early out if we can
    {
        f32 maxPointDistSquared = max(max(absSqr(p1-p2), absSqr(p2-p3)), absSqr(p3-p1));
        f32 minDistSquared = min(min(absSqr(p1-point), absSqr(p2-point)), absSqr(p3-point));
        if (minDistSquared > distSquared + maxPointDistSquared * 0.25) {
            // NOTE: Should this be maxPointDist*square(sin(pi/4)) ???
            return distSquared;
        }
    }
    // B(t) = (1-t)^2*p1 + 2t(1-t)*p2 + t^2*p3
    // B'(t) = 2((1-t)(p2-p1) + t(p3-p2))
    // M = p2 - p1
    // N = p3 - p2 - M
    // B'(t) = 2(M + t*N)
    // dot(B(t) - point, B'(t)) = 0
    // dot((1-t)^2*p1 + 2t(1-t)*p2 + t^2*p3 - point, 2(M + t*N)) = 0
    // at^3 + bt^2 + ct + d = 0
    // a = N^2
    // b = 3*dot(M, N)
    // c = 2*M^2 + dot(p1-point, N)
    // d = dot(p1-point, M)
    const vec2 m = p2 - p1;
    const vec2 n = p3 - p2 - m;
    const vec2 o = p1 - point;

    const f32 a = absSqr(n);
    const f32 b = dot(m, n) * 3.0;
    const f32 c = absSqr(m) * 2.0 + dot(o, n);
    const f32 d = dot(o, m);
    SolutionCubic<f32> solution = SolveCubic<f32>(a, b, c, d);
    // We're guaranteed at least 1 solution.
    f32 dist;
    if (solution.x1 < 0.0) {
        dist = absSqr(p1-point);
    } else if (solution.x1 > 1.0) {
        dist = absSqr(p3-point);
    } else {
        dist = absSqr(Point(solution.x1)-point);
    }
    if (dist < distSquared) {
        distSquared = dist;
    }
    if (solution.x3Real) {
        if (solution.x3 < 0.0) {
            dist = absSqr(p1-point);
        } else if (solution.x3 > 1.0) {
            dist = absSqr(p3-point);
        } else {
            dist = absSqr(Point(solution.x3)-point);
        }
        if (dist < distSquared) {
            distSquared = dist;
        }
    }
    if (solution.x2Real) {
        if (solution.x2 < 0.0) {
            dist = absSqr(p1-point);
        } else if (solution.x2 > 1.0) {
            dist = absSqr(p3-point);
        } else {
            dist = absSqr(Point(solution.x2)-point);
        }
        if (dist < distSquared) {
            distSquared = dist;
        }
    }
    return distSquared;
}

void Curve::Scale(const mat2 &scale) {
    p1 = scale * p1;
    p2 = scale * p2;
    p3 = scale * p3;
}

void Curve::Offset(const vec2 &offset) {
    p1 = p1 + offset;
    p2 = p2 + offset;
    p3 = p3 + offset;
}

bool Glyph::Inside(const vec2 &point) const {
    i32 winding = 0;
    for (const Curve& curve : curves) {
        winding += curve.Intersection(point);
    }
    for (const Line& line : lines) {
        winding += line.Intersection(point);
    }
    return winding != 0;
}

f32 Glyph::MinDistance(vec2 point, const f32& startingDist) const {
    f32 minDistSquared = startingDist*startingDist; // Glyphs should be normalized to the em square more or less.
    for (const Curve& curve : curves) {
        minDistSquared = curve.DistanceLess(point, minDistSquared);
    }
    for (const Line& line : lines) {
        minDistSquared = line.DistanceLess(point, minDistSquared);
    }
    return sqrt(minDistSquared);
}

void Glyph::AddFromGlyfPoints(glyfPoint *glyfPoints, i32 count) {
    Curve curve;
    Line line;
    for (i32 i = 0; i < count; i++) {
        if (glyfPoints[i%count].onCurve) {
            if (glyfPoints[(i+1)%count].onCurve) {
                // Line segment
                line.p1 = glyfPoints[i%count].coords;
                line.p2 = glyfPoints[(i+1)%count].coords;
                lines.Append(line);
            } else {
                // Bezier
                curve.p1 = glyfPoints[i%count].coords;
                curve.p2 = glyfPoints[(i+1)%count].coords;
                if (glyfPoints[(i+2)%count].onCurve) {
                    curve.p3 = glyfPoints[(i+2)%count].coords;
                    i++; // next iteration starts at i+2
                } else {
                    curve.p3 = (glyfPoints[(i+1)%count].coords + glyfPoints[(i+2)%count].coords) * 0.5;
                }
                curves.Append(curve);
            }
        } else {
            if (glyfPoints[(i+1)%count].onCurve) {
                // I don't think this should happen???
                continue;
            } else {
                // Implied on-curve points on either side
                curve.p1 = (glyfPoints[i%count].coords + glyfPoints[(i+1)%count].coords) * 0.5;
                curve.p2 = glyfPoints[(i+1)%count].coords;
                if (glyfPoints[(i+2)%count].onCurve) {
                    curve.p3 = glyfPoints[(i+2)%count].coords;
                    i++;
                } else {
                    curve.p3 = (glyfPoints[(i+1)%count].coords + glyfPoints[(i+2)%count].coords) * 0.5;
                }
                curves.Append(curve);
            }
        }
    }
}

void Glyph::Scale(const mat2 &scale) {
    for (Curve& curve : curves) {
        curve.Scale(scale);
    }
    for (Line& line : lines) {
        line.Scale(scale);
    }
}

void Glyph::Offset(const vec2 &offset) {
    for (Curve& curve : curves) {
        curve.Offset(offset);
    }
    for (Line& line : lines) {
        line.Offset(offset);
    }
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
    data.tableData.Resize(align(data.offsetMax-data.offsetMin, 4), 0);

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
        u16 numOfLongHorMetrics = 0;
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
            } else if (tag == "hhea"_Tag) {
                tables::hhea *hhea = (tables::hhea*)ptr;
                hhea->EndianSwap();
                numOfLongHorMetrics = hhea->numOfLongHorMetrics;
            }
        }
        // To parse the 'loca' table correctly, head needs to be parsed first
        // To parse the 'hmtx' table correctly, hhea needs to be parsed first
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
            } else if (tag == "hmtx"_Tag) {
                tables::hmtx *hmtx = (tables::hmtx*)ptr;
                hmtx->EndianSwap(numOfLongHorMetrics, numGlyphs);
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
            } else if (tag == "hhea"_Tag) {
                data.glyfParsed.horHeader = (tables::hhea*)ptr;
            } else if (tag == "hmtx"_Tag) {
                data.glyfParsed.horMetrics = (tables::hmtx*)ptr;
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
        if (data.glyfParsed.horHeader == nullptr) {
            error = "Can't use glyf without hhea!";
            return false;
        }
        if (data.glyfParsed.horMetrics == nullptr) {
            error = "Can't use glyf without hmtx!";
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

u16 Font::GetGlyphIndex(char32 unicode) const {
    for (i32 i = 0; i < data.cmaps.size; i++) {
        tables::cmap_format_any *cmap = (tables::cmap_format_any*)(data.tableData.data + data.cmaps[i]);
        u16 glyphIndex = cmap->GetGlyphIndex(unicode);
        if (glyphIndex) {
            return glyphIndex;
        }
    }
    return 0;
}

Glyph Font::GetGlyphByIndex(u16 index) const {
    if (data.cffParsed.active) {
        // TODO: Something not stupid like this
        throw std::runtime_error("CFF parsing not yet implemented!");
    } else if (data.glyfParsed.active) {
        return data.glyfParsed.GetGlyph(index);
    }
    throw std::runtime_error("No glyph data available/supported!");
}

Glyph Font::GetGlyph(char32 unicode) const {
    u16 glyphIndex = GetGlyphIndex(unicode);
    return GetGlyphByIndex(glyphIndex);
}

GlyphInfo Font::GetGlyphInfoByIndex(u16 index) const {
    if (data.cffParsed.active) {
        // TODO: Something not stupid like this
        throw std::runtime_error("CFF parsing not yet implemented!");
    } else if (data.glyfParsed.active) {
        return data.glyfParsed.GetGlyphInfo(index);
    }
    throw std::runtime_error("No glyph data available/supported!");
}

GlyphInfo Font::GetGlyphInfo(char32 unicode) const {
    u16 glyphIndex = GetGlyphIndex(unicode);
    return GetGlyphInfoByIndex(glyphIndex);
}

void Font::PrintGlyph(char32 unicode) const {
    static u64 totalParseTime = 0;
    static u64 totalDrawTime = 0;
    static u32 iterations = 0;
    Nanoseconds glyphParseTime, glyphDrawTime;
    glyphDrawTime = Nanoseconds(0);
    ClockTime start = Clock::now();
    Glyph glyph;
    try {
        glyph = GetGlyph(unicode);
    } catch (std::runtime_error &err) {
        cout << "Failed to get glyph: " << err.what() << std::endl;
        return;
    }
    glyphParseTime = Clock::now()-start;
    const f32 margin = 0.03;
    const i32 scale = 4;
    i32 stepsX = 16 * scale;
    i32 stepsY = 16 * scale;
    const char distSymbolsPos[] = "X-.";
    const char distSymbolsNeg[] = "@*'";
    const f32 factorX = 1.0 / (f32)stepsX;
    const f32 factorY = 1.0 / (f32)stepsY;
    stepsY += i32(ceil(f32(stepsY) * margin * 2.0));
    stepsX += i32(ceil(f32(stepsX) * margin * 2.0));
    for (i32 y = stepsY-1; y >= 0; y--) {
        f32 prevDist = 1000.0;
        for (i32 x = 0; x < stepsX; x++) {
            vec2 point((f32)x * factorX - margin, (f32)y * factorY - margin);
            // point *= max(glyph.size.x, glyph.size.y);
            if (point.x > glyph.info.size.x + margin) {
                break;
            }
            start = Clock::now();
            f32 dist = glyph.MinDistance(point, prevDist);
            glyphDrawTime += Clock::now()-start;
            prevDist = dist + factorX; // Assume the worst change possible
            if (glyph.Inside(point)) {
                if (dist < margin) {
                    cout << distSymbolsNeg[i32(dist/margin*3.0)];
                } else {
                    cout << ' ';
                }
            } else {
                if (dist < margin) {
                    cout << distSymbolsPos[i32(dist/margin*3.0)];
                } else {
                    cout << ' ';
                }
            }
        }
        cout << "\n";
    }
    cout << std::endl;
    totalParseTime += glyphParseTime.count();
    totalDrawTime += glyphDrawTime.count();
    iterations++;
    // cout << "Glyph Parse Time: " << glyphParseTime.count() << "ns\n"
    //     "Glyph Draw Time: " << glyphDrawTime.count() << "ns" << std::endl;
    if (iterations%64 == 0) {
        cout << "After " << iterations << " iterations, average glyph parse time is "
             << totalParseTime/iterations << "ns and average glyph draw time is "
             << totalDrawTime/iterations << "ns.\nTotal glyph parse time is "
             << totalParseTime/1000000 << "ms and total glyph draw time is "
             << totalDrawTime/1000000 << "ms." << std::endl;
    }
}

//
//      Some helper functions for FontBuilder
//

bool Intersects(const Box &a, const Box &b) {
    return (a.min.x <= b.max.x
         && a.max.x >= b.min.x
         && a.min.y <= b.max.y
         && a.max.y >= b.min.y);
}

bool Intersects(const Box &box, const vec2 &point) {
    const f32 epsilon = 0.001;
    return (point.x == median(box.min.x-epsilon, point.x, box.max.x+epsilon)
         && point.y == median(box.min.y-epsilon, point.y, box.max.y+epsilon));
}

void InsertCorner(Array<vec2> &array, vec2 toInsert) {
    // Make sure the corners are sorted by how far they are from the origin
    float dist = max(toInsert.x, toInsert.y);
    i32 insertPos = array.size;
    for (i32 i = 0; i < array.size; i++) {
        float dist2 = max(array[i].x, array[i].y);
        if (dist == dist2) {
            if (absSqr(toInsert) > absSqr(array[i]))
                continue;
        }
        if (dist <= dist2) {
            insertPos = i;
            break;
        }
    }
    array.Insert(insertPos, toInsert);
}

void PurgeCorners(Array<vec2> &corners, const Box &bounds) {
    for (i32 i = 0; i < corners.size; i++) {
        if (Intersects(bounds, corners[i])) {
            corners.Erase(i);
            i--;
        }
    }
}

const i32 boxListScale = 1;

bool BoxListXNode::Intersects(Box box) {
    for (i32 i = 0; i < boxes.size; i++) {
        if (font::Intersects(box, boxes[i])) {
            return true;
        }
    }
    return false;
}

void BoxListX::AddBox(Box box) {
    i32 minX = (i32)box.min.x / boxListScale;
    i32 maxX = (i32)box.max.x / boxListScale + 1;
    if (nodes.size < maxX) {
        nodes.Resize(maxX);
    }
    for (i32 x = minX; x < maxX; x++) {
        nodes[x].boxes.Append(box);
    }
}

bool BoxListX::Intersects(Box box) {
    i32 minX = (i32)box.min.x / boxListScale;
    i32 maxX = min((i32)box.max.x / boxListScale + 1, nodes.size);
    for (i32 x = minX; x < maxX; x++) {
        if (nodes[x].Intersects(box)) {
            return true;
        }
    }
    return false;
}

void BoxListY::AddBox(Box box) {
    i32 minY = (i32)box.min.y / boxListScale;
    i32 maxY = (i32)box.max.y / boxListScale + 1;
    if (lists.size < maxY) {
        lists.Resize(maxY);
    }
    for (i32 y = minY; y < maxY; y++) {
        lists[y].AddBox(box);
    }
}

bool BoxListY::Intersects(Box box) {
    i32 minY = (i32)box.min.y / boxListScale;
    i32 maxY = min((i32)box.max.y / boxListScale + 1, lists.size);
    for (i32 y = minY; y < maxY; y++) {
        if (lists[y].Intersects(box)) {
            return true;
        }
    }
    return false;
}

void FontBuilder::ResizeImage(i32 w, i32 h) {
    if (w == dimensions.x && h == dimensions.y) {
        return;
    }
    Array<u8> newPixels(w*h);
    for (i32 i = 0; i < newPixels.size; i+=8) {
        u64 *px = (u64*)(newPixels.data + i);
        *px = 0;
    }
    for (i32 y = 0; y < dimensions.y; y++) {
        for (i32 x = 0; x < dimensions.x; x++) {
            newPixels[y*w + x] = pixels[y*dimensions.x + x];
        }
    }
    dimensions = vec2i(w, h);
    pixels = std::move(newPixels);
}

bool FontBuilder::AddRange(char32 min, char32 max) {
    if (font == nullptr) {
        error = "You didn't give FontBuilder a Font*!";
        return false;
    }
    for (char32 c = min; c <= max; c++) { // OI, THAT'S THE LANGUAGE THIS IS WRITTEN IN!!!
        // Let the hype for such trivial things flow through you!
        u16 glyphIndex = font->GetGlyphIndex(c);
        if (allIndices.count(glyphIndex) == 0) {
            indicesToAdd += glyphIndex;
            allIndices.insert(glyphIndex);
        }
    }
    return true;
}

bool FontBuilder::AddString(WString string) {
    if (font == nullptr) {
        error = "You didn't give FontBuilder a Font*!";
        return false;
    }
    for (char32 c : string) {
        u16 glyphIndex = font->GetGlyphIndex(c);
        if (allIndices.count(glyphIndex) == 0) {
            indicesToAdd += glyphIndex;
            allIndices.insert(glyphIndex);
        }
    }
    return true;
}

void RenderThreadProc(FontBuilder *fontBuilder, Array<Glyph> *glyphsToAdd, const f32 boundSquare, const i32 numThreads, const i32 threadId) {
    vec2i &dimensions = fontBuilder->dimensions;
    for (i32 i = threadId; i < glyphsToAdd->size; i+=numThreads) {
        Glyph &glyph = (*glyphsToAdd)[i];
        if (glyph.info.size.x == 0.0 || glyph.info.size.y == 0.0 || glyph.components.size != 0) {
            continue;
        }
        const i32 texX = glyph.info.pos.x*dimensions.x;
        const i32 texY = glyph.info.pos.y*dimensions.y;
        const f32 offsetX = glyph.info.pos.x * (f32)dimensions.x - (f32)texX;
        const f32 offsetY = glyph.info.pos.y * (f32)dimensions.y - (f32)texY;
        const i32 texW = glyph.info.size.x*dimensions.x;
        const i32 texH = glyph.info.size.y*dimensions.y;

        const f32 factorX = boundSquare / (f32)dimensions.x;
        const f32 factorY = boundSquare / (f32)dimensions.y;

        for (i32 y = 0; y <= texH; y++) {
            f32 prevDist = sdfDistance;
            for (i32 x = 0; x <= texW; x++) {
                vec2 point = vec2(((f32)x - offsetX) * factorX - sdfDistance,
                                  ((f32)y + offsetY) * factorY - sdfDistance);
                i32 xx = texX + x;
                if (xx >= dimensions.x || xx < 0) {
                    break;
                }
                i32 yy = texY + texH - y;
                if (yy >= dimensions.y || yy < 0) {
                    break;
                }
                u8& pixel = fontBuilder->pixels[dimensions.x * yy + xx];
                f32 dist = glyph.MinDistance(point, prevDist);
                prevDist = dist + factorX; // Assume the worst change possible
                if (glyph.Inside(point)) {
                    if (dist < sdfDistance) {
                        dist = (1.0 + dist / sdfDistance) * 0.5;
                    } else {
                        dist = 1.0;
                    }
                } else {
                    if (dist < sdfDistance) {
                        dist = (1.0 - dist / sdfDistance) * 0.5;
                    } else {
                        dist = 0.0;
                    }
                }
                pixel = u8(dist*255.0);
            }
        }
    }
}

String FormatTime(Nanoseconds time) {
    String out;
    u64 count = time.count();
    const u64 unitTimes[] = {UINT64_MAX, 60000000000, 1000000000, 1000000, 1000, 1};
    const char *unitStrings[] = {"m", "s", "ms", "μs", "ns"};
    bool addSpace = false;
    for (u32 i = 0; i < 5; i++) {
        if (count > unitTimes[i+1]) {
            if (addSpace) {
                out += ' ';
            }
            out += ToString((count%unitTimes[i])/unitTimes[i+1]) + unitStrings[i];
            addSpace = true;
        }
    }
    return out;
}

bool FontBuilder::Build() {
    if (font == nullptr) {
        error = "You didn't give FontBuilder a Font*!";
        return false;
    }
    if (indicesToAdd.size == 0) {
        // Nothing to do.
        return true;
    }
    if (renderThreadCount < 1) {
        renderThreadCount = std::thread::hardware_concurrency();
        if (renderThreadCount == 0) {
            // Couldn't get actual concurrency so this is just a guess.
            // More than actual concurrency isn't really a bad thing since it's lockless, unless the kernel am dum.
            renderThreadCount = 8;
            cout << "Using default concurrency: " << renderThreadCount << std::endl;
        } else {
            cout << "Using concurrency: " << renderThreadCount << std::endl;
        }
    }
    ClockTime start = Clock::now();
    Array<Glyph> glyphsToAdd;
    glyphsToAdd.Reserve(indicesToAdd.size);
    for (i32 i = 0; i < indicesToAdd.size; i++) {
        Glyph glyph = font->GetGlyphByIndex(indicesToAdd[i]);
        if (glyph.components.size != 0) {
            for (Component& component : glyph.components) {
                if (allIndices.count(component.glyphIndex) == 0) {
                    indicesToAdd += component.glyphIndex;
                    allIndices.insert(component.glyphIndex);
                }
            }
            glyph.info.size = vec2(0.0);
            glyph.curves.Clear();
            glyph.lines.Clear();
        }
        glyphsToAdd.Append(std::move(glyph));
    }
    for (i32 i = 0; i < indicesToAdd.size; i++) {
        indexToId.Set(indicesToAdd[i], glyphs.size + i);
    }
    indicesToAdd.Clear();
    cout << "Took " << FormatTime(Nanoseconds(Clock::now()-start)) << " to parse glyphs." << std::endl;
    cout << "Packing " << glyphsToAdd.size << " glyphs..." << std::endl;
    struct SizeIndex {
        i32 index;
        vec2 size;
    };
    start = Clock::now();
    Array<SizeIndex> sortedIndices;
    sortedIndices.Reserve(glyphsToAdd.size/2);
    for (i32 i = glyphsToAdd.size-1; i >= 0; i--) {
        if (glyphsToAdd[i].info.size.x == 0.0 || glyphsToAdd[i].info.size.y == 0.0) {
            continue;
        }
        SizeIndex sizeIndex = {i, glyphsToAdd[i].info.size};
        i32 insertPos = 0;
        for (i32 ii = 0; ii < sortedIndices.size; ii++) {
            insertPos = ii;
            if (sortedIndices[ii].size.x == sizeIndex.size.x) {
                if (sortedIndices[ii].size.y >= sizeIndex.size.y)
                    continue;
            }
            if (sortedIndices[ii].size.x <= sizeIndex.size.x) {
                break;
            }
        }
        sortedIndices.Insert(insertPos, sizeIndex);
    }
    cout << "Took " << FormatTime(Nanoseconds(Clock::now()-start)) << " to sort by size." << std::endl;
    if (corners.size == 0) {
        corners.Append(vec2(0.0));
        bounding = vec2(0.0);
        boundSquare = 0.0;
        area = 0.0;
    }
    start = Clock::now();
    for (SizeIndex& si : sortedIndices) {
        Glyph &glyph = glyphsToAdd[si.index];
        Box box;
        for (i32 i = 0; i < corners.size; i++) {
            box.min = corners[i];
            box.max = box.min + glyph.info.size + vec2(sdfDistance*2.0);
            if (boxes.Intersects(box)) {
                // Go to the next corner
                continue;
            }
            glyph.info.pos = corners[i];
            area += (box.max.x-box.min.x) * (box.max.y-box.min.y);
            boxes.AddBox(box);
            PurgeCorners(corners, box);
            if (box.max.x > bounding.x) {
                bounding.x = box.max.x;
            }
            if (box.max.y > bounding.y) {
                bounding.y = box.max.y;
            }
            box.max += vec2(0.002);
            InsertCorner(corners, vec2(box.max.x, box.min.y));
            InsertCorner(corners, vec2(box.min.x, box.max.y));
            if (box.min.x != corners[i].x) {
                box.min.x = corners[i].x;
                InsertCorner(corners, vec2(box.min.x, box.max.y));
            }
            if (box.min.y != corners[i].y) {
                box.min.y = corners[i].y;
                InsertCorner(corners, vec2(box.max.x, box.min.y));
            }
            break;
        }
    }
    Nanoseconds packingTime = Clock::now()-start;
    cout << "Took " << FormatTime(packingTime) << " to pack glyphs." << std::endl;
    f32 totalArea = bounding.x * bounding.y;
    cout << "Of a total page area of " << totalArea << ", "
         << u32(area/totalArea*100.0) << "% was used." << std::endl;
    bounding.x = max(bounding.x, 1.0f);
    bounding.y = max(bounding.y, 1.0f);
    f32 oldBoundSquare = boundSquare;
    // if (boundSquare == 0.0) {
        boundSquare = i32(ceil(max(bounding.x, bounding.y))*64.0)/64;
    // } else {
    //     if (max(bounding.x, bounding.y) > boundSquare)
    //         boundSquare *= 2.0;
    // }
    scale = boundSquare;
    edge = sdfDistance*32.0;

    vec2i dimensionsNew = vec2i(i32(boundSquare)*resolution);
    cout << "Texture dimensions = {" << dimensionsNew.x << ", " << dimensionsNew.y << "}" << std::endl;
    ResizeImage(dimensionsNew.x, dimensionsNew.y);
    for (i32 i = 0; i < glyphs.size; i++) {
        Glyph& glyph = glyphs[i];
        glyph.info.pos /= boundSquare/oldBoundSquare;
        glyph.info.size /= boundSquare/oldBoundSquare;
        glyph.info.offset /= boundSquare/oldBoundSquare;
    }
    for (i32 i = 0; i < glyphsToAdd.size; i++) {
        Glyph& glyph = glyphsToAdd[i];
        glyph.info.size += sdfDistance*2.0;
        glyph.info.offset += sdfDistance;
        glyph.info.pos /= boundSquare;
        glyph.info.size /= boundSquare;
        glyph.info.offset /= boundSquare;
    }
    start = Clock::now();
    // Now do the rendering
    Array<Thread> threads(renderThreadCount);
    for (i32 i = 0; i < renderThreadCount; i++) {
        threads[i] = Thread(RenderThreadProc, this, &glyphsToAdd, boundSquare, renderThreadCount, i);
    }
    for (i32 i = 0; i < renderThreadCount; i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
    Nanoseconds renderingTime = Clock::now() - start;
    cout << "Rendering took " << FormatTime(renderingTime) << std::endl;
    glyphs.Append(std::move(glyphsToAdd));
    return true;
}

} // namespace font

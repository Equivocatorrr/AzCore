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
                bool found = false;
                for (i32 u = 0; u < data.uniqueTables.size; u++) {
                    if (record.offset == data.uniqueTables[u].offset) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    data.uniqueTables.Append(record);
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
#ifdef LOG_VERBOSE
                        cout << "\tEncoding[" << enc << "]:\nPlatformID: " << encoding->platformID
                             << " PlatformSpecificID: " << encoding->platformSpecificID
                             << "\nOffset: " << encoding->offset
                             << "\nFormat: " << *((u16*)(ptr+encoding->offset)) << "\n";
#endif
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

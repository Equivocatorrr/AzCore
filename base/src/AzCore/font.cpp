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
        cout << "Checksums completed. " << checksumsCorrect << "/" << checksumsCompleted
             << " correct.\n" << std::endl;

        if (SysEndian.little) {
            for (i32 i = 0; i < data.uniqueTables.size; i++) {
                tables::Record &record = data.uniqueTables[i];
                char *ptr = data.tableData.data + record.offset - data.offsetMin;
                Tag_t &tag = record.tableTag;
                Array<u32> uniqueEncodingOffsets;
                if (tag == "head"_Tag || tag == "bhed"_Tag) {
                    tables::head *head = (tables::head*)ptr;
                    head->EndianSwap();
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
                } else {
                    cout << ToString(record.tableTag) << " table not necessary/supported." << std::endl;
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
                if (tag == "head"_Tag || tag == "bhed"_Tag) {
                    tables::head *head = (tables::head*)ptr;
#ifdef LOG_VERBOSE
                    cout << "\thead table:\nVersion " << head->version.major << "." << head->version.minor
                         << " Revision: " << head->fontRevision.major << "." << head->fontRevision.minor
                         << "\nFlags: 0x" << std::hex << head->flags << " MacStyle: 0x" << head->macStyle
                         << std::dec << " unitsPerEm: " << head->unitsPerEm
                         << "\nxMin: " << head->xMin << " xMax: " << head->xMax
                         << " yMin: " << head->yMin << " yMax " << head->yMax << "\n" << std::endl;
#endif
                } else if (tag == "cmap"_Tag) {
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
                        #define CHOOSE(in) chosenCmap = in; data.cmaps[i] = u32((char*)index - data.tableData.data) + encoding->offset
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

/*
    File: font.cpp
    Author: Philip Haynes
*/

#include "font.hpp"

#define LOG_VERBOSE

namespace font {

    Tag_t operator "" _Tag(const char name[5], size_t size) {
        Tag_t out;
        for (u32 i = 0; i < 4; i++) {
            out.name[i] = name[i];
        }
        return out;
    }

    u16 readU16(std::ifstream &file, bool swapEndian) {
        u16 buffer;
        file.read((char*)&buffer, 2);
        return endianSwap(buffer, swapEndian);
    }
    u32 readU32(std::ifstream &file, bool swapEndian) {
        u32 buffer;
        file.read((char*)&buffer, 4);
        return endianSwap(buffer, swapEndian);
    }
    u64 readU64(std::ifstream &file, bool swapEndian) {
        u64 buffer;
        file.read((char*)&buffer, 8);
        return endianSwap(buffer, swapEndian);
    }

    String error = "No Error";
    io::logStream cout("font.log");

    bool operator==(Tag_t a, Tag_t b) {
        return a.data == b.data;
    }

    namespace tables {

        u32 Checksum(u32 *table, u32 length) {
            u32 sum = 0;
            u32 *end = table + ((length+3) & ~3) / sizeof(u32);
            while (table < end) {
                sum += *table++;
            }
            return sum;
        }

        void Offset::Read(std::ifstream &file, bool swapEndian) {
            char buffer[12];
            file.read(buffer, 12);
            sfntVersion.data = *(u32*)buffer;
            numTables = bytesToU16(&buffer[4], swapEndian);
            searchRange = bytesToU16(&buffer[6], swapEndian);
            entrySelector = bytesToU16(&buffer[8], swapEndian);
            rangeShift = bytesToU16(&buffer[10], swapEndian);
            tables.Resize(numTables);
            for (u32 i = 0; i < numTables; i++) {
                tables[i].Read(file, swapEndian);
            }
        }

        bool TTCHeader::Read(std::ifstream &file, bool swapEndian) {
            ttcTag.data = readU32(file, false);
            if (ttcTag == "ttcf"_Tag) {
                {
                    char buffer[8];
                    file.read(buffer, 8);
                    majorVersion = bytesToU16(&buffer[0], swapEndian);
                    minorVersion = bytesToU16(&buffer[2], swapEndian);
                    numFonts = bytesToU32(&buffer[4], swapEndian);
                }
                offsetTables.Resize(numFonts);
                file.read((char*)offsetTables.data, numFonts * 4);
                if (swapEndian) {
                    for (u32 i = 0; i < numFonts; i++) {
                        offsetTables[i] = endianSwap(offsetTables[i]);
                    }
                }
                if (majorVersion == 2) {
                    char buffer[12];
                    file.read(buffer, 12);
                    dsigTag.data = bytesToU32(&buffer[0], false);
                    dsigLength = bytesToU32(&buffer[4], swapEndian);
                    dsigOffset = bytesToU32(&buffer[8], swapEndian);
                } else if (majorVersion != 1) {
                    error = "Unknown TTC file version: " + ToString(majorVersion) + "." + ToString(minorVersion);
                    return false;
                }
            } else {
                majorVersion = 0;
                numFonts = 1;
                offsetTables.Resize(1);
                offsetTables[0] = 0;
            }
            return true;
        }

        void Record::Read(std::ifstream &file, bool swapEndian) {
            char buffer[sizeof(Record)];
            file.read(buffer, sizeof(Record));
            tableTag.data = *(u32*)buffer;
            checkSum = bytesToU32(&buffer[4], swapEndian);
            offset = bytesToU32(&buffer[8], swapEndian);
            length = bytesToU32(&buffer[12], swapEndian);
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

        const bool swapEndian = !isSystemBigEndian();

        data.ttcHeader.Read(data.file, swapEndian);

#ifdef LOG_VERBOSE
        if (data.ttcHeader.ttcTag.data == endianSwap((u32)0x10000,swapEndian)) {
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
            error = "Unknown sfntVersion for file: \"" + filename + "\"";
            return false;
        }
#endif

        data.offsetTables.Resize(data.ttcHeader.numFonts);
        for (u32 i = 0; i < data.ttcHeader.numFonts; i++) {
            data.file.seekg(data.ttcHeader.offsetTables[i]);
            tables::Offset &offsetTable = data.offsetTables[i];
            offsetTable.Read(data.file, swapEndian);
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
#ifdef LOG_VERBOSE
                char name[5] = {0};
                for (u32 x = 0; x < 4; x++) {
                    name[x] = record.tableTag.name[x];
                }
                cout << "\tTable: \"" << name << "\"\n\t\toffset = " << record.offset << ", length = " << record.length << std::endl;
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
        return true;
    }

}

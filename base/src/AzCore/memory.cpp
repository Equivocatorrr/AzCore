/*
    File: memory.cpp
    Author: Philip Haynes
*/

#include "memory.hpp"

#include "Memory/Endian.cpp"
#include "Memory/String.cpp"

#include <cstdio>

namespace AzCore {

size_t align(const size_t& size, const size_t& alignment) {
    // if (size % alignment == 0) {
    //     return size;
    // } else {
    //     return (size/alignment+1)*alignment;
    // }
    return (size + alignment-1) & ~(alignment-1);
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

Array<char> FileContents(String filename) {
    Array<char> result;
    FILE *file = fopen(filename.data, "rb");
    if (!file) {
        return result;
    }
    fseek(file, 0, SEEK_END);
    result.Resize(ftell(file));
    fseek(file, 0, SEEK_SET);
    if ((i32)fread(result.data, 1, result.size, file) != result.size) {
        return result;
    }
    fclose(file);
    return result;
}

} // namespace AzCore

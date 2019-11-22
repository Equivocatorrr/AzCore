/*
    File: Endian.cpp
    Author: Philip Haynes
*/

#include "Endian.hpp"

namespace AzCore {

SystemEndianness_t SysEndian{1};

u16 bytesToU16(char bytes[2], bool swapEndian)
{
    union {
        char bytes[2];
        u16 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 2; i++)
        {
            out.bytes[i] = bytes[1 - i];
        }
    }
    else
    {
        out.val = *(u16 *)bytes;
    }
    return out.val;
}
u32 bytesToU32(char bytes[4], bool swapEndian)
{
    union {
        char bytes[4];
        u32 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 4; i++)
        {
            out.bytes[i] = bytes[3 - i];
        }
    }
    else
    {
        out.val = *(u32 *)bytes;
    }
    return out.val;
}
u64 bytesToU64(char bytes[8], bool swapEndian)
{
    union {
        char bytes[8];
        u64 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 8; i++)
        {
            out.bytes[i] = bytes[7 - i];
        }
    }
    else
    {
        out.val = *(u64 *)bytes;
    }
    return out.val;
}

i16 bytesToI16(char bytes[2], bool swapEndian)
{
    union {
        char bytes[2];
        i16 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 2; i++)
        {
            out.bytes[i] = bytes[1 - i];
        }
    }
    else
    {
        out.val = *(i16 *)bytes;
    }
    return out.val;
}
i32 bytesToI32(char bytes[4], bool swapEndian)
{
    union {
        char bytes[4];
        i32 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 4; i++)
        {
            out.bytes[i] = bytes[3 - i];
        }
    }
    else
    {
        out.val = *(i32 *)bytes;
    }
    return out.val;
}
i64 bytesToI64(char bytes[8], bool swapEndian)
{
    union {
        char bytes[8];
        i64 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 8; i++)
        {
            out.bytes[i] = bytes[7 - i];
        }
    }
    else
    {
        out.val = *(i64 *)bytes;
    }
    return out.val;
}

f32 bytesToF32(char bytes[4], bool swapEndian)
{
    union {
        char bytes[4];
        f32 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 4; i++)
        {
            out.bytes[i] = bytes[3 - i];
        }
    }
    else
    {
        out.val = *(f32 *)bytes;
    }
    return out.val;
}
f64 bytesToF64(char bytes[8], bool swapEndian)
{
    union {
        char bytes[8];
        f64 val;
    } out;
    if (swapEndian)
    {
        for (u32 i = 0; i < 8; i++)
        {
            out.bytes[i] = bytes[7 - i];
        }
    }
    else
    {
        out.val = *(f64 *)bytes;
    }
    return out.val;
}

u16 endianSwap(u16 in, bool swapEndian)
{
    if (!swapEndian)
    {
        return in;
    }
    union {
        char bytes[2];
        u16 val;
    } b_in, b_out;
    b_in.val = in;
    for (u32 i = 0; i < 2; i++)
    {
        b_out.bytes[i] = b_in.bytes[1 - i];
    }
    return b_out.val;
}

u32 endianSwap(u32 in, bool swapEndian)
{
    if (!swapEndian)
    {
        return in;
    }
    union {
        char bytes[4];
        u32 val;
    } b_in, b_out;
    b_in.val = in;
    for (u32 i = 0; i < 4; i++)
    {
        b_out.bytes[i] = b_in.bytes[3 - i];
    }
    return b_out.val;
}

u64 endianSwap(u64 in, bool swapEndian)
{
    if (!swapEndian)
    {
        return in;
    }
    union {
        char bytes[8];
        u64 val;
    } b_in, b_out;
    b_in.val = in;
    for (u32 i = 0; i < 8; i++)
    {
        b_out.bytes[i] = b_in.bytes[7 - i];
    }
    return b_out.val;
}

} // namespace AzCore
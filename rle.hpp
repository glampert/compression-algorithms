
// ================================================================================================
// -*- C++ -*-
// File: rle.hpp
// Author: Guilherme R. Lampert
// Created on: 16/02/16
// Brief: Simple Run Length Encoding (RLE) in C++11.
// ================================================================================================

#ifndef RLE_HPP
#define RLE_HPP

// ---------
//  LICENSE
// ---------
// This software is in the public domain. Where that dedication is not recognized,
// you are granted a perpetual, irrevocable license to copy, distribute, and modify
// this file as you see fit.
//
// The source code is provided "as is", without warranty of any kind, express or implied.
// No attribution is required, but a mention about the author is appreciated.
//
// -------
//  SETUP
// -------
// #define RLE_IMPLEMENTATION in one source file before including
// this file, then use rle.hpp as a normal header file elsewhere.
//

#include <cstdint>

namespace rle
{

// 16-bits run-length word allows for very long sequences,
// but is also very inefficient if the run-lengths are generally
// short. Byte-size words are used if this is not defined.
//#define RLE_WORD_SIZE_16

using UByte = std::uint8_t;

#ifdef RLE_WORD_SIZE_16
    using UWord = std::uint16_t;
    constexpr UWord MaxRunLength = 0xFFFF; // Max run length: 65535 => 4 bytes.
#else // !RLE_WORD_SIZE_16
    using UWord = std::uint8_t;
    constexpr UWord MaxRunLength = 0xFF;   // Max run length: 255 => 2 bytes.
#endif // RLE_WORD_SIZE_16

// RLE encode/decode raw bytes:
int easyEncode(const UByte * input, int inSizeBytes, UByte * output, int outSizeBytes);
int easyDecode(const UByte * input, int inSizeBytes, UByte * output, int outSizeBytes);

} // namespace rle {}

// ================== End of header file ==================
#endif // RLE_HPP
// ================== End of header file ==================

// ================================================================================================
//
//                                     RLE Implementation
//
// ================================================================================================

#ifdef RLE_IMPLEMENTATION

namespace rle
{

template<typename T>
static int writeData(UByte *& output, const T val)
{
    *reinterpret_cast<T *>(output) = val;
    output += sizeof(T);
    return sizeof(T);
}

template<typename T>
static void readData(const UByte *& input, T & val)
{
    val = *reinterpret_cast<const T *>(input);
    input += sizeof(T);
}

int easyEncode(const UByte * input, const int inSizeBytes, UByte * output, const int outSizeBytes)
{
    if (input == nullptr || output == nullptr)
    {
        return -1;
    }
    if (inSizeBytes <= 0 || outSizeBytes <= 0)
    {
        return -1;
    }

    int bytesWritten = 0;
    UWord rleCount   = 0;
    UByte rleByte    = *input;

    for (int i = 0; i < inSizeBytes; ++i, ++rleCount)
    {
        const UByte b = *input++;

        // Output when we hit the end of a sequence or the max size of a RLE word:
        if (b != rleByte || rleCount == MaxRunLength)
        {
            if ((bytesWritten + sizeof(UWord) + sizeof(UByte)) > static_cast<unsigned>(outSizeBytes))
            {
                // Can't fit anymore data! Stop with an error.
                return -1;
            }
            bytesWritten += writeData(output, rleCount);
            bytesWritten += writeData(output, rleByte);
            rleCount = 0;
            rleByte  = b;
        }
    }

    // Residual count at the end:
    if (rleCount != 0)
    {
        if ((bytesWritten + sizeof(UWord) + sizeof(UByte)) > static_cast<unsigned>(outSizeBytes))
        {
            return -1; // No more space! Output not complete.
        }
        bytesWritten += writeData(output, rleCount);
        bytesWritten += writeData(output, rleByte);
    }

    return bytesWritten;
}

int easyDecode(const UByte * input, const int inSizeBytes, UByte * output, const int outSizeBytes)
{
    if (input == nullptr || output == nullptr)
    {
        return -1;
    }
    if (inSizeBytes <= 0 || outSizeBytes <= 0)
    {
        return -1;
    }

    int bytesWritten = 0;
    UWord rleCount   = 0;
    UByte rleByte    = 0;

    for (int i = 0; i < inSizeBytes; i += sizeof(rleCount) + sizeof(rleByte))
    {
        readData(input, rleCount);
        readData(input, rleByte);

        // Replicate the RLE packet.
        while (rleCount--)
        {
            *output++ = rleByte;
            if (++bytesWritten == outSizeBytes && rleCount != 0)
            {
                // Reached end of output and we are not done yet, stop with an error.
                return -1;
            }
        }
    }

    return bytesWritten;
}

} // namespace rle {}

// ================ End of implementation =================
#endif // RLE_IMPLEMENTATION
// ================ End of implementation =================

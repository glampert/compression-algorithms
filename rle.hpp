
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
// RLE_WORD_SIZE_16 #define controls the size of the RLE word/count.
// If not defined, use 8-bits count.

#include <cstdint>

namespace rle
{

// RLE encode/decode raw bytes:
int easyEncode(const std::uint8_t * input, int inSizeBytes, std::uint8_t * output, int outSizeBytes);
int easyDecode(const std::uint8_t * input, int inSizeBytes, std::uint8_t * output, int outSizeBytes);

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

//
// #define RLE_WORD_SIZE_16
// 16-bits run-length word allows for very long sequences,
// but is also very inefficient if the run-lengths are generally
// short. Byte-size words are used if this is not defined.
//
#ifdef RLE_WORD_SIZE_16
    using RleWord = std::uint16_t;
    constexpr RleWord MaxRunLength = RleWord(0xFFFF); // Max run length: 65535 => 4 bytes.
#else // !RLE_WORD_SIZE_16
    using RleWord = std::uint8_t;
    constexpr RleWord MaxRunLength = RleWord(0xFF);   // Max run length: 255 => 2 bytes.
#endif // RLE_WORD_SIZE_16

// ========================================================

template<typename T>
static int writeData(std::uint8_t *& output, const T val)
{
    *reinterpret_cast<T *>(output) = val;
    output += sizeof(T);
    return sizeof(T);
}

template<typename T>
static void readData(const std::uint8_t *& input, T & val)
{
    val = *reinterpret_cast<const T *>(input);
    input += sizeof(T);
}

// ========================================================

int easyEncode(const std::uint8_t * input, const int inSizeBytes, std::uint8_t * output, const int outSizeBytes)
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
    RleWord rleCount = 0;
    std::uint8_t rleByte = *input;

    for (int i = 0; i < inSizeBytes; ++i, ++rleCount)
    {
        const std::uint8_t b = *input++;

        // Output when we hit the end of a sequence or the max size of a RLE word:
        if (b != rleByte || rleCount == MaxRunLength)
        {
            if ((bytesWritten + sizeof(RleWord) + sizeof(std::uint8_t)) > static_cast<unsigned>(outSizeBytes))
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
        if ((bytesWritten + sizeof(RleWord) + sizeof(std::uint8_t)) > static_cast<unsigned>(outSizeBytes))
        {
            return -1; // No more space! Output not complete.
        }
        bytesWritten += writeData(output, rleCount);
        bytesWritten += writeData(output, rleByte);
    }

    return bytesWritten;
}

// ========================================================

int easyDecode(const std::uint8_t * input, const int inSizeBytes, std::uint8_t * output, const int outSizeBytes)
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
    RleWord rleCount = 0;
    std::uint8_t rleByte = 0;

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

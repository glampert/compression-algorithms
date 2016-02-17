
// ================================================================================================
// -*- C++ -*-
// File: tests.cpp
// Author: Guilherme R. Lampert
// Created on: 15/02/16
// Brief: Basic unit tests for the data compression algorithms.
//
// This source code is in the public domain.
// You are free to do whatever you want with it.
//
// Compile with:
// c++ -std=c++11 -O3 -Wall -Wextra -Weffc++ -Wshadow -pedantic tests.cpp -o tests
// ================================================================================================

#define RLE_IMPLEMENTATION
#include "rle.hpp"

#define LZW_IMPLEMENTATION
#include "lzw.hpp"

#define HUFFMAN_IMPLEMENTATION
#include "huffman.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>

// ========================================================
// Test sample data:
// ========================================================

// A binary dump of "lenna.tga", the classic Computer Graphics
// sample image. Size is 256x256 pixels, RGBA uncompressed.
// This is meant to stress test the algorithms, since the
// buffer is fairly large.
#include "lenna_tga.hpp"

// 512 randomly shuffled byte values:
static const std::uint8_t random512[512] = {
    92 , 247, 240, 124, 48 , 228, 11 , 232, 194, 52 , 109, 48 ,
    208, 39 , 109, 31 , 1  , 245, 107, 13 , 181, 38 , 201, 78 ,
    194, 107, 50 , 116, 120, 88 , 250, 172, 81 , 155, 12 , 42 ,
    141, 210, 209, 175, 116, 227, 185, 171, 241, 121, 155, 85 ,
    139, 119, 244, 0  , 34 , 163, 104, 25 , 195, 75 , 248, 226,
    149, 191, 110, 239, 2  , 165, 166, 12 , 222, 140, 35 , 53 ,
    22 , 145, 158, 115, 50 , 80 , 249, 91 , 226, 90 , 224, 18 ,
    92 , 23 , 97 , 83 , 139, 29 , 242, 81 , 53 , 197, 206, 184,
    113, 11 , 213, 25 , 34 , 69 , 118, 154, 184, 63 , 62 , 243,
    212, 4  , 192, 235, 211, 148, 21 , 3  , 33 , 182, 204, 6  ,
    217, 173, 182, 169, 151, 127, 235, 101, 234, 88 , 21 , 242,
    206, 136, 96 , 28 , 175, 8  , 18 , 245, 150, 170, 19 , 174,
    183, 82 , 44 , 157, 141, 61 , 114, 100, 218, 138, 218, 135,
    61 , 89 , 241, 126, 112, 191, 215, 229, 113, 129, 231, 76 ,
    89 , 64 , 102, 185, 15 , 30 , 68 , 16 , 69 , 77 , 147, 187,
    7  , 183, 250, 57 , 51 , 144, 94 , 40 , 203, 63 , 66 , 189,
    132, 171, 80 , 134, 190, 4  , 2  , 127, 133, 118, 72 , 55 ,
    212, 189, 90 , 103, 87 , 44 , 132, 143, 255, 221, 243, 100,
    200, 237, 10 , 47 , 128, 20 , 52 , 57 , 40 , 176, 230, 156,
    230, 154, 198, 234, 161, 163, 45 , 167, 101, 146, 142, 179,
    169, 98 , 238, 114, 150, 14 , 83 , 24 , 202, 136, 219, 222,
    35 , 103, 28 , 37 , 70 , 251, 51 , 177, 124, 22 , 197, 20 ,
    214, 42 , 199, 159, 147, 244, 65 , 55 , 46 , 253, 30 , 188,
    239, 6  , 32 , 108, 205, 23 , 199, 180, 168, 108, 160, 24 ,
    79 , 198, 74 , 3  , 96 , 153, 216, 159, 152, 119, 67 , 93 ,
    247, 187, 5  , 91 , 41 , 143, 176, 19 , 177, 65 , 236, 135,
    93 , 95 , 205, 68 , 76 , 190, 217, 164, 224, 209, 82 , 219,
    161, 220, 129, 162, 85 , 84 , 152, 248, 210, 145, 246, 56 ,
    60 , 128, 225, 149, 146, 125, 153, 26 , 131, 49 , 211, 123,
    70 , 117, 204, 86 , 137, 236, 170, 142, 86 , 164, 202, 180,
    105, 98 , 37 , 254, 196, 214, 31 , 46 , 213, 62 , 79 , 66 ,
    115, 84 , 254, 5  , 178, 251, 223, 95 , 117, 36 , 122, 17 ,
    162, 148, 126, 156, 238, 167, 33 , 94 , 123, 87 , 255, 229,
    78 , 111, 221, 240, 228, 13 , 173, 200, 193, 43 , 186, 216,
    138, 232, 225, 49 , 15 , 157, 9  , 41 , 59 , 249, 160, 220,
    38 , 144, 192, 102, 122, 193, 47 , 17 , 223, 181, 97 , 26 ,
    207, 73 , 196, 16 , 71 , 7  , 203, 99 , 252, 29 , 233, 130,
    120, 110, 168, 227, 174, 14 , 231, 99 , 73 , 165, 43 , 158,
    9  , 252, 188, 1  , 8  , 32 , 112, 137, 54 , 172, 131, 27 ,
    27 , 59 , 201, 10 , 64 , 179, 58 , 74 , 58 , 237, 134, 0  ,
    207, 130, 77 , 72 , 253, 60 , 106, 233, 71 , 121, 178, 215,
    39 , 45 , 106, 186, 133, 36 , 56 , 54 , 166, 208, 75 , 104,
    105, 125, 67 , 151, 140, 195, 246, 111
};

// A couple strings:
static const std::uint8_t str0[] = "Hello world!";
static const std::uint8_t str1[] = "The Essential Feature;";
static const std::uint8_t str2[] = "Hello Dr. Chandra, my name is HAL-9000. I'm ready for my first lesson...";
static const std::uint8_t str3[] = "\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11";

// ========================================================
// Run Length Encoding (RLE) tests:
// ========================================================

static void Test_RLE_EncodeDecode(const rle::UByte * sampleData, const int sampleSize)
{
    std::vector<rle::UByte> compressedBuffer(sampleSize * 4, 0); // RLE might make things bigger.
    std::vector<rle::UByte> uncompressedBuffer(sampleSize, 0);

    // Compress:
    const int compressedSize = rle::easyEncode(sampleData, sampleSize,
                                               compressedBuffer.data(),
                                               compressedBuffer.size());

    std::cout << "RLE compressed size bytes   = " << compressedSize << "\n";
    std::cout << "RLE uncompressed size bytes = " << sampleSize << "\n";

    // Restore:
    const int uncompressedSize = rle::easyDecode(compressedBuffer.data(), compressedSize,
                                                 uncompressedBuffer.data(), uncompressedBuffer.size());

    // Validate:
    bool successful = true;
    if (uncompressedSize != sampleSize)
    {
        std::cerr << "RLE COMPRESSION ERROR! Size mismatch!\n";
        successful = false;
    }
    if (std::memcmp(uncompressedBuffer.data(), sampleData, sampleSize) != 0)
    {
        std::cerr << "RLE COMPRESSION ERROR! Data corrupted!\n";
        successful = false;
    }

    if (successful)
    {
        std::cout << "RLE compression successful!\n";
    }
    // No additional memory is allocated by the RLE encoder/decoder.
    // You have to provide big buffers.
}

static void Test_RLE()
{
    std::cout << "> Testing random512...\n";
    Test_RLE_EncodeDecode(random512, sizeof(random512));

    std::cout << "> Testing strings...\n";
    Test_RLE_EncodeDecode(str0, sizeof(str0));
    Test_RLE_EncodeDecode(str1, sizeof(str1));
    Test_RLE_EncodeDecode(str2, sizeof(str2));
    Test_RLE_EncodeDecode(str3, sizeof(str3));

    std::cout << "> Testing lenna.tga...\n";
    Test_RLE_EncodeDecode(lennaTgaData, sizeof(lennaTgaData));
}

// ========================================================
// LZW encoding/decoding tests:
// ========================================================

static void Test_LZW_EncodeDecode(const lzw::UByte * sampleData, const int sampleSize)
{
    int compressedSizeBytes = 0;
    int compressedSizeBits  = 0;
    lzw::UByte * compressedData = nullptr;
    std::vector<lzw::UByte> uncompressedBuffer(sampleSize, 0);

    // Compress:
    lzw::easyEncode(sampleData, sampleSize, &compressedData,
                    &compressedSizeBytes, &compressedSizeBits);

    std::cout << "LZW compressed size bytes   = " << compressedSizeBytes << "\n";
    std::cout << "LZW uncompressed size bytes = " << sampleSize << "\n";

    // Restore:
    const int uncompressedSize = lzw::easyDecode(compressedData, compressedSizeBytes, compressedSizeBits,
                                                 uncompressedBuffer.data(), uncompressedBuffer.size());

    // Validate:
    bool successful = true;
    if (uncompressedSize != sampleSize)
    {
        std::cerr << "LZW COMPRESSION ERROR! Size mismatch!\n";
        successful = false;
    }
    if (std::memcmp(uncompressedBuffer.data(), sampleData, sampleSize) != 0)
    {
        std::cerr << "LZW COMPRESSION ERROR! Data corrupted!\n";
        successful = false;
    }

    if (successful)
    {
        std::cout << "LZW compression successful!\n";
    }

    // easyEncode() uses LZW_MALLOC (std::malloc).
    LZW_MFREE(compressedData);
}

static void Test_LZW()
{
    std::cout << "> Testing random512...\n";
    Test_LZW_EncodeDecode(random512, sizeof(random512));

    std::cout << "> Testing strings...\n";
    Test_LZW_EncodeDecode(str0, sizeof(str0));
    Test_LZW_EncodeDecode(str1, sizeof(str1));
    Test_LZW_EncodeDecode(str2, sizeof(str2));
    Test_LZW_EncodeDecode(str3, sizeof(str3));

    std::cout << "> Testing lenna.tga...\n";
    Test_LZW_EncodeDecode(lennaTgaData, sizeof(lennaTgaData));
}

// ========================================================
// Huffman encoding/decoding tests:
// ========================================================

static void Test_Huffman_EncodeDecode(const huffman::UByte * sampleData, const int sampleSize)
{
    int compressedSizeBytes = 0;
    int compressedSizeBits  = 0;
    huffman::UByte * compressedData = nullptr;
    std::vector<huffman::UByte> uncompressedBuffer(sampleSize, 0);

    // Compress:
    huffman::easyEncode(sampleData, sampleSize, &compressedData,
                        &compressedSizeBytes, &compressedSizeBits);

    std::cout << "Huffman compressed size bytes   = " << compressedSizeBytes << "\n";
    std::cout << "Huffman uncompressed size bytes = " << sampleSize << "\n";

    // Restore:
    const int uncompressedSize = huffman::easyDecode(compressedData, compressedSizeBytes, compressedSizeBits,
                                                     uncompressedBuffer.data(), uncompressedBuffer.size());

    // Validate:
    bool successful = true;
    if (uncompressedSize != sampleSize)
    {
        std::cerr << "HUFFMAN COMPRESSION ERROR! Size mismatch!\n";
        successful = false;
    }
    if (std::memcmp(uncompressedBuffer.data(), sampleData, sampleSize) != 0)
    {
        std::cerr << "HUFFMAN COMPRESSION ERROR! Data corrupted!\n";
        successful = false;
    }

    if (successful)
    {
        std::cout << "Huffman compression successful!\n";
    }

    // easyEncode() uses HUFFMAN_MALLOC (std::malloc).
    HUFFMAN_MFREE(compressedData);
}

static void Test_Huffman()
{
    std::cout << "> Testing random512...\n";
    Test_Huffman_EncodeDecode(random512, sizeof(random512));

    std::cout << "> Testing strings...\n";
    Test_Huffman_EncodeDecode(str0, sizeof(str0));
    Test_Huffman_EncodeDecode(str1, sizeof(str1));
    Test_Huffman_EncodeDecode(str2, sizeof(str2));
    Test_Huffman_EncodeDecode(str3, sizeof(str3));

    std::cout << "> Testing lenna.tga...\n";
    Test_Huffman_EncodeDecode(lennaTgaData, sizeof(lennaTgaData));
}

// ========================================================
// main() -- Unit tests driver:
// ========================================================

#define TEST(func)                                                                                         \
    do                                                                                                     \
    {                                                                                                      \
        std::cout << ">>> Testing " << #func << " encoding/decoding.\n";                                   \
        const auto startTime = std::chrono::system_clock::now();                                           \
        Test_##func();                                                                                     \
        const auto endTime = std::chrono::system_clock::now();                                             \
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;                                \
        std::cout << ">>> " << #func << " tests completed in " << elapsedSeconds.count() << " seconds.\n"; \
        std::cout << std::endl;                                                                            \
    }                                                                                                      \
    while (0)

int main()
{
    std::cout << "\nRunning unit tests for the compression algorithms...\n\n";
    TEST(RLE);
    TEST(LZW);
    TEST(Huffman);
}

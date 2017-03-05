
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
// c++ -std=c++11 -O3 -Wall -Wextra -Weffc++ -Wshadow -pedantic -I.. tests.cpp -o tests
// ================================================================================================

#define RLE_IMPLEMENTATION
#include "rle.hpp"

#define LZW_IMPLEMENTATION
#include "lzw.hpp"

#define HUFFMAN_IMPLEMENTATION
#include "huffman.hpp"

#define RICE_IMPLEMENTATION
#include "rice.hpp"

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
#include "random_512.hpp"

// A couple strings:
static const std::uint8_t str0[] = "Hello world!";
static const std::uint8_t str1[] = "The Essential Feature;";
static const std::uint8_t str2[] = "Hello Dr. Chandra, my name is HAL-9000. I'm ready for my first lesson...";
static const std::uint8_t str3[] = "\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11";

// ========================================================
// Run Length Encoding (RLE) tests:
// ========================================================

static void Test_RLE_EncodeDecode(const std::uint8_t * sampleData, const int sampleSize)
{
    std::vector<std::uint8_t> compressedBuffer(sampleSize * 4, 0); // RLE might make things bigger.
    std::vector<std::uint8_t> uncompressedBuffer(sampleSize, 0);

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

static void Test_LZW_EncodeDecode(const std::uint8_t * sampleData, const int sampleSize)
{
    int compressedSizeBytes = 0;
    int compressedSizeBits  = 0;
    std::uint8_t * compressedData = nullptr;
    std::vector<std::uint8_t> uncompressedBuffer(sampleSize, 0);

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

static void Test_Huffman_EncodeDecode(const std::uint8_t * sampleData, const int sampleSize)
{
    int compressedSizeBytes = 0;
    int compressedSizeBits  = 0;
    std::uint8_t * compressedData = nullptr;
    std::vector<std::uint8_t> uncompressedBuffer(sampleSize, 0);

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
// Rice encoding/decoding tests (named after Robert Rice):
// ========================================================

static void Test_Rice_EncodeDecode(const std::uint8_t * sampleData, const int sampleSize)
{
    int compressedSizeBytes = 0;
    int compressedSizeBits  = 0;
    std::uint8_t * compressedData = nullptr;
    std::vector<std::uint8_t> uncompressedBuffer(sampleSize, 0);

    // Compress:
    rice::easyEncode(sampleData, sampleSize, &compressedData,
                     &compressedSizeBytes, &compressedSizeBits);

    std::cout << "Rice compressed size bytes   = " << compressedSizeBytes << "\n";
    std::cout << "Rice uncompressed size bytes = " << sampleSize << "\n";

    // Restore:
    const int uncompressedSize = rice::easyDecode(compressedData, compressedSizeBytes, compressedSizeBits,
                                                  uncompressedBuffer.data(), uncompressedBuffer.size());

    // Validate:
    bool successful = true;
    if (uncompressedSize != sampleSize)
    {
        std::cerr << "RICE COMPRESSION ERROR! Size mismatch!\n";
        successful = false;
    }
    if (std::memcmp(uncompressedBuffer.data(), sampleData, sampleSize) != 0)
    {
        std::cerr << "RICE COMPRESSION ERROR! Data corrupted!\n";
        successful = false;
    }

    if (successful)
    {
        std::cout << "Rice compression successful!\n";
    }

    // easyEncode() uses RICE_MALLOC (std::malloc).
    RICE_MFREE(compressedData);
}

static void Test_Rice()
{
    std::cout << "> Testing random512...\n";
    Test_Rice_EncodeDecode(random512, sizeof(random512));

    std::cout << "> Testing strings...\n";
    Test_Rice_EncodeDecode(str0, sizeof(str0));
    Test_Rice_EncodeDecode(str1, sizeof(str1));
    Test_Rice_EncodeDecode(str2, sizeof(str2));
    Test_Rice_EncodeDecode(str3, sizeof(str3));

    std::cout << "> Testing lenna.tga...\n";
    Test_Rice_EncodeDecode(lennaTgaData, sizeof(lennaTgaData));
}

// ========================================================
// main() -- Unit tests driver:
// ========================================================

#define TEST(func)                                                                                         \
    do {                                                                                                   \
        std::cout << ">>> Testing " << #func << " encoding/decoding.\n";                                   \
        const auto startTime = std::chrono::system_clock::now();                                           \
        Test_##func();                                                                                     \
        const auto endTime = std::chrono::system_clock::now();                                             \
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;                                \
        std::cout << ">>> " << #func << " tests completed in " << elapsedSeconds.count() << " seconds.\n"; \
        std::cout << std::endl;                                                                            \
    } while (0,0)

// ========================================================

int main()
{
    std::cout << "\nRunning unit tests for the compression algorithms...\n\n";
    TEST(RLE);
    TEST(LZW);
    TEST(Huffman);
    TEST(Rice);
}

// ========================================================


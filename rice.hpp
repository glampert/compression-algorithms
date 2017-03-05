
// ================================================================================================
// -*- C++ -*-
// File: rice.hpp
// Author: Guilherme R. Lampert
// Created on: 05/03/17
// Brief: Rice encoding/decoding (named after Robert F. Rice), which is based on Golomb Coding.
//        http://dmr.ath.cx/code/rice/
//        https://en.wikipedia.org/wiki/Golomb_coding
// ================================================================================================

#ifndef RICE_HPP
#define RICE_HPP

#include <cstdint>
#include <cstdlib>

// If you provide a custom malloc(), you must also provide a custom free().
// Note: We never check RICE_MALLOC's return for null. A custom implementation
// should just abort with a fatal error if the program runs out of memory.
#ifndef RICE_MALLOC
    #define RICE_MALLOC std::malloc
    #define RICE_MFREE  std::free
#endif // RICE_MALLOC

namespace rice
{

// ========================================================

// The default fatalError() function writes to stderr and aborts.
#ifndef RICE_ERROR
    void fatalError(const char * message);
    #define RICE_USING_DEFAULT_ERROR_HANDLER
    #define RICE_ERROR(message) ::rice::fatalError(message)
#endif // RICE_ERROR

// ========================================================
// class Encoder:
// ========================================================

class Encoder final
{
public:

    // No copy/assignment.
    Encoder(const Encoder &) = delete;
    Encoder & operator = (const Encoder &) = delete;

    Encoder();
    explicit Encoder(int initialSizeInBits, int growthGranularity = 2);

    void encodeByte(int value, int KBits);
    void writeKBitsWord(std::uint32_t KBits, int bitCount);
    void appendBit(int bit);

    static int computeCodeLength(int value, int KBits);
    static int findBestKBits(const std::uint8_t * input, int inSizeBytes, int KBitsMax, int * outBestSizeBits);

    int getByteCount() const;
    int getBitCount()  const;
    const std::uint8_t * getBitStream() const;

    void allocate(int bitsWanted);
    void setGranularity(int growthGranularity);
    std::uint8_t * release();

    ~Encoder();

private:

    void internalInit();
    static int nextPowerOfTwo(int num);
    static std::uint8_t * allocBytes(int bytesWanted, std::uint8_t * oldPtr, int oldSize);

    std::uint8_t * stream; // Growable buffer to store our bits. Heap allocated & owned by the class instance.
    int bytesAllocated;    // Current size of heap-allocated stream buffer *in bytes*.
    int granularity;       // Amount bytesAllocated multiplies by when auto-resizing in appendBit().
    int currBytePos;       // Current byte being written to, from 0 to bytesAllocated-1.
    int nextBitPos;        // Bit position within the current byte to access next. 0 to 7.
    int numBitsWritten;    // Number of bits in use from the stream buffer, not including byte-rounding padding.
};

// ========================================================
// class Decoder:
// ========================================================

class Decoder final
{
public:

    // No copy/assignment.
    Decoder(const Decoder &) = delete;
    Decoder & operator = (const Decoder &) = delete;

    Decoder(const Encoder & encoder);
    Decoder(const std::uint8_t * encodedData, int encodedSizeBytes, int encodedSizeBits);

    void reset();
    bool readNextBit(int & bitOut);
    int readKBitsWord(int bitCount);

    int getByteCount() const { return sizeInBytes; }
    int getBitCount()  const { return sizeInBits;  }
    const std::uint8_t * getBitStream() const { return stream; }

private:

    const std::uint8_t * stream; // Pointer to the external bit stream. Not owned by the reader.
    const int sizeInBytes;       // Size of the stream *in bytes*. Might include padding.
    const int sizeInBits;        // Size of the stream *in bits*, padding *not* include.
    int currBytePos;             // Current byte being read in the stream.
    int nextBitPos;              // Bit position within the current byte to access next. 0 to 7.
    int numBitsRead;             // Total bits read from the stream so far. Never includes byte-rounding padding.
};

// ========================================================
// easyEncode() / easyDecode():
// ========================================================

// Quick Rice data compression. Output compressed data is heap allocated
// with RICE_MALLOC() and should be later freed with RICE_MFREE().
void easyEncode(const std::uint8_t * uncompressed, int uncompressedSizeBytes,
                std::uint8_t ** compressed, int * compressedSizeBytes, int * compressedSizeBits);

// Decompress back the output of easyEncode().
// The uncompressed output buffer is assumed to be big enough to hold the uncompressed data,
// if it happens to be smaller, the decoder will return a partial output and the return value
// of this function will be less than uncompressedSizeBytes.
int easyDecode(const std::uint8_t * compressed, int compressedSizeBytes, int compressedSizeBits,
               std::uint8_t * uncompressed, int uncompressedSizeBytes);

} // namespace rice {}

// ================== End of header file ==================
#endif // RICE_HPP
// ================== End of header file ==================

// ================================================================================================
//
//                               Rice Encoder/Decoder Implementation
//
// ================================================================================================

#ifdef RICE_IMPLEMENTATION

#ifdef RICE_USING_DEFAULT_ERROR_HANDLER
    #include <cstdio> // For the default error handler
#endif // RICE_USING_DEFAULT_ERROR_HANDLER

#include <cassert>

namespace rice
{

// ========================================================

#ifdef RICE_USING_DEFAULT_ERROR_HANDLER

// Prints a fatal error to stderr and aborts the process.
// This is the default method used by RICE_ERROR(), but
// you can override the macro to use other error handling
// mechanisms, such as C++ exceptions.
void fatalError(const char * const message)
{
    std::fprintf(stderr, "Rice encoder/decoder error: %s\n", message);
    std::abort();
}

#endif // RICE_USING_DEFAULT_ERROR_HANDLER

// ========================================================
// class Encoder:
// ========================================================

Encoder::Encoder()
{
    // 8192 bits for a start (1024 bytes). It will resize if needed.
    // Default granularity is 2.
    internalInit();
    allocate(8192);
}

Encoder::Encoder(const int initialSizeInBits, const int growthGranularity)
{
    internalInit();
    setGranularity(growthGranularity);
    allocate(initialSizeInBits);
}

Encoder::~Encoder()
{
    if (stream != nullptr)
    {
        RICE_MFREE(stream);
    }
}

void Encoder::internalInit()
{
    stream         = nullptr;
    bytesAllocated = 0;
    granularity    = 2;
    currBytePos    = 0;
    nextBitPos     = 0;
    numBitsWritten = 0;
}

void Encoder::encodeByte(const int value, const int KBits)
{
    const int m = 1 << KBits;
    const int q = value / m;

    // Write the quotient code (q 1 bits followed by a terminating 0)
    for (int i = 0; i < q; ++i)
    {
        appendBit(1);
    }
    appendBit(0);

    // Write the reminder (last k bits of the value)
    for (int i = KBits - 1; i >= 0; i--)
    {
        appendBit((value >> i) & 1);
    }
}

int Encoder::computeCodeLength(const int value, const int KBits)
{
    const int m = 1 << KBits;
    const int q = value / m;
    return q + 1 + KBits;
}

int Encoder::findBestKBits(const std::uint8_t * input, const int inSizeBytes, const int KBitsMax, int * outBestSizeBits)
{
    assert(input != nullptr);
    assert(outBestSizeBits != nullptr);

    int bestKBits = 0;
    int bestSize  = 0;

    for (int k = 0; k <= KBitsMax; ++k)
    {
        int outputSize = 0;
        for (int i = 0; i < inSizeBytes; ++i)
        {
            outputSize += computeCodeLength(input[i], k);
        }

        if (bestSize == 0 || outputSize < bestSize)
        {
            bestSize = outputSize;
            bestKBits = k;
        }
    }

    *outBestSizeBits = bestSize;
    return bestKBits;
}

void Encoder::writeKBitsWord(const std::uint32_t KBits, const int bitCount)
{
    assert(bitCount <= 32);
    for (int b = 0; b < bitCount; ++b)
    {
        const std::uint32_t mask = std::uint32_t(1) << b;
        const int bit = !!(KBits & mask);
        appendBit(bit);
    }
}

void Encoder::appendBit(const int bit)
{
    const std::uint32_t mask = std::uint32_t(1) << nextBitPos;
    stream[currBytePos] = (stream[currBytePos] & ~mask) | (-bit & mask);
    ++numBitsWritten;

    if (++nextBitPos == 8)
    {
        nextBitPos = 0;
        if (++currBytePos == bytesAllocated)
        {
            allocate(bytesAllocated * granularity * 8);
        }
    }
}

int Encoder::getByteCount() const
{
    int usedBytes = numBitsWritten / 8;
    int leftovers = numBitsWritten % 8;
    if (leftovers != 0)
    {
        ++usedBytes;
    }
    assert(usedBytes <= bytesAllocated);
    return usedBytes;
}

int Encoder::getBitCount() const
{
    return numBitsWritten;
}

const std::uint8_t * Encoder::getBitStream() const
{
    return stream;
}

void Encoder::setGranularity(const int growthGranularity)
{
    granularity = (growthGranularity >= 2) ? growthGranularity : 2;
}

std::uint8_t * Encoder::release()
{
    std::uint8_t * oldPtr = stream;
    internalInit();
    return oldPtr;
}

void Encoder::allocate(int bitsWanted)
{
    // Require at least a byte.
    if (bitsWanted <= 0)
    {
        bitsWanted = 8;
    }

    // Round upwards if needed:
    if ((bitsWanted % 8) != 0)
    {
        bitsWanted = nextPowerOfTwo(bitsWanted);
    }

    // We might already have the required count.
    const int sizeInBytes = bitsWanted / 8;
    if (sizeInBytes <= bytesAllocated)
    {
        return;
    }

    stream = allocBytes(sizeInBytes, stream, bytesAllocated);
    bytesAllocated = sizeInBytes;
}

std::uint8_t * Encoder::allocBytes(const int bytesWanted, std::uint8_t * oldPtr, const int oldSize)
{
    std::uint8_t * newMemory = static_cast<std::uint8_t *>(RICE_MALLOC(bytesWanted));
    std::memset(newMemory, 0, bytesWanted);

    if (oldPtr != nullptr)
    {
        std::memcpy(newMemory, oldPtr, oldSize);
        RICE_MFREE(oldPtr);
    }

    return newMemory;
}

int Encoder::nextPowerOfTwo(int num)
{
    --num;
    for (std::size_t i = 1; i < sizeof(num) * 8; i <<= 1)
    {
        num = num | num >> i;
    }
    return ++num;
}

// ========================================================
// class Decoder:
// ========================================================

Decoder::Decoder(const Encoder & encoder)
    : Decoder(encoder.getBitStream(), encoder.getByteCount(), encoder.getBitCount())
{
}

Decoder::Decoder(const std::uint8_t * encodedData, const int encodedSizeBytes, const int encodedSizeBits)
    : stream(encodedData)
    , sizeInBytes(encodedSizeBytes)
    , sizeInBits(encodedSizeBits)
{
    reset();
}

void Decoder::reset()
{
    currBytePos = 0;
    nextBitPos  = 0;
    numBitsRead = 0;
}

bool Decoder::readNextBit(int & bitOut)
{
    if (numBitsRead >= sizeInBits)
    {
        return false; // We are done.
    }

    const std::uint32_t mask = std::uint32_t(1) << nextBitPos;
    bitOut = !!(stream[currBytePos] & mask);
    ++numBitsRead;

    if (++nextBitPos == 8)
    {
        nextBitPos = 0;
        ++currBytePos;
    }
    return true;
}

int Decoder::readKBitsWord(const int bitCount)
{
    assert(bitCount <= 32);

    std::uint32_t num = 0;
    for (int b = 0; b < bitCount; ++b)
    {
        int bit;
        if (!readNextBit(bit))
        {
            RICE_ERROR("Failed to read bits from stream! Unexpected end.");
            break;
        }

        // Based on a "Stanford bit-hack":
        // http://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
        const std::uint32_t mask = std::uint32_t(1) << b;
        num = (num & ~mask) | (-bit & mask);
    }

    return static_cast<int>(num);
}

// ========================================================
// easyEncode() implementation:
// ========================================================

void easyEncode(const std::uint8_t * uncompressed, const int uncompressedSizeBytes,
                std::uint8_t ** compressed, int * compressedSizeBytes, int * compressedSizeBits)
{
    if (uncompressed == nullptr || compressed == nullptr)
    {
        RICE_ERROR("rice::easyEncode(): Null data pointer(s)!");
        return;
    }

    if (uncompressedSizeBytes <= 0 || compressedSizeBytes == nullptr || compressedSizeBits == nullptr)
    {
        RICE_ERROR("rice::easyEncode(): Bad in/out sizes!");
        return;
    }

    // Do up to 8 passes to try finding the best K number of bits for the encoding.
    int minCompressedBitSize;
    const int KBits = Encoder::findBestKBits(uncompressed, uncompressedSizeBytes, 8, &minCompressedBitSize);

    Encoder bitStreamEncoder(minCompressedBitSize);

    // The decoder needs to know the number of bits we've used.
    // Since the max is 8, we only need up to 4 bits for that.
    bitStreamEncoder.writeKBitsWord(KBits, 4);

    // Encode each byte of the input:
    for (int b = 0; b < uncompressedSizeBytes; ++b)
    {
        bitStreamEncoder.encodeByte(uncompressed[b], KBits);
    }

    // Pass ownership of the compressed data buffer to the user pointer:
    *compressedSizeBytes = bitStreamEncoder.getByteCount();
    *compressedSizeBits  = bitStreamEncoder.getBitCount();
    *compressed          = bitStreamEncoder.release();
}

// ========================================================
// easyDecode() implementation:
// ========================================================

int easyDecode(const std::uint8_t * compressed, const int compressedSizeBytes, const int compressedSizeBits,
               std::uint8_t * uncompressed, const int uncompressedSizeBytes)
{
    if (compressed == nullptr || uncompressed == nullptr)
    {
        RICE_ERROR("rice::easyDecode(): Null data pointer(s)!");
        return 0;
    }

    if (compressedSizeBytes <= 0 || compressedSizeBits <= 0 || uncompressedSizeBytes <= 0)
    {
        RICE_ERROR("rice::easyDecode(): Bad in/out sizes!");
        return 0;
    }

    Decoder bitStreamDecoder(compressed, compressedSizeBytes, compressedSizeBits);

    // KBits word length is fixed to 4 bits.
    const int KBits = bitStreamDecoder.readKBitsWord(4);
    const int m = 1 << KBits;

    int bytesDecoded = 0;
    for (;;)
    {
        int q   = 0;
        int bit = 0;

        // Reconstruct q:
        while (bitStreamDecoder.readNextBit(bit) && (bit == 1))
        {
            ++q;
        }

        // Reconstruct the remainder:
        int value = m * q;
        for (int i = KBits - 1; i >= 0; i--)
        {
            if (!bitStreamDecoder.readNextBit(bit))
            {
                RICE_ERROR("Failed to read bits from stream! Unexpected end.");
                return bytesDecoded;
            }
            value = value | (bit << i);
        }

        *uncompressed++ = static_cast<std::uint8_t>(value);
        bytesDecoded++;

        if (bytesDecoded == uncompressedSizeBytes)
        {
            break; // Decompress buffer is full.
        }
    }

    return bytesDecoded;
}

} // namespace rice {}

// ================ End of implementation =================
#endif // RICE_IMPLEMENTATION
// ================ End of implementation =================

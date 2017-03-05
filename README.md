
# Compression Algorithms

C++11 implementations of common data compression algorithms.

**License:**

*Public Domain*. Source code in this repository is provided "as is", without warranty of any kind,
express or implied. No attribution is required, but a mention about the author is appreciated.

----

- `rle.hpp`: [Run Length Encoding](https://en.wikipedia.org/wiki/Run-length_encoding) with either 8 or 16 bits run-length words.
- `lzw.hpp`: [Lempel–Ziv–Welch](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch) compression with varying code lengths and a 4096 max entries dictionary.
- `huffman.hpp`: Simple [Huffman Coding](https://en.wikipedia.org/wiki/Huffman_coding) with 64-bits max code length.
- `rice.hpp`: [Rice/Golomb Coding](https://en.wikipedia.org/wiki/Golomb_coding) with optimal code length (8 bits max).

These libraries are header only and self contained. You have to include the `.hpp` in one source file
and define `XYZ_IMPLEMENTATION` to generate the implementation code in that source file. After that,
the header file can be used as a normal C++ header. This is the same design of the [stb](https://github.com/nothings/stb) libraries.

See `tests.cpp` for some usage examples.


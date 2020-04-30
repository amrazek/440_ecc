#pragma once
#include <bitset>


template <size_t NumDataBits, size_t NumCheckBits>
struct DecodeResult
{
    bool success;

    std::bitset<NumDataBits> decoded_bits;                  // the "result"
    std::bitset<NumDataBits + NumCheckBits> stored_bits;    // the bits, as they were stored (maybe corrupted)
    std::bitset<NumDataBits> original_bits;                 // the original data that was stored. Chunk will insert this

    size_t num_corrupt_bits;
    size_t num_corrected_bits;

    DecodeResult() : success(false), num_corrupt_bits(0), num_corrected_bits(0)
    {
    }
};

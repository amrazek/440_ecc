#pragma once
#include <bitset>
#include <memory>


template <size_t NumDataBits, size_t NumCheckBits>
struct DecodeResult
{
    bool success;

    std::bitset<NumDataBits> decoded_bits;
    // todo: uncorrected bits? so we can compare
    std::bitset<NumDataBits + NumCheckBits> stored_bits;

    size_t num_corrupt_bits;
    size_t num_corrected_bits;

    DecodeResult() : success(false), num_corrupt_bits(0), num_corrected_bits(0)
    {
    }
};

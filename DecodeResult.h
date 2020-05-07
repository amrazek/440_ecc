/*-----------------------------------------------------------------------------
 * DecodeResult.h
 *---------------------------------------------------------------------------*/
#pragma once
#include <bitset>


template <size_t NumDataBits, size_t NumEncodedBits>
struct DecodeResult
{
    bool success;                               // whether the correction strategy thinks the data is right
    bool error_detected;                        // was error detected?
    bool correct;                               // whether result was actually successful (original data == decoded data)

    std::bitset<NumDataBits> decoded_bits;                  // the "result"
    std::bitset<NumEncodedBits> stored_bits;    // the bits, as they were stored (maybe corrupted)
    std::bitset<NumDataBits> original_bits;                 // the original data that was stored. Chunk will insert this

    size_t num_corrupt_bits;
    size_t num_corrected_bits;

    DecodeResult() : success(false), error_detected(false), correct(false), num_corrupt_bits(0), num_corrected_bits(0)
    {
    }
};


/*/////////////////////////////////////////////////////////////////////////////
 * end DecodeResult.h
 *///////////////////////////////////////////////////////////////////////////*/
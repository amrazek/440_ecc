#pragma once
#include <bitset>
#include "CorrectionStrategy.h"


template <size_t NumDataBits>
// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class ParityBit: public CorrectionStrategy<NumDataBits, NumDataBits + 1>
{
    typedef typename CorrectionStrategy<NumDataBits, NumDataBits + 1>::DecodeResult DecodeResult;

public:

    // given a piece of data (in terms of bits), encodes data with parity bit
    std::bitset<NumDataBits + 1> encode(const typename CorrectionStrategy<NumDataBits, NumDataBits + 1>::DataBits& data) const override
    {
        auto check = BitStream<NumDataBits + 1>::convert_bitset_to_bitset(data);

        // parity bit should be LSB, at index 0 -> shift everything
        check <<= 1;

        check[0] = data.count() % 2 == 1; // odd number of bits set? parity bit = 1

        assert(check.count() % 2 == 0);

        return check;
    }


    // given stored data (in terms of bits, encoded by function above), recover original data
    DecodeResult decode(
        typename CorrectionStrategy<NumDataBits, NumDataBits + 1>::StoredBits storedData) const override
    {
        DecodeResult result;

        // if the stored data is unchanged, a parity check should result in zero (even parity)
        result.success = storedData.count() % 2 == 0;
        result.error_detected = !result.success;
        result.num_corrupt_bits = result.success ? 0 : 1;
        result.num_corrected_bits = 0;

        // useful to examine later
        result.stored_bits = storedData;

        // no error correction, so stored data is also returned data
        // just need to remove the parity bit that was added as LSB
        for (size_t i = 1; i < storedData.size(); ++i)
            result.decoded_bits[i - 1] = storedData[i];

        return result;
    }
};

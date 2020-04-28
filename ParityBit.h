#pragma once
#include <bitset>
#include "CorrectionStrategy.h"


template <int NumDataBits>
// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class ParityCheck: public CorrectionStrategy<NumDataBits, 1>
{
    typedef typename CorrectionStrategy<NumDataBits, 1>::DecodeResult DecodeResult;

public:

    // given a piece of data (in terms of bits), generates check bits
    std::bitset<NumDataBits + 1> encode(const typename CorrectionStrategy<NumDataBits, 1>::DataBits& data) const override
    {
        std::bitset<NumDataBits + 1> check;

        // want parity bit as LSB which is index 0 in the bitset convention
        for (size_t i = 0; i < NumDataBits; ++i)
            check[i + 1] = data[i];

        check[0] = data.count() % 2 == 1; // odd number of bits set? parity bit = 1

        assert(check.count() % 2 == 0);

        return check;
    }


    // given stored data (in terms of bits, encoded by function above), recover original data
    DecodeResult decode(
        typename CorrectionStrategy<NumDataBits, 1>::StoredBits storedData) const override
    {
        DecodeResult result;

        result.stored_bits = storedData;

        // no error correction, so stored data is also returned data
        // just need to remove the parity bit that was added as LSB
        for (size_t i = 1; i < storedData.size(); ++i)
            result.decoded_bits[i - 1] = storedData[i];

        // if the stored data is unchanged, a parity check should result in zero (even parity)
        result.success = storedData.count() % 2 == 0;

        result.num_corrupt_bits = result.success ? 0 : 1;

        return result;
    }
};

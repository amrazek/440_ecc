#pragma once
#include <cmath>
#include "BitStream.h"
#include "CorrectionStrategy.h"


constexpr size_t two_to_power_of(int exponent)
{
    return exponent > 0 ? 2 * two_to_power_of(exponent - 1) : 1;
}


constexpr size_t calc_num_redundant_bits(size_t dataBits, size_t redundant_bits = 0)
{
    // 2^r >= data bits + r + 1
    return two_to_power_of(redundant_bits) >= dataBits + redundant_bits + 1 ? redundant_bits : calc_num_redundant_bits(dataBits, redundant_bits + 1);
}


template <size_t NumDataBits>
class HammingCode : public CorrectionStrategy<NumDataBits, calc_num_redundant_bits(NumDataBits)>
{
    typedef std::bitset<NumDataBits + calc_num_redundant_bits(NumDataBits)> StoredDataBits;
    typedef DecodeResult<NumDataBits, calc_num_redundant_bits(NumDataBits)> DecodeResult;

public:
    static constexpr size_t DATA_BIT_COUNT = NumDataBits;
    static constexpr size_t CHECK_BIT_COUNT = calc_num_redundant_bits(NumDataBits);
    static constexpr size_t TOTAL_BIT_COUNT = DATA_BIT_COUNT + CHECK_BIT_COUNT;

    StoredDataBits encode(const std::bitset<NumDataBits>& data) const override
    {
        // the parity bits for hamming code are actually interleaved among the
        // data bits in a pattern: each parity bit is at a power-of-two index - 1
        StoredDataBits encoded;

        // first, distribute data bits among correct locations
        size_t idx_data = 0;

        for (size_t encode_idx = 0; encode_idx < TOTAL_BIT_COUNT; ++encode_idx)
        {
            // is this a parity bit location? (idx is a power of 2)
            // if so, ignore for now
            // note: this little snippet assumes 1-based array
            if (encode_idx + 1 && !(encode_idx + 1 & encode_idx))
                continue;
            
            // normal data bit
            encoded[encode_idx] = data[idx_data++];
        }
        
        // compute parity bits
        size_t parity_idx = 1;
        for (size_t i = 0; i < CHECK_BIT_COUNT; ++i)
        {
            const size_t mask = 1 << i;
 
            // compute parity value by looking at all bits
            // with the ith bit set in their index
            size_t counter = 0;

            for (size_t position = 0; position < TOTAL_BIT_COUNT; ++position) {
                const auto t = (mask & (position + 1));

                if (t != 0)
                    if (encoded.test(position))
                        ++counter;
            }

            // use even parity bit: counter should be an even value
            // set parity bit to 1 if it's not
            encoded[mask - 1] = counter % 2 == 1;
        }


        return encoded;
    }

    DecodeResult decode(StoredDataBits storedData) const override
    {
        DecodeResult result;

        result.success = false;

        return result;
    }
};
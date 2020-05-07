#pragma once
#include "CorrectionStrategy.h"
#include <functional>


constexpr size_t two_to_power_of(int exponent)
{
    return exponent > 0 ? 2 * two_to_power_of(exponent - 1) : 1;
}


constexpr size_t calc_hamming_code_check_bits(size_t dataBits, size_t redundant_bits = 0)
{
    // 2^r >= data bits + r + 1
    return two_to_power_of(redundant_bits) >= dataBits + redundant_bits + 1 ? redundant_bits : calc_hamming_code_check_bits(dataBits, redundant_bits + 1);
}


template <size_t NumDataBits>
class HammingCode : public CorrectionStrategy<NumDataBits, calc_hamming_code_check_bits(NumDataBits) + 1> // +1 for extending hamming code
{
    typedef std::bitset<NumDataBits + calc_hamming_code_check_bits(NumDataBits)> StoredDataBits_t;
    typedef DecodeResult<NumDataBits, calc_hamming_code_check_bits(NumDataBits)> DecodeResult_t;
    typedef std::function<void (StoredDataBits_t& data, size_t parityIdx, bool parityVal)> ParityCalcPostFunc_t;

    static void compute_parity_bits(StoredDataBits_t& encoded, ParityCalcPostFunc_t func)
    {
        // compute parity bits
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
            bool parityBit = counter % 2 == 1;

            func(encoded, mask - 1, parityBit); // parity bit actual idx is 0-based
        }
    }

    static bool is_power_of_two(size_t val)
    {
        return val && !(val & val - 1);
    }

public:
    static constexpr size_t DATA_BIT_COUNT = NumDataBits;
    static constexpr size_t CHECK_BIT_COUNT = calc_hamming_code_check_bits(NumDataBits);
    static constexpr size_t TOTAL_BIT_COUNT = DATA_BIT_COUNT + CHECK_BIT_COUNT;

    StoredDataBits_t encode(const std::bitset<NumDataBits>& data) const override
    {
        // the parity bits for hamming code are actually interleaved among the
        // data bits in a pattern: each parity bit is at a power-of-two index - 1
        StoredDataBits_t encoded;

        // first, distribute data bits among correct locations
        size_t idx_data = 0;

        for (size_t encode_idx = 0; encode_idx < TOTAL_BIT_COUNT; ++encode_idx)
        {
            // is this a parity bit location? (idx is a power of 2)
            // if so, ignore for now.
            // we assume 1-based array for this snippet
            if (is_power_of_two(encode_idx + 1))
                continue;
            
            // normal data bit
            encoded[encode_idx] = data[idx_data++];
        }

        // compute parity bits and set them
        compute_parity_bits(encoded, [](StoredDataBits_t& data, size_t parityIdx, bool parityVal) { data[parityIdx] = parityVal; });

        return encoded;
    }


    DecodeResult_t decode(StoredDataBits_t storedData) const override
    {
        DecodeResult_t result;
        auto corrupt = false;
        size_t corruptIdx = 0, position = 1; // note that using 1-based position, will need correcting when we flip

        // first, re-compute parity bits to check integrity of data
        compute_parity_bits(storedData, [&corrupt, &corruptIdx, &position](StoredDataBits_t& data, size_t parityIdx, bool parityVal)
        {
            // does calculated parity val match the expected one? if not, a corrupt bit is detected
            // note: we expect the correct bits to be set here for even parity. So we're always
            // expecting FALSE for parity val
            if (parityVal)  // parity bits of data are not correct!
                corrupt = true;

            // if we get non-zero parity, it is used to encode the position of the
            // (at least one) bad bit
            if (corrupt && parityVal)
                corruptIdx += position;

            position *= 2;
        });

        if (corrupt)
        {
            // correct the error (remember: corruptIdx is 1-based, bitset is 0-based)
            storedData.flip(corruptIdx - 1);

            // compute parity again. If it doesn't come out right, there's another
            // error in the data which we can't fix
            auto fixed = true;

            compute_parity_bits(storedData, [&fixed](StoredDataBits_t& data, size_t parityIdx, bool parityVal)
            {
                fixed = fixed && !parityVal;
            });

            result.num_corrected_bits = fixed ? 1 : 0;
            result.num_corrupt_bits = fixed ? 1 : 2;

            result.success = fixed;

        } else { // not corrupt

            result.num_corrupt_bits = 0;
            result.success = true;
        }


        // recover original data by throwing out the parity bits, which aren't needed anymore
        size_t dataIdx = 0;

        for (size_t i = 0; i < TOTAL_BIT_COUNT; ++i)
        {
            if (is_power_of_two(i + 1)) continue;

            result.decoded_bits[dataIdx++] = storedData.test(i);
        }

        return result;
    }
};
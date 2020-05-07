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


// Implements extended Hamming code (with extra parity bit on data at MSB)
template <size_t NumDataBits>
// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class HammingCode : public CorrectionStrategy<NumDataBits, NumDataBits + 1 + calc_hamming_code_check_bits(NumDataBits + 1)> // +1 data bits for extended hamming code
{
public:
    static constexpr size_t DATA_BIT_COUNT = NumDataBits + 1; // note: there is one extra parity bit over the Hamming code to allow for double error detection
    static constexpr size_t CHECK_BIT_COUNT = calc_hamming_code_check_bits(DATA_BIT_COUNT);
    static constexpr size_t TOTAL_BIT_COUNT = DATA_BIT_COUNT + CHECK_BIT_COUNT;

    typedef std::bitset<TOTAL_BIT_COUNT> StoredDataBits_t;
    typedef DecodeResult<NumDataBits, TOTAL_BIT_COUNT> DecodeResult_t;
    typedef std::bitset<NumDataBits + 1> DecodedBits_t;
    typedef std::function<void (StoredDataBits_t& data, size_t parityIdx, bool parityVal)> ParityCalcPostFunc_t;

private:
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
            const auto parityBit = counter % 2 == 1;

            func(encoded, mask - 1, parityBit); // parity bit actual idx is 0-based
        }
    }

    static bool is_power_of_two(size_t val)
    {
        return val && !(val & val - 1);
    }


    static DecodedBits_t fetch_decoded_data(StoredDataBits_t& encoded)
    {
        size_t dataIdx = 0;
        DecodedBits_t decoded;

        for (size_t i = 0; i < TOTAL_BIT_COUNT; ++i) {
            if (is_power_of_two(i + 1)) continue;

            decoded[dataIdx++] = encoded.test(i);
        }

        return decoded;
    }

public:
    StoredDataBits_t encode(const std::bitset<NumDataBits>& unencodedData) const override
    {
        // the parity bits for hamming code are actually interleaved among the
        // data bits in a pattern: each parity bit is at a power-of-two index - 1
        StoredDataBits_t encoded;

        // first, distribute data bits among correct locations
        // for the extended Hamming code, we're going to need to add one extra parity bit
        // as MSB of the data bits (so actually we're encoding NumDataBits + 1 bits)
        auto extendedData = BitStream<DATA_BIT_COUNT>::convert_bitset_to_bitset(unencodedData);
        extendedData[NumDataBits] = unencodedData.count() % 2 != 0; // add parity bit to data
            
        size_t idx_data = 0;

        for (size_t encode_idx = 0; encode_idx < TOTAL_BIT_COUNT; ++encode_idx)
        {
            // is this a Hamming parity bit location? (idx is a power of 2)
            // if so, ignore for now.
            // we assume 1-based array for this snippet
            if (is_power_of_two(encode_idx + 1))
                continue;
            
            // normal data bit
            encoded[encode_idx] = extendedData[idx_data++];
        }




        // compute Hamming parity bits and set them
        compute_parity_bits(encoded, [](StoredDataBits_t& data, size_t parityIdx, bool parityVal) { data[parityIdx] = parityVal; });

        return encoded;
    }


    DecodeResult_t decode(StoredDataBits_t storedData) const override
    {
        DecodeResult_t result;
        StoredDataBits_t corrected = storedData;
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

        if (corrupt && corruptIdx < TOTAL_BIT_COUNT) {
            // correct the error (remember: corruptIdx is 1-based, bitset is 0-based)
            corrected.flip(corruptIdx - 1);

#if _DEBUG
            // compute parity again. should be fixed, but this is not a guarantee the bits are correct
            auto fixed = true;

            compute_parity_bits(corrected, [&fixed](StoredDataBits_t& data, size_t parityIdx, bool parityVal)
            {
                fixed = fixed && !parityVal;
            });


            assert(fixed);
#endif
        }


        // recover original data by removing the Hamming parity bits, which aren't needed anymore
        auto decoded = fetch_decoded_data(corrected);

        // remember this is the extended Hamming code, so there's actually an extra bit at MSB position
        // which is a parity check. This is needed to perform double error detection, otherwise
        // Hamming code can't detect double errors
        auto recovered = BitStream<NumDataBits>::convert_bitset_to_bitset(decoded);

        // check the parity bit to make sure there wasn't a double error
        const auto paritySet = decoded.test(decoded.size() - 1);
        const auto needsParitySet = recovered.count() % 2 != 0;

        auto de = paritySet != needsParitySet; // double error test

        result.decoded_bits = recovered;
        result.success = !de;
        result.error_detected = corrupt;

        if (corrupt) {
            if (de) {
                result.num_corrupt_bits = 2;
                result.num_corrected_bits = 0;

                // our correction was meaningless, so return decoded bits with no attempted correction
                auto f = fetch_decoded_data(storedData);
                result.decoded_bits = BitStream<NumDataBits>::convert_bitset_to_bitset(f);
            } else {
                result.num_corrupt_bits = 1;
                result.num_corrected_bits = 1;
            }
        }

        return result;
    }
};

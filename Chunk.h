#pragma once
#include <bitset>
#include <cassert>
#include "BitStream.h"
#include "CorrectionStrategy.h"


/*
 * Represents a chunk of <num_bits> memory, with <check_bits> bytes of check bits
 */
template <size_t NumDataBits, size_t NumCheckBits = 1>
class Chunk
{
public:
    typedef std::bitset<NumDataBits> DataBits;
    typedef std::bitset<NumDataBits + NumCheckBits> StoredBits;
    typedef std::shared_ptr<CorrectionStrategy<NumDataBits, NumCheckBits>> StrategyPtr;


private:

    // note: first bit is LSB
    StoredBits m_stored;
    DataBits m_original; // uncorrupted original data
    StrategyPtr m_strategy;


    // erase this chunk
    void clear_contents()
    {
        m_original = DataBits();
        m_stored = m_strategy->encode(m_original);
    }



public:
    explicit Chunk(StrategyPtr strategy) : m_strategy(strategy)
    {
        assert(strategy.get() != nullptr);
        clear_contents();
    }


    // allows some type T to be stored in this memory chunk. T doesn't necessarily have to
    // take up all data bits
    void store(BitStream<NumDataBits>& bs)
    {
        m_stored = m_strategy->encode(bs);
        m_original = bs;
    }

    DecodeResult<NumDataBits, NumCheckBits> retrieve() const
    {
        auto result = m_strategy->decode(m_stored);

        // return the actual original data as well, in case error detection has
        // failed to detect an error and the data is actually corrupt
        result.original_bits = m_original;
        result.stored_bits = m_stored;

        result.success = result.success && result.original_bits == result.decoded_bits;

        return result;
    }


    void corrupt(size_t bit_idx)
    {
        m_stored.flip(bit_idx);
    }
};

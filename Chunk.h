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
    StrategyPtr m_strategy;


    // erase this chunk
    void clear_contents()
    {
        m_stored = m_strategy->encode(DataBits());
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
    }

    DecodeResult<NumDataBits, NumCheckBits> retrieve() const
    {
        return m_strategy->decode(m_stored);
    }


    void corrupt(size_t bit_idx)
    {
        m_stored.flip(bit_idx);
    }
};

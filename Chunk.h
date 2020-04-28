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



    //template <class T>
    //bool retrieve(T& buf) const
    //{

    //    assert(sizeof(T) * 8 <= NumDataBits);

    //    uint8_t buf[NumDataBits] = { 0 };

    //    read<T>(static_cast<void*>(buf), 0);

    //    // check bits by running encoder again
    //    const auto returnedCheck = m_encoder(m_contents);

    //    return returnedCheck == m_check;  // if no data corruption, check bits should match
    //}

    //// included for stuff that might be stored as arrays (for example: character arrays inside this chunk)
    //// meant for primitive types
    //// buf is where data will be copied to
    //template <class T>
    //void read(void* buf, size_t itemIdx) const
    //{
    //    assert(sizeof(T) * 8 * (itemIdx + 1) <= NumDataBits);

    //    const auto pLoc = reinterpret_cast<uint8_t*>(buf);

    //    for (size_t i = 0; i < sizeof(T); ++i)
    //    {
    //        // recover this byte
    //        uint8_t pVal = 0;

    //        for (size_t j = i * 8; j < (i + 1) * 8; ++j)
    //            pVal |= (m_contents.test(j) ? 1 : 0) << (j - i * 8);

    //        *(pLoc + i) = pVal;
    //    }
    //}

    //// note: bit 0 = LSB
    //typename ContentBits_t::reference operator[](size_t bit_idx)
    //{
    //    assert(bit_idx <= NumDataBits);

    //    return m_contents[bit_idx];
    //}

    //typename ContentBits_t::reference operator[](size_t bit_idx) const
    //{
    //    assert(bit_idx <= NumDataBits);

    //    return m_contents[bit_idx];
    //}

    //void clear()
    //{
    //    clear_contents();
    //}

    void corrupt(size_t bit_idx)
    {
        m_stored.flip(bit_idx);
    }
};

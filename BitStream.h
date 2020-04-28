#pragma once
#include <bitset>
#include <cstdint>
#include <cassert>


/*
 * The only purpose of this struct is to facilitate conversion between a bitset and actual POD types.
 * It's essentially an intermediate type that prevents Chunk and the correction strategy/result from
 * having to know much about what the actual data is, and can instead focus on manipulating bits
 */
template <int size>
struct BitStream : public std::bitset<size>
{
    BitStream() = default;
    BitStream(std::bitset<size> bits) : std::bitset<size>(bits) {}


    // create a bitstream from a single POD type
    template <class T>
    static BitStream from(T data)
    {
        static_assert(std::is_pod<T>::value, "Only primitive types allowed");

        assert(sizeof(T) * 8 <= size);

        const auto pVal = reinterpret_cast<const uint8_t*>(&data);

        return BitStream<size>::from(pVal, sizeof(T) * 8);
    }

    template <class T>
    static BitStream from(const T* buf, size_t length_bits)
    {
        return from_buffer(reinterpret_cast<const uint8_t*>(buf), length_bits);
    }


    // create a bitstream from a buffer of bytes, with given length in bits
    // (since there is no requirement that stored memory be in exact byte intervals
    // in this implementation, e.g. data could be 20 bits long)
    static BitStream<size> from_buffer(const uint8_t* buf, size_t length_bits)
    {
        assert(length_bits <= size);
        BitStream<size> bits;

        // operate per byte
        for (size_t bitIdx = 0; bitIdx < length_bits; bitIdx += 8)
        {
            // get byte we're working on
            auto byteVal = *(buf + bitIdx / 8);

            // 8 bits per byte
            for (size_t bit = 0; bit < 8 && (bitIdx + bit < length_bits); ++bit) {
                bits[bitIdx + bit] = (byteVal & 1) > 0;
                byteVal >>= 1;
            }
        }

        return bits;
    }

    template <class T>
    T to() const
    {
        static_assert(std::is_pod<T>::value, "Only primitive types allowed");
        assert(sizeof(T) * 8 <= size);

        auto result = to_many<T>(1);

        return result[0];
    }


    template <class T>
    std::shared_ptr<T[]> to_many(size_t count) const
    {
        static_assert(std::is_pod<T>::value, "Only primitive types allowed");
        assert(sizeof(T) * 8 * count <= size);

        auto buf = std::shared_ptr<T[]>(new T[count], std::default_delete<T[]>());
        const auto pbuf = reinterpret_cast<uint8_t*>(buf.get());

        memset(buf.get(), 0, sizeof(T) * count);

        for (size_t bitIdx = 0; bitIdx < size; bitIdx += 8)
        {
            uint8_t byteVal = 0;

            for (size_t bit = 0; bit < 8 && (bitIdx + bit < size); ++bit)
                byteVal |= (this->test(bitIdx + bit) ? 1 : 0) << bit;

            // don't overwrite data we're not meant to
            if (bitIdx / 8 < sizeof(T) * count)
                *(pbuf + bitIdx / 8) = byteVal;
        }

        //return *(reinterpret_cast<T*>(buf));
        return buf;
    }


    //template <int size>
    //static void bitset_to_raw(std::bitset<size> bits, uint8_t* buf)
    //{
    //    // assumption: buf is large enough to fit these bits

    //    for (size_t byteIdx = 0; byteIdx < size; byteIdx += 8)
    //    {
    //        uint8_t byteVal = 0;

    //        for (size_t bit = 0; bit < 8 && (byteIdx * 8 + bit < size); ++bit)
    //            byteVal |= (bits.test(byteIdx * 8 + bit) ? 1 : 0) << bit;

    //        *(buf + byteIdx) = byteVal;
    //    }

    //}
};
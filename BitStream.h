#pragma once
#include <bitset>
#include <cstdint>
#include <cassert>
#include <algorithm>


/*
 * The only purpose of this struct is to facilitate conversion between a bitset and actual POD types.
 * It's essentially an intermediate type that prevents Chunk and the correction strategy/result from
 * having to know much about what the actual data is, and can instead focus on manipulating bits
 */
template <size_t Size>
struct BitStream : public std::bitset<Size>
{
    BitStream() = default;
    BitStream(std::bitset<Size> bits) : std::bitset<Size>(bits) {}


    // create a bitstream from a single POD type
    template <class T>
    static BitStream from(T data)
    {
        static_assert(std::is_pod<T>::value, "Only primitive types allowed");

        assert(sizeof(T) * 8 <= Size);

        const auto pVal = reinterpret_cast<const uint8_t*>(&data);

        return from(pVal, sizeof(T) * 8);
    }

    template <class T>
    static BitStream from(const T* buf, size_t length_bits)
    {
        return from_buffer(reinterpret_cast<const uint8_t*>(buf), length_bits);
    }


    // create a bitstream from a buffer of bytes, with given length in bits
    // (since there is no requirement that stored memory be in exact byte intervals
    // in this implementation, e.g. data could be 20 bits long)
    static BitStream<Size> from_buffer(const uint8_t* buf, size_t length_bits)
    {
        assert(length_bits <= Size);
        BitStream<Size> bits;

        // operate per byte
        for (size_t bitIdx = 0; bitIdx < length_bits; bitIdx += 8)
        {
            // get byte we're working on
            auto byteVal = *(buf + bitIdx / 8);

            // 8 bits per byte (but don't go beyond length_bits total)
            for (size_t bit = 0; bit < 8 && (bitIdx + bit < length_bits); ++bit) {
                bits[bitIdx + bit] = (byteVal & 1) > 0;
                byteVal >>= 1;
            }
        }

        return bits;
    }


    // note: assumes buf is at least ceil(size / 8) bytes long
    void to_buffer(uint8_t* buf)
    {
        for (size_t bitIdx = 0; bitIdx < Size; bitIdx += 8)
        {
            uint8_t byteVal = 0;

            for (size_t bit = 0; bit < 8 && (bitIdx + bit < Size); ++bit)
                byteVal |= (this->test(bitIdx + bit) ? 1 : 0) << bit;

            *(buf + bitIdx / 8) = byteVal;
        }
    }


    template <class T>
    T to() const
    {
        auto result = to_many<T>(1);

        return result[0];
    }


    template <class T>
    std::shared_ptr<T[]> to_many(size_t count) const
    {
        static_assert(std::is_pod<T>::value, "Only primitive types allowed");
        assert(sizeof(T) * 8 * count <= Size);

        auto buf = std::shared_ptr<T[]>(new T[count], std::default_delete<T[]>());
        const auto pbuf = reinterpret_cast<uint8_t*>(buf.get());

        memset(buf.get(), 0, sizeof(T) * count);

        for (size_t bitIdx = 0; bitIdx < Size; bitIdx += 8)
        {
            uint8_t byteVal = 0;

            for (size_t bit = 0; bit < 8 && (bitIdx + bit < Size); ++bit)
                byteVal |= (this->test(bitIdx + bit) ? 1 : 0) << bit;

            // don't overwrite data we're not meant to
            // if within bit range specified by count and the actual size of each T
            if (bitIdx / 8 < sizeof(T) * count)
                *(pbuf + bitIdx / 8) = byteVal;
        }

        return buf;
    }


    // convert from given bitset to bitstream of template Size
    // useful to avoid some repetition
    template <class T>
    static std::bitset<Size> convert_bitset_to_bitset(const T& bitset)
    {
        std::bitset<Size> bits;

        for (size_t i = 0; i < std::min(Size, bitset.size()); ++i)
            bits[i] = bitset[i];

        return bits;
    }
};



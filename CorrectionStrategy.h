#pragma once
#include <bitset>
#include "DecodeResult.h"


template <int NumDataBits, int NumCheckBits>
// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
struct CorrectionStrategy
{
    typedef std::bitset<NumDataBits> DataBits;
    typedef std::bitset<NumDataBits + NumCheckBits> StoredBits;

    typedef DecodeResult<NumDataBits, NumCheckBits> DecodeResult;


    // data = data to generate check bit data from
    virtual std::bitset<NumDataBits + NumCheckBits> encode(const DataBits& data) const = 0;

    // storedData = data stored in memory. Might be corrupt. Was generated using encode()
    virtual DecodeResult decode(StoredBits storedData) const = 0;

};

/*-----------------------------------------------------------------------------
 * CorrectionStrategy.h
 *---------------------------------------------------------------------------*/
#pragma once
#include <bitset>
#include "DecodeResult.h"


template <size_t NumDataBits, size_t NumEncodedBits>
// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
struct CorrectionStrategy
{
    typedef std::bitset<NumDataBits> DataBits;
    typedef std::bitset<NumEncodedBits> StoredBits;

    typedef DecodeResult<NumDataBits, NumEncodedBits> DecodeResult;


    // data = data to generate check bit data from
    virtual std::bitset<NumEncodedBits> encode(const DataBits& data) const = 0;

    // storedData = data stored in memory. Might be corrupt. Was generated using encode()
    virtual DecodeResult decode(StoredBits storedData) const = 0;

};

/*/////////////////////////////////////////////////////////////////////////////
 * end CorrectionStrategy.h
 *///////////////////////////////////////////////////////////////////////////*/
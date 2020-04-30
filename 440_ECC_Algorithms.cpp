#include <iostream>
#include <memory>
#include "Chunk.h"
#include "ParityBit.h"
#include "HammingCode.h"

using std::cout;
using std::endl;


void parity()
{
    const char* buf2 = "Hello, world!";

    const auto size = 14 * 8;

    Chunk<size> chunk(Chunk<size, 1>::StrategyPtr(new ParityCheck<size>));

    auto bs = BitStream<size>::from(buf2, size);

    chunk.store(bs);;

    chunk.corrupt(22);

    auto result = chunk.retrieve();

    auto rbs = BitStream<size>(result.decoded_bits);

    auto final_result = rbs.to_many<char>(14);

    char buf[14] = { '\0' };

    memcpy(buf, final_result.get(), 14);
}


void hamming()
{
    int i = 0b0000000001011001; // 1011001

    //const auto size_bits = sizeof(i) * 8;
    const auto size_bits = 7; // just the 1010 part

    typedef HammingCode<size_bits> Hamming_t;
    typedef Chunk<Hamming_t::DATA_BIT_COUNT, Hamming_t::CHECK_BIT_COUNT> Chunk_t;

    const auto hammingStrategy = std::dynamic_pointer_cast<Chunk_t::StrategyPtr::element_type>(std::make_shared<Hamming_t>());


    auto chunk = Chunk_t(hammingStrategy);
    auto bs = BitStream<Hamming_t::DATA_BIT_COUNT>::from_buffer(reinterpret_cast<const uint8_t*>(&i), size_bits);

    chunk.store(bs);

    auto result = chunk.retrieve();
    auto rbs = BitStream<Hamming_t::DATA_BIT_COUNT>(result.decoded_bits);
    auto final_result = rbs.to<int>();

}

int main()
{
    parity();
    hamming();

}


#include <iostream>
#include "Chunk.h"
#include "ParityBit.h"

using std::cout;
using std::endl;

int main()
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

    char buf[14] = {'\0'};

    memcpy(buf, final_result.get(), 14);


}


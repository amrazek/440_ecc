#include <iostream>
#include <iomanip>
#include <memory>
#include <functional>
#include "Chunk.h"
#include "ParityBit.h"
#include "HammingCode.h"
#include "Tester.h"

using std::cout;
using std::endl;
using std::setw;

//
//void parity()
//{
//    const char* buf2 = "Hello, world!";
//
//    const auto size = 14 * 8;
//
//    Chunk<size> chunk(Chunk<size, size + 1>::StrategyPtr(new ParityCheck<size>));
//
//    auto bs = BitStream<size>::from(buf2, size);
//
//    chunk.store(bs);;
//
//    chunk.corrupt(22);
//
//    auto result = chunk.retrieve();
//
//    auto rbs = BitStream<size>(result.decoded_bits);
//
//    auto final_result = rbs.to_many<char>(14);
//
//    char buf[14] = { '\0' };
//
//    memcpy(buf, final_result.get(), 14);
//}


void hamming()
{
    int i = 0b0000000001011011; 

    //const auto size_bits = sizeof(i) * 8;
    const auto size_bits = 7; // just the 1010 part

    typedef HammingCode<size_bits> Hamming_t;
    typedef Chunk<size_bits, Hamming_t::TOTAL_BIT_COUNT> Chunk_t;

    const auto hammingStrategy = std::dynamic_pointer_cast<Chunk_t::StrategyPtr::element_type>(std::make_shared<Hamming_t>());


    for (size_t corruptIdx = 0; corruptIdx < Hamming_t::TOTAL_BIT_COUNT - 1; ++corruptIdx) {
        auto chunk = Chunk_t(hammingStrategy);
        auto bs = BitStream<size_bits>::from_buffer(reinterpret_cast<const uint8_t*>(&i), size_bits);

        chunk.store(bs);
        //chunk.corrupt(3);
        //chunk.corrupt(10);
        chunk.corrupt(corruptIdx);
        chunk.corrupt((corruptIdx + 1) % size_bits);

        const auto result = chunk.retrieve();
        auto rbs = BitStream<size_bits>(result.decoded_bits);

        // final result
        int j = 0;

        rbs.to_buffer(reinterpret_cast<uint8_t*>(&j));

        assert(!result.correct || !result.success);
    }
}


template <int data_bits>
void demo_hamming(uint8_t* data,
                  const std::function<void (uint8_t*)>& printFn, // don't print newlines
                  std::function<void (Chunk<data_bits, HammingCode<data_bits>::TOTAL_BIT_COUNT>&)> corruptFn)
{
    typedef HammingCode<data_bits> Hamming_t;
    const auto strategy = std::make_shared<Hamming_t>();
    const auto cout_width = Hamming_t::TOTAL_BIT_COUNT + 4;

    Chunk<data_bits, Hamming_t::TOTAL_BIT_COUNT> chunk(strategy);

    cout << std::left << setw(cout_width);

    // store original data as bitset
    printFn(data);
    cout << "original data\n";
    cout << "--------- Hamming Code ----------\n";
    cout << "Data bits: " << data_bits << " (+ 1 for extended Hamming code) = " << (data_bits + 1) << endl;
    cout << "Check bits: " << Hamming_t::CHECK_BIT_COUNT << " for " << Hamming_t::DATA_BIT_COUNT << " bits of data\n";
    cout << "Total: " << Hamming_t::TOTAL_BIT_COUNT << " bits to encode data" << endl << endl;

    auto original_bs = BitStream<data_bits>::from_buffer(reinterpret_cast<const uint8_t*>(data), data_bits);
    
    cout << setw(cout_width) << original_bs.to_string() << "original bits\n";

    // place bitset into chunk of memory
    chunk.store(original_bs);
    cout << setw(cout_width) << chunk.to_string() << "encoded using Hamming code\n";

    // corrupt memory
    auto uncorrupted_memory = chunk.get();
    corruptFn(chunk);
    auto corrupted_memory = chunk.get();
    cout << setw(cout_width) << chunk.to_string() << "(memory after possible corruption)\n";

    // note corrupted bits
    for (size_t i = 0; i < Hamming_t::TOTAL_BIT_COUNT; ++i)
    {
        const auto idx = Hamming_t::TOTAL_BIT_COUNT - i - 1;

        auto c = corrupted_memory.test(idx) != uncorrupted_memory.test(idx) ? "^" : " "; // MSB to LSB ordering
        cout << c;
    }
    cout << std::string(" ", cout_width - Hamming_t::TOTAL_BIT_COUNT) << "corrupted bits\n";

    // retrieve data bitstream
    auto result = chunk.retrieve();
    auto rbs = BitStream<data_bits>(result.decoded_bits);

    // store in new buffer
    const auto tmpBuf = std::shared_ptr<uint8_t[]>(new uint8_t[(data_bits + 8) / 8], std::default_delete<uint8_t>());
    memset(tmpBuf.get(), 0, (data_bits + 8) / 8);

    rbs.to_buffer(tmpBuf.get());

    // print retrieved value
    cout << setw(cout_width);
    printFn(tmpBuf.get());
    cout << "retrieved value\n";

    // print stats
    cout << "-- Summary --\n";
    cout << "Number of errors detected: " << result.num_corrupt_bits << endl;
    cout << "Number of errors corrected: " << result.num_corrected_bits << endl;
    cout << "Original data: ";    printFn(data); cout << endl;
    cout << "Decoded data: ";     printFn(tmpBuf.get()); cout << endl;
    cout << endl;
    cout << "Result correct: " << std::boolalpha << result.correct << endl;
    cout << "Error detected: " << std::boolalpha << result.error_detected << endl;
    cout << "Error corrected: " << std::boolalpha << (result.success && result.correct) << endl;
    
    cout << "Summary result: ";

    if (result.correct) {
        if (result.success)
        {
            if (result.error_detected)
                cout << "corruption was detected and corrected\n";
            else cout << "data was not corrupted\n";
        } else // implies algorithm failed and thought there was a double error
            cout << "Algorithm produced false positive on corruption\n";
    } else { // corruption exists
        if (!result.success)
        {
            assert(result.error_detected);
            cout << "corruption detected on multiple bits; uncorrectable\n";
        } else {
            // remember it's possible to get tricked by 3 or more errors
            if (!result.error_detected)
                cout << "corruption was not detected!\n";
            else if (result.num_corrupt_bits == result.num_corrected_bits)
                cout << "more than 2 errors have resulted in Hamming code failing\n";
            else cout << "corruption was detected, but was not correctable\n";
        } 
    } 
}


void print_int(uint8_t* p)
{
    cout << *(reinterpret_cast<int*>(p));
}

template <size_t num_data_bits, size_t num_encoded_bits>
void corrupt_int(Chunk<num_data_bits, num_encoded_bits>& memory)
{
    // corrupt third bit
    memory.corrupt(2);

    // corrupt bit
    memory.corrupt(4);

    memory.corrupt(9);
}

bool compare_int(uint8_t* p)
{
    return false;
}

int main()
{
    //parity();
    hamming();

    // test with an integer value
    int val = 123456;
    constexpr size_t num_data_bits = sizeof(val) * 8;

    typedef HammingCode<num_data_bits> Hamming_t;


    // demo hamming code
    demo_hamming<num_data_bits>(
        reinterpret_cast<uint8_t*>(&val),

        // print function
        print_int,
        corrupt_int<num_data_bits, Hamming_t::TOTAL_BIT_COUNT>);

    return 0;
}


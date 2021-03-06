/*-----------------------------------------------------------------------------
 * 440_ECC_Algorithms.h
 *---------------------------------------------------------------------------*/
#include <iostream>
#include <iomanip>
#include <memory>
#include <functional>
#include "Chunk.h"
#include "ParityBit.h"
#include "HammingCode.h"

using std::cout;
using std::endl;
using std::setw;


template <size_t encoded_bits>
void identify_corrupt_bits(size_t cout_width, const BitStream<encoded_bits>& original, const BitStream<encoded_bits>& maybeCorrupt) {
    cout << setw(cout_width) << maybeCorrupt.to_string() << "(memory after possible corruption)\n";

    // note corrupted bits
    for (size_t i = 0; i < encoded_bits; ++i)
    {
        const auto idx = encoded_bits - i - 1;

        auto c = original.test(idx) != maybeCorrupt.test(idx) ? "^" : " "; // MSB to LSB ordering
        cout << c;
    }
    cout << std::string(" ", cout_width - encoded_bits) << "corrupted bits\n";
}


template <size_t data_bits>
void demo_parity(uint8_t* data,
                 const std::function<void (uint8_t*)>& printFn,
                 std::function<void (Chunk<data_bits, data_bits + 1>&)> corruptFn)
{
    const auto strategy = std::make_shared<ParityBit<data_bits>>();
    const auto cout_width = data_bits + 4;
    Chunk<data_bits, data_bits + 1> chunk(strategy);

    cout << std::left << setw(cout_width); printFn(data); cout << "original data\n";
    cout << "---------- Parity Bit ------------------\n";
    cout << "Data bits: " << data_bits << endl;
    cout << "Check bits: 1\n";
    cout << "Total bits: " << (data_bits + 1) << endl;

    auto original_bs = BitStream<data_bits>::from_buffer(reinterpret_cast<const uint8_t*>(data), data_bits);
    cout << setw(cout_width) << original_bs.to_string() << "original bits\n";

    // place bitset into chunk of memory
    chunk.store(original_bs);
    cout << setw(cout_width) << chunk.to_string() << "encoded using parity bit\n";

    // corrupt memory
    auto uncorrupted_memory = chunk.get();
    corruptFn(chunk);
    auto corrupted_memory = chunk.get();
    identify_corrupt_bits<data_bits + 1>(cout_width, uncorrupted_memory, corrupted_memory);

    // retrieve data bitstream (attempt to recover memory)
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
    cout << "Correct data retrieved: " << std::boolalpha << result.correct << endl;
    cout << "Error detected: " << std::boolalpha << result.error_detected << endl;
    cout << "Error corrected: N/A\n";

    cout << "Summary result: ";

    // no correction capability or detection for > 1 bit
    assert(result.num_corrected_bits == 0);
    assert(result.num_corrupt_bits <= 1);

    if (result.correct)
    {
        assert(result.success);
        cout << "no memory error" << endl;
        
    } else
    {
        if (result.success)
            cout << "parity check failed to identify corrupt memory" << endl;
        else cout << "parity check identified corrupt memory" << endl;
    }

    cout << "---------- end parity check ----------- \n" << endl;
}


template <size_t data_bits>
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
    cout << "---------- Hamming Code ----------------\n";
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
    identify_corrupt_bits(cout_width, uncorrupted_memory, corrupted_memory);

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
    cout << "Correct data retrieved: " << std::boolalpha << result.correct << endl;
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

    cout << endl;
    cout << "---------- end Hamming code ----------- \n" << endl;
}


template <class T>
void print_data(uint8_t* const p)
{
    static_assert(std::is_pod<T>::value, "must be POD type");
    cout << *(reinterpret_cast<T*>(p));
}



// parity bit example used in paper - no corruption
void example_parity_1()
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef Chunk<data_bits, data_bits + 1> Chunk_t;

    // no corruption
    demo_parity<data_bits>(reinterpret_cast<uint8_t*>(&val), print_data<char>, 
        [](Chunk_t& memory) {
            // nop
        });
}

// parity bit example used in paper - bit 1 corrupted
void example_parity_2() 
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef Chunk<data_bits, data_bits + 1> Chunk_t;

    // corruption on bit 1
    demo_parity<data_bits>(reinterpret_cast<uint8_t*>(&val), print_data<char>,
        [](Chunk_t& memory) {
            memory.corrupt(1);
        });
}

// parity bit example used in paper - bits 1 and 5 corrupted
void example_parity_3()
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef Chunk<data_bits, data_bits + 1> Chunk_t;

    // corruption on bit 1 and 5
    demo_parity<data_bits>(reinterpret_cast<uint8_t*>(&val), print_data<char>,
        [](Chunk_t& memory) {
            memory.corrupt(1);
            memory.corrupt(5);
        });
}


// hamming code example used in paper - no corruption
void example_hamming_1()
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef HammingCode<data_bits> Hamming_t;

    demo_hamming<data_bits>(
        reinterpret_cast<uint8_t*>(&val),

        // print function
        print_data<char>,
        [](Hamming_t::Chunk_t& memory)
    {
        // nop
    });
}

// hamming code example used in paper - bit 1 corrupted
void example_hamming_2()
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef HammingCode<data_bits> Hamming_t;

    demo_hamming<data_bits>(
        reinterpret_cast<uint8_t*>(&val),

        // print function
        print_data<char>,
        [](Hamming_t::Chunk_t& memory)
        {
            memory.corrupt(1);
        });
}

// hamming code example used in paper - bits 1 and 5 corrupted
void example_hamming_3()
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef HammingCode<data_bits> Hamming_t;

    demo_hamming<data_bits>(
        reinterpret_cast<uint8_t*>(&val),

        // print function
        print_data<char>,
        [](Hamming_t::Chunk_t& memory)
        {
            memory.corrupt(1);
            memory.corrupt(5);
        });
}

// hamming code example used in paper - bits 1, 5 and 9 corrupted
void example_hamming_4()
{
    char val = 'p'; // use only 7 bits - 111 0000
    constexpr auto data_bits = 7;
    typedef HammingCode<data_bits> Hamming_t;

    demo_hamming<data_bits>(
        reinterpret_cast<uint8_t*>(&val),

        // print function
        print_data<char>,
        [](Hamming_t::Chunk_t& memory)
        {
            memory.corrupt(1);
            memory.corrupt(5);
            memory.corrupt(9);
        });
}


void hamming();

int main() 
{
    example_parity_1();
    example_parity_2();
    example_parity_3();

    example_hamming_1();
    example_hamming_2();
    example_hamming_3();
    example_hamming_4();

    return 0;
}


// used to test Hamming code 
void hamming()
{
    int i = 0b0000000001011011;

    const auto size_bits = 7; // just the 1011011 part

    typedef HammingCode<size_bits> Hamming_t;
    typedef Chunk<size_bits, Hamming_t::TOTAL_BIT_COUNT> Chunk_t;

    const auto hammingStrategy = std::dynamic_pointer_cast<Chunk_t::StrategyPtr::element_type>(std::make_shared<Hamming_t>());


    for (size_t corruptIdx = 0; corruptIdx < Hamming_t::TOTAL_BIT_COUNT - 1; ++corruptIdx) {
        auto chunk = Chunk_t(hammingStrategy);
        auto bs = BitStream<size_bits>::from_buffer(reinterpret_cast<const uint8_t*>(&i), size_bits);

        chunk.store(bs);
        chunk.corrupt(corruptIdx);
        chunk.corrupt((corruptIdx + 1) % size_bits);

        const auto result = chunk.retrieve();
        auto rbs = BitStream<size_bits>(result.decoded_bits);

        // final result
        int j = 0;

        rbs.to_buffer(reinterpret_cast<uint8_t*>(&j));

        assert(!result.correct || !result.success);
        assert(!result.success);
    }
}

/*/////////////////////////////////////////////////////////////////////////////
 * end 440_ECC_Algorithms.h
 *///////////////////////////////////////////////////////////////////////////*/
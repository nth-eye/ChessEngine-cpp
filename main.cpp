#include <ctime>
#include <chrono>
#include "log.h"
#include "board.h"
#include "test.h"

// Measure execution time of a function.
template<size_t N = 1, bool Avg = true, class Fn, class ...Args>
clock_t measure_time(Fn &&fn, Args &&...args)
{
    clock_t begin = clock();
    for (size_t i = 0; i < N; ++i) 
        fn(args...);
    clock_t end = clock();
    clock_t res = end - begin;
    return Avg ? res / N : res;
}

int main(int, char**) 
{
    // Board board;

    // if (board.set_pos(FEN_START)) // "r3kbnr/p7/1p1B1q2/3b1p1P/PpPp4/2Q4N/3PPP1P/RN2KB1R b KQkq -"
    //     board.print();
    // else 
    //     LOG("set_pos: failed \n");

    // auto depth = 8;

    // LOG("\nStarting test to depth %d \n", depth);

    // auto start = std::chrono::steady_clock::now();
    // auto all_nodes = perft<true>(board, depth);
    // auto time = std::chrono::steady_clock::now() - start;

    // LOG("\nTest completed: %lu nodes visited in %f ms \n", all_nodes, time.count() / 1'000'000.0);

    // printf("test:   %lu clock_t \n", measure_time<10000>(perft, board, depth));

    test_perft("../perft.txt");
}

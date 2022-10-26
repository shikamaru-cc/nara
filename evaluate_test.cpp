#include <iostream>

#include "bench.hpp"
#include "board.hpp"
#include "evaluate.hpp"

int main() {
    // EXXEOOOEEOOOEOE
    // EOOEXXXEEXXXEXE
    /*
    std::vector<nara::gomoku_chess> vec{
        nara::EMPTY,
        nara::BLACK,
        nara::BLACK,
        nara::EMPTY,
        nara::WHITE,
        nara::WHITE,
        nara::WHITE,
        nara::EMPTY,
        nara::EMPTY,
        nara::WHITE,
        nara::WHITE,
        nara::WHITE,
        nara::EMPTY,
        nara::WHITE,
        nara::EMPTY
    };

    auto [res_blk, res_wht] = nara::evaluate(vec);
    nara::debug_result(std::cout, res_blk);
    nara::debug_result(std::cout, res_wht);
    */

    auto start = std::chrono::system_clock::now();
    auto res = nara::evaluate("EXXEXXEXEXEXXEO");
    auto end = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start)<< '\n';

    nara::debug_result(std::cout, res);

    res = nara::evaluate("EXXXXOOEXXXXOOE");
    nara::debug_result(std::cout, res);
}

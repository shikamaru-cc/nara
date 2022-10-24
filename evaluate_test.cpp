#include <iostream>

#include "bench.hpp"
#include "board.hpp"
#include "evaluate.hpp"

int main() {
    /*
    for(auto const & p : nara::patterns) {
        std::cout << p.score << '\n';
    }
    */

    // EXXEOOOEEXXXOOE
    // EOOEXXXEEOOOXXE
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
        nara::BLACK,
        nara::BLACK,
        nara::BLACK,
        nara::WHITE,
        nara::WHITE,
        nara::EMPTY
    };

    /*
    nara::eval_result res_blk, res_wht;
    auto used = benchmark([&]{
        auto [blk, wht] = nara::evaluate(vec);
        res_blk = blk;
        res_wht = wht;
    });
    */

    auto [res_blk, res_wht] = nara::evaluate(vec);
    // std::cout << "eval use " << used << '\n';
    nara::debug_result(std::cout, res_blk);
    nara::debug_result(std::cout, res_wht);
}

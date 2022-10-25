#pragma once

#include <cassert>
#include <regex>
#include <tuple>
#include <vector>
#include <ranges>
#include <string>
#include <iostream>
#include <algorithm>

#include "board.hpp"

namespace nara {

enum pattern_type { SICK2 = 0, FLEX2, SICK3, FLEX3, SICK4, FLEX4, FLEX5 };

const static int pattern_score[7] = {
    10, 100, 100, 1000, 1000, 10000, 100000
};

struct pattern {
    std::regex re;
    pattern_type t;
};

std::vector<pattern> patterns{
    {std::regex("OXXE"), SICK2},
    {std::regex("EXXO"), SICK2},

    {std::regex("EXXE"), FLEX2},
    {std::regex("EXEXE"), FLEX2},

    {std::regex("EXXXO"), SICK3},
    {std::regex("OXXXE"), SICK3},
    {std::regex("OXXEXE"), SICK3},
    {std::regex("EXXEXO"), SICK3},
    {std::regex("OXEXXE"), SICK3},
    {std::regex("EXEXXO"), SICK3},

    {std::regex("EXXXE"), FLEX3},
    {std::regex("EXXEXE"), FLEX3},
    {std::regex("EXEXXE"), FLEX3},

    {std::regex("EXXXXO"), SICK4},
    {std::regex("OXXXXE"), SICK4},
    {std::regex("XXEXX"), SICK4},

    {std::regex("EXXXXE"), FLEX4},

    {std::regex("XXXXX"), FLEX5},
};

struct eval_result {
    int pattern_cnt[7];
    int score;
    eval_result(): pattern_cnt({0}), score(0) {}
};

int search_all(std::string const & line, std::regex const & re) {
    std::string subject(line);
    std::smatch m;
    int cnt = 0;
    while(std::regex_search(subject, m, re)) {
        cnt++;
        subject = m.suffix().str();
    }
    return cnt;
}

eval_result evaluate_wrap(std::string const & wrapped) {
    eval_result res;
    for(auto const & p : patterns) {
        int nmatch = search_all(wrapped, p.re);
        res.pattern_cnt[p.t] += nmatch;
        res.score += pattern_score[p.t] * nmatch;
    }
    return res;
}

eval_result evaluate(std::string const & line) {
    return evaluate_wrap("O" + line + "O");
}

char chess2ch4black(gomoku_chess chess) {
    assert(chess != OUTBX);
    switch(chess) {
    case BLACK: return 'X';
    case WHITE: return 'O';
    default: return 'E';
    }
}

char chess2ch4white(gomoku_chess chess) {
    assert(chess != OUTBX);
    switch(chess) {
    case WHITE: return 'X';
    case BLACK: return 'O';
    default: return 'E';
    }
}

// return eval_result black, eval_result white
std::tuple<eval_result, eval_result>
evaluate(std::vector<gomoku_chess> const & chesses) {
    if(chesses.size() < 5)
        return std::make_tuple(eval_result{}, eval_result{});

    std::string black;
    std::ranges::copy(
        chesses | std::views::transform(chess2ch4black),
        std::back_inserter(black)
    );

    std::string white;
    std::ranges::copy(
        chesses | std::views::transform(chess2ch4white),
        std::back_inserter(white)
    );

    return std::make_tuple(evaluate(black), evaluate(white));
}

void debug_result(std::ostream & os, eval_result const & res) {
    os << "result score: " << res.score << '\n';
    os << "SICK2: " << res.pattern_cnt[nara::SICK2] << '\n';
    os << "FLEX2: " << res.pattern_cnt[nara::FLEX2] << '\n';
    os << "SICK3: " << res.pattern_cnt[nara::SICK3] << '\n';
    os << "FLEX3: " << res.pattern_cnt[nara::FLEX3] << '\n';
    os << "SICK4: " << res.pattern_cnt[nara::SICK4] << '\n';
    os << "FLEX4: " << res.pattern_cnt[nara::FLEX4] << '\n';
    os << "FLEX5: " << res.pattern_cnt[nara::FLEX5] << '\n';
}

} // namespace nara

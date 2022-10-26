#pragma once

#include <cassert>
#include <regex>
#include <tuple>
#include <vector>
#include <ranges>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_set>

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

std::vector<pattern> patterns {
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

    {std::regex("EXXXXE"), FLEX4},

    {std::regex("XXXXX"), FLEX5},
};

struct eval_result {
    int pattern_cnt[7];
    int score;

    std::vector<int> to_five;
    std::vector<int> to_flex4;

    eval_result(): pattern_cnt({0}), score(0) {}
};

int search_and_push(
    std::string const & line,
    std::vector<int> & to_push,
    std::regex re,
    std::vector<int> const & idxs)
{
    int si = 0;
    int cnt = 0;
    std::smatch m;
    std::string s = line;

    while(std::regex_search(s, m, re))
    {
        for(int idx : idxs)
            to_push.emplace_back(si + m.position() + idx);

        cnt++;
        si += m.position() + m.length();
        s = s.substr(m.position() + m.length());
    }

    return cnt;
}

int search_flex3(std::string const & line, std::vector<int> & to_flex4)
{
    int cnt = 0;
    cnt += search_and_push(line, to_flex4, std::regex("EXXXE"), {0, 4});
    cnt += search_and_push(line, to_flex4, std::regex("EXXEXE"), {0, 3, 5});
    cnt += search_and_push(line, to_flex4, std::regex("EXEXXE"), {0, 2, 5});
    return cnt;
}

int search_sick4(std::string const & line, std::vector<int> & to_five)
{
    int cnt = 0;
    cnt += search_and_push(line, to_five, std::regex("EXXXXO"), {0});
    cnt += search_and_push(line, to_five, std::regex("OXXXXE"), {5});
    cnt += search_and_push(line, to_five, std::regex("XXEXX"), {2});
    return cnt;
}

int search_all(std::string const & line, std::regex const & re)
{
    std::string subject(line);
    std::smatch m;
    int cnt = 0;
    while(std::regex_search(subject, m, re))
    {
        cnt++;
        subject = m.suffix().str();
    }
    return cnt;
}

eval_result evaluate_wrap(std::string const & wrapped)
{
    eval_result res;

    int flex3_cnt = search_flex3(wrapped, res.to_flex4);
    res.pattern_cnt[FLEX3] += flex3_cnt;
    res.score += pattern_score[FLEX3] * flex3_cnt;

    int sick4_cnt = search_sick4(wrapped, res.to_five);
    res.pattern_cnt[SICK4] += sick4_cnt;
    res.score += pattern_score[SICK4] * sick4_cnt;

    for(auto const & p : patterns)
    {
        int nmatch = search_all(wrapped, p.re);
        res.pattern_cnt[p.t] += nmatch;
        res.score += pattern_score[p.t] * nmatch;
    }

    return res;
}

void to_unique(std::vector<int> & vec)
{
    std::unordered_set<int> s;
    for (int i : vec)
        s.insert(i);
    vec.assign(s.begin(), s.end());
}

eval_result evaluate(std::string const & line)
{
    auto res = evaluate_wrap("O" + line + "O");

    to_unique(res.to_flex4);
    to_unique(res.to_five);

    auto dec = [](int & i){ i--; };
    std::ranges::for_each(res.to_flex4, dec);
    std::ranges::for_each(res.to_five, dec);

    return res;
}

char chess2ch4black(gomoku_chess chess)
{
    switch(chess) {
    case BLACK: return 'X';
    case WHITE: return 'O';
    default: return 'E';
    }
}

char chess2ch4white(gomoku_chess chess)
{
    switch(chess) {
    case WHITE: return 'X';
    case BLACK: return 'O';
    default: return 'E';
    }
}

// return eval_result black, eval_result white
std::tuple<eval_result, eval_result>
evaluate(std::vector<gomoku_chess> const & chesses)
{
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

void debug_result(std::ostream & os, eval_result const & res)
{
    os << "result score: " << res.score << '\n';
    os << "SICK2: " << res.pattern_cnt[nara::SICK2] << '\n';
    os << "FLEX2: " << res.pattern_cnt[nara::FLEX2] << '\n';
    os << "SICK3: " << res.pattern_cnt[nara::SICK3] << '\n';
    os << "FLEX3: " << res.pattern_cnt[nara::FLEX3] << '\n';
    os << "SICK4: " << res.pattern_cnt[nara::SICK4] << '\n';
    os << "FLEX4: " << res.pattern_cnt[nara::FLEX4] << '\n';
    os << "FLEX5: " << res.pattern_cnt[nara::FLEX5] << '\n';

    os << "to_flex4: ";
    for(auto i : res.to_flex4)
        os << i << ' ';
    os << '\n';

    os << "to_five: ";
    for(auto i : res.to_five)
        os << i << ' ';
    os << '\n';
}

} // namespace nara

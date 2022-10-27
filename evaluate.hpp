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

struct line_t {
    point_t ori;
    point_t dir;
};

std::vector<line_t> all_lines;

void gen_lines()
{
    point_t origin, dir;

    // line '-'
    dir = {0, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++)
    {
        origin = {i, 0};
        all_lines.push_back(line_t{origin, dir});
    }

    // line '|'
    dir = {1, 0};
    for(int i = 0; i < gomoku_board::WIDTH; i++)
    {
        origin = {0, i};
        all_lines.push_back(line_t{origin, dir});
    }

    // line '/'
    dir = {-1, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++)
    {
        origin = {i, 0};
        all_lines.push_back(line_t{origin, dir});
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++)
    {
        origin = {14, i};
        all_lines.push_back(line_t{origin, dir});
    }

    // line '\'
    dir = {1, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++)
    {
        origin = {i, 0};
        all_lines.push_back(line_t{origin, dir});
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++)
    {
        origin = {14, i};
        all_lines.push_back(line_t{origin, dir});
    }
}

enum pattern_type { SICK2 = 0, FLEX2, SICK3, FLEX3, SICK4, FLEX4, FLEX5 };

static constexpr int pattern_score[7] = {
    10, 100, 100, 1000, 1000, 10000, 1000000
};

constexpr int score_win = pattern_score[FLEX5];

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

// for debug
static int last_eval_times;

auto evaluate_line(gomoku_board const & board, line_t line)
{
    last_eval_times++;

    // auto start = std::chrono::system_clock::now();

    std::vector<nara::gomoku_chess> chesses;

    point_t ori= line.ori;
    point_t dir = line.dir;
    point_t p = ori;
    while(!gomoku_board::outbox(p.x, p.y))
    {
        switch(board.getchess(p.x, p.y)) {
        case BLACK: chesses.push_back(nara::BLACK); break;
        case WHITE: chesses.push_back(nara::WHITE); break;
        case EMPTY: chesses.push_back(nara::EMPTY); break;
        }
        p = {p.x + dir.x, p.y + dir.y};
    }

    /*
    auto end = std::chrono::system_clock::now();
    logger << "evaluate_line use "
           << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
           << "\n";
    logger.flush();
    */

    return nara::evaluate(chesses);
}

auto evaluate_board(gomoku_board const & board)
{
    if(all_lines.empty()) gen_lines();

    std::vector<nara::eval_result> results_blk;
    std::vector<nara::eval_result> results_wht;
    for(auto line : all_lines) {
        auto [res_blk, res_wht] = evaluate_line(board, line);
        results_blk.push_back(std::move(res_blk));
        results_wht.push_back(std::move(res_wht));
    }
    return std::make_tuple(results_blk, results_wht);
}

} // namespace nara

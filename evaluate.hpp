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

bool operator == (const line_t l1, const line_t l2)
{
    return l1.ori == l2.ori && l1.dir == l2.dir;
}

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
        origin = {0, i};
        all_lines.push_back(line_t{origin, dir});
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++)
    {
        origin = {i, 0};
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

    {std::regex("XXXXX"), FLEX5},
};

struct eval_result {
    int pattern_cnt[7];
    int score;

    std::vector<int> to_sick4;
    std::vector<int> to_flex4;
    std::vector<int> to_five;

    eval_result(): pattern_cnt({0}), score(0) {}

    void add(pattern_type pt, int n)
    {
        pattern_cnt[pt] += n;
        score += n * pattern_score[pt];
    }
};

int search_and_push(
    std::string const & line,
    std::vector<int> & to_push,
    std::regex const & re,
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

int search_sick3(std::string const & line, std::vector<int> & to_sick4)
{
    static const auto EXXXO = std::regex("EXXXO");
    static const auto OXXXE = std::regex("OXXXE");
    static const auto OXXEXE = std::regex("OXXEXE");
    static const auto EXXEXO = std::regex("EXXEXO");
    static const auto OXEXXE = std::regex("OXEXXE");
    static const auto EXEXXO = std::regex("EXEXXO");

    int cnt = 0;
    cnt += search_and_push(line, to_sick4, EXXXO, {0});
    cnt += search_and_push(line, to_sick4, OXXXE, {4});
    cnt += search_and_push(line, to_sick4, OXXEXE, {3, 5});
    cnt += search_and_push(line, to_sick4, EXXEXO, {0, 3});
    cnt += search_and_push(line, to_sick4, OXEXXE, {2, 5});
    cnt += search_and_push(line, to_sick4, EXEXXO, {0, 2});
    return cnt;
}

int search_flex3(std::string const & line, std::vector<int> & to_flex4)
{
    static const auto EXXXE = std::regex("EXXXE");
    static const auto EXXEXE = std::regex("EXXEXE");
    static const auto EXEXXE = std::regex("EXEXXE");

    int cnt = 0;
    cnt += search_and_push(line, to_flex4, EXXXE, {0, 4});
    cnt += search_and_push(line, to_flex4, EXXEXE, {0, 3, 5});
    cnt += search_and_push(line, to_flex4, EXEXXE, {0, 2, 5});
    return cnt;
}

int search_sick4(std::string const & line, std::vector<int> & to_five)
{
    static const auto EXXXXO = std::regex("EXXXXO");
    static const auto OXXXXE = std::regex("OXXXXE");
    static const auto XXEXX = std::regex("XXEXX");
    static const auto XXXEX = std::regex("XXXEX");
    static const auto XEXXX = std::regex("XEXXX");

    int cnt = 0;
    cnt += search_and_push(line, to_five, EXXXXO, {0});
    cnt += search_and_push(line, to_five, OXXXXE, {5});
    cnt += search_and_push(line, to_five, XXEXX, {2});
    cnt += search_and_push(line, to_five, XXXEX, {3});
    cnt += search_and_push(line, to_five, XEXXX, {1});
    return cnt;
}

int search_flex4(std::string const & line, std::vector<int> & to_five)
{
    return search_and_push(line, to_five, std::regex("EXXXXE"), {0, 5});
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

    res.add(SICK3, search_sick3(wrapped, res.to_sick4));
    res.add(FLEX3, search_flex3(wrapped, res.to_flex4));
    res.add(SICK4, search_sick4(wrapped, res.to_five));
    res.add(FLEX4, search_flex4(wrapped, res.to_five));

    for(auto const & p : patterns)
        res.add(p.t, search_all(wrapped, p.re));

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
    std::ranges::for_each(res.to_sick4, dec);
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

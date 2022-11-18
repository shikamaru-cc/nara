#pragma once

#include <array>
#include <cstdint>
#include <cmath>
#include <numeric>

#include "log.hpp"
#include "board.hpp"

namespace nara
{

enum category_t
{
    NONE = 0,
    BLOCK1,
    FLEX1,
    BLOCK2,
    FLEX2,
    BLOCK3,
    FLEX3,
    BLOCK4,
    FLEX4,
    FIVE,
};

const int score_win = 1000000;
const int score_lose = -score_win;

constexpr bool is_empty(uint8_t px, uint8_t py, int at)
{
    return (px & (1 << at)) == 0 and (py & (1 << at)) == 0;
}

constexpr bool is_category(uint8_t px, uint8_t py, int cat);
constexpr int step_to(uint8_t px, uint8_t py, int c)
{
    int cnt = 0;
    for (int i = 0; i < 8; i++)
    {
        if (is_empty(px, py, i))
        {
            uint8_t px_new = px | (1 << i);
            if (is_category(px_new, py, c))
                cnt++;
        }
    }
    return cnt;
}

constexpr bool has_cont_five(uint8_t px, uint8_t py)
{
    int cnt = 0;
    for (uint8_t i = 1 << 4; i > 0; i <<= 1)
    {
        if ((px & i) > 0 and (py & i) == 0)
            cnt++;
        else
            break;
    }
    for (uint8_t i = 1 << 3; i > 0; i >>= 1)
    {
        if ((px & i) > 0 and (py & i) == 0)
            cnt++;
        else
            break;
    }
    return cnt >= 4;
}

constexpr bool is_category(uint8_t px, uint8_t py, int cat)
{
    switch (cat)
    {
    case FIVE:      return has_cont_five(px, py);
    case BLOCK4:    return step_to(px, py, FIVE) == 1;
    case FLEX4:     return step_to(px, py, FIVE) > 1;
    default:        return step_to(px, py, cat + 2) > 0;
    }
}

constexpr int cal_category(uint8_t px, uint8_t py)
{
    for (int c = FIVE; c > NONE; c--)
        if (is_category(px, py, c))
            return c;
    return NONE;
}

int bit_count(uint8_t x)
{
    int cnt = 0;
    while (x)
    {
        cnt += x & 1;
        x >>= 1;
    }
    return cnt;
}

using line_pattern = std::array<uint8_t, 2>;

int cal_rank(uint8_t px, uint8_t py)
{
    static const int rank[5] = {1, 4, 9, 16, 25};
    static const uint8_t masks[5] = {0b11110000, 0b01111000, 0b00111100, 0b00011110, 0b00001111};
    int val = 0;
    for (uint8_t mask : masks)
        if ((mask & py) == 0)
            val += rank[bit_count(mask & px)];
    return val;
}

int cal_rank(line_pattern p) { return cal_rank(p[0], p[1]); }

constexpr auto gen_category_table()
{
    std::array<std::array<int, 256>, 256> table{};

    for (int px = 0; px < 256; px++)
        for (int py = 0; py < 256; py++)
            table[px][py] = cal_category(px, py);

    return table;
}

constexpr std::array<std::array<int, 256>, 256> category_table = gen_category_table();

constexpr int get_category(uint8_t px, uint8_t py) { return category_table[px][py]; }

constexpr int get_category(line_pattern p) { return get_category(p[0], p[1]); }

struct chess_state
{
    int neighbors[4];

    line_pattern pattern_blk[4];
    line_pattern pattern_wht[4];

    using all_category = std::array<int, 10>;

    all_category cats_blk[4];
    all_category cats_wht[4];

    line_pattern get_pattern(gomoku_chess chess, int dir)
    {
        assert(chess == WHITE or chess == BLACK);
        return (chess == BLACK) ? pattern_blk[dir] : pattern_wht[dir];
    }

    void set_pattern(gomoku_chess chess, int dir, uint8_t px, uint8_t py)
    {
        auto & p = (chess == BLACK) ? pattern_blk[dir] : pattern_wht[dir];
        p[0] = px;
        p[1] = py;
    }

    void update_chess(gomoku_chess chess, int dir, int step)
    {
        assert(step != 0);

        int mask = (step > 0) ? (1 << 4) >> step : (1 << 3) << (-step);

        if (chess == EMPTY)
        {
            pattern_blk[dir][0] &= ~mask;
            pattern_blk[dir][1] &= ~mask;
            pattern_wht[dir][0] &= ~mask;
            pattern_wht[dir][1] &= ~mask;

            if (std::abs(step) <= 2)
                neighbors[dir]--;
        }
        else
        {
            if (chess == BLACK)
            {
                pattern_blk[dir][0] |= mask;
                pattern_wht[dir][1] |= mask;
            }
            else
            {
                pattern_blk[dir][1] |= mask;
                pattern_wht[dir][0] |= mask;
            }

            if (std::abs(step) <= 2)
                neighbors[dir]++;
        }

        update_cats(dir);
    }

    int rankof(gomoku_chess chess)
    {
        int val = 0;
        for (size_t i = 0; i < 4; i++)
        {
            if (chess == BLACK)
                val += cal_rank(pattern_blk[i]);
            else
                val += cal_rank(pattern_wht[i]);
        }
        return val;
    }

    bool has_neighbor()
    {
        return neighbors[0] > 0 or neighbors[1] > 0 or neighbors[2] > 0 or neighbors[3] > 0;
    }

    bool has_category(gomoku_chess for_chess, int cat)
    {
        for (int dir = 0; dir < 4; dir++)
        {
            auto p = get_pattern(for_chess, dir);
            if (cal_category(p[0], p[1]) == cat)
                return true;
        }
        return false;
    }

    void update_cats(int dir)
    {
        cats_blk[dir] = cats_wht[dir] = {0};

        line_pattern p;

        p = get_pattern(BLACK, dir);
        cats_blk[dir][get_category(p)]++;

        p = get_pattern(WHITE, dir);
        cats_wht[dir][get_category(p)]++;
    }

    void update_cats()
    {
        for (int dir = 0; dir < 4; dir++)
            update_cats(dir);
    }

    static std::array<int, 10> sum_cats(std::array<int, 10> prev, const std::array<int, 10> & one)
    {
        for (int i = 0; i < 10; i++)
            prev[i] += one[i];
        return prev;
    }

    auto cats_for(gomoku_chess chess)
    {
        const auto & cats = (chess == BLACK) ? cats_blk : cats_wht;
        all_category ret = {0};

        for (int dir = 0; dir < 4; dir++)
            for (int cat = 0; cat < 10; cat++)
                ret[cat] += cats[dir][cat];

        return ret;
    }
};

// bool state_equal(chess_state s1, chess_state s2)
// {
//     for (int i = 0; i < 4; i++)
//     {
//         if (s1.neighbors[i] != s2.neighbors[i] or
//             s1.pattern_blk[i].px != s2.pattern_blk[i].px or
//             s1.pattern_blk[i].py != s2.pattern_blk[i].py or
//             s1.pattern_wht[i].px != s2.pattern_wht[i].px or
//             s1.pattern_wht[i].py != s2.pattern_wht[i].py)
//             return false;
//     }
//     return true;
// }

chess_state get_state(gomoku_board const& board, point_t pos)
{
    chess_state ret;
    for (int dir = 0; dir < 4; dir++)
    {
        int neigh_cnt = 0;
        uint8_t blk_px, blk_py, wht_px, wht_py;
        blk_px = blk_py = wht_px = wht_py = 0;
        int i = 7;
        for (int fac = -4; fac <= 4; fac++)
        {
            if (fac == 0) continue;

            point_t p = pos + directions[dir] * fac;

            if (board.outbox(p))
            {
                blk_py |= (1 << i);
                wht_py |= (1 << i);
            }
            else if (board.getchess(p) == BLACK)
            {
                blk_px |= (1 << i);
                wht_py |= (1 << i);
            }
            else if (board.getchess(p) == WHITE)
            {
                blk_py |= (1 << i);
                wht_px |= (1 << i);
            }
            i--;
        }
        ret.set_pattern(BLACK, dir, blk_px, blk_py);
        ret.set_pattern(WHITE, dir, wht_px, wht_py);
    }
    ret.update_cats();

    for (int dir = 0; dir < 4; dir++)
    {
        int neigh_cnt = 0;
        for (int fac = -2; fac <= 2; fac++)
        {
            if (fac == 0) continue;

            point_t p = pos + directions[dir] * fac;

            if (board.outbox(p))
                continue;

            if (not board.getchess(p) == EMPTY)
                neigh_cnt++;
        }
        ret.neighbors[dir] = neigh_cnt;
    }
    return ret;
}

chess_state get_state(gomoku_board const& board, int x, int y) { return get_state(board, point_t{x, y}); }

int evaluate(gomoku_board const& board, gomoku_chess chess)
{
    int val = 0;
    for (int i = 0; i < 15; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            if (board.getchess(i, j) == chess)
                val += get_state(board, i, j).rankof(chess);
        }
    }
    return val;
}

gomoku_chess get_winner(gomoku_board const& board, point_t pos)
{
    auto chess = board.getchess(pos);

    if (chess == EMPTY)
        return EMPTY;

    for (int dir = 0; dir < 4; dir++)
    {
        auto p = get_state(board, pos).get_pattern(chess, dir);
        if (cal_category(p[0], p[1]) == FIVE)
            return chess;
    }

    return EMPTY;
}

} // namespace nara

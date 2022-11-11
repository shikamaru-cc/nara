#pragma once

#include <array>
#include <cstdint>

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

constexpr auto gen_category_table()
{
    std::array<std::array<int, 256>, 256> table{};

    for (int px = 0; px < 256; px++)
        for (int py = 0; py < 256; py++)
            table[px][py] = cal_category(px, py);

    return table;
}

constexpr std::array<std::array<int, 256>, 256> category_table = gen_category_table();

constexpr int get_category(uint8_t px, uint8_t py)
{
    return category_table[px][py];
}

struct pattern_pair
{
    uint8_t px;
    uint8_t py;
};

struct chess_state
{
    point_t pos;

    int neighbors[4];

    pattern_pair pattern_blk[4];
    pattern_pair pattern_wht[4];

    std::array<int, 10> cats_blk;
    std::array<int, 10> cats_wht;

    pattern_pair get_pattern(gomoku_chess chess, point_t dir)
    {
        assert(chess == WHITE or chess == BLACK);
        int idx = idx_of_dir(dir);
        return (chess == BLACK) ? pattern_blk[idx] : pattern_wht[idx];
    }

    void set_pattern(gomoku_chess chess, point_t dir, uint8_t px, uint8_t py)
    {
        pattern_pair *ptr = (chess == BLACK) ? pattern_blk : pattern_wht;
        ptr += idx_of_dir(dir);
        ptr->px = px;
        ptr->py = py;
    }

    void set_neighbor(point_t dir, int cnt)
    {
        neighbors[idx_of_dir(dir)] = cnt;
    }

    int rankof(gomoku_chess chess)
    {
        int val = 0;
        for (size_t i = 0; i < 4; i++)
        {
            if (chess == BLACK)
                val += cal_rank(pattern_blk[i].px, pattern_blk[i].py);
            else
                val += cal_rank(pattern_wht[i].px, pattern_wht[i].py);
        }
        return val;
    }

    bool has_neighbor()
    {
        return neighbors[0] > 0 or neighbors[1] > 0 or neighbors[2] > 0 or neighbors[3] > 0;
    }

    bool has_category(gomoku_chess for_chess, int cat)
    {
        for (auto dir : directions)
        {
            auto p = get_pattern(for_chess, dir);
            if (cal_category(p.px, p.py) == cat)
                return true;
        }
        return false;
    }

    int cnt_category(gomoku_chess for_chess, int cat)
    {
        int ret = 0;
        for (auto dir : directions)
        {
            auto p = get_pattern(for_chess, dir);
            if (cal_category(p.px, p.py) == cat)
                ret++;
        }
        return ret;
    }

    void update_cats()
    {
        for (int i = 0; i < 10; i++)
        {
            cats_blk[i] = 0;
            cats_wht[i] = 0;
        }
        for (auto dir : directions)
        {
            auto p = get_pattern(BLACK, dir);
            cats_blk[get_category(p.px, p.py)]++;
            p = get_pattern(WHITE, dir);
            cats_wht[get_category(p.px, p.py)]++;
        }
    }

    auto cats_for(gomoku_chess chess)
    {
        return (chess == BLACK) ? cats_blk : cats_wht;
    }
};

chess_state get_state(gomoku_board const& board, point_t pos)
{
    chess_state ret;
    ret.pos = pos;
    for (auto dir : directions)
    {
        int neigh_cnt = 0;
        uint8_t blk_px, blk_py, wht_px, wht_py;
        blk_px = blk_py = wht_px = wht_py = 0;
        int i = 7;
        for (auto p : mkline(pos, dir))
        {
            if (p == pos) continue;

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

    for (auto dir : directions)
    {
        int neigh_cnt = 0;
        for (auto p : mkline(pos, dir, 2))
        {
            if (p == pos) continue;

            if (board.outbox(p))
                continue;

            auto chess = board.getchess(p);
            if (chess == BLACK or chess == WHITE)
                neigh_cnt++;
        }
        ret.set_neighbor(dir, neigh_cnt);
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

    for (auto dir : directions)
    {
        auto p = get_state(board, pos).get_pattern(chess, dir);
        if (cal_category(p.px, p.py) == FIVE)
            return chess;
    }

    return EMPTY;
}

} // namespace nara

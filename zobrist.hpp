#pragma once

#include <array>
#include <random>
#include <limits>
#include <cstddef>

#include "board.hpp"

namespace nara
{

using zobrist_t = std::array<std::array<size_t, 15>, 15>;

struct zobrist_hash
{
    size_t operator() (const zobrist_t zob) const
    {
        size_t h = 0;
        for (size_t i = 0; i < 15; i++)
            for (size_t j = 0; j < 15; j++)
                if (zob[i][j]) h ^= zob[i][j];
        return h;
    }
};

struct zobrist_eq
{
    bool operator() (const zobrist_t z1, const zobrist_t z2) const
    {
        for (size_t i = 0; i < 15; i++)
            for (size_t j = 0; j < 15; j++)
                if (z1[i][j] != z2[i][j]) return false;
        return true;
    }
};


zobrist_t zob_blk;
zobrist_t zob_wht;

void init_zobrist()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned long> dist(1, std::numeric_limits<unsigned long>::max());

    for (size_t i = 0; i < 15; i++)
    {
        for (size_t j = 0; j < 15; j++)
        {
            zob_blk[i][j] = dist(gen);
            zob_wht[i][j] = dist(gen);
        }
    }
}

size_t zobrist_val(int x, int y, gomoku_chess chess)
{
    if (chess == EMPTY) return 0;
    return (chess == BLACK) ? zob_blk[x][y] : zob_wht[x][y];
}

size_t zobrist_val(point_t p, gomoku_chess chess) { return zobrist_val(p.x, p.y, chess); }

} // namespace nara
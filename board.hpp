#pragma once

#include <cassert>
#include <iostream>

namespace nara
{

struct point_t
{
    int x;
    int y;
    point_t(int _x = 0, int _y = 0) : x(_x), y(_y)
    {
    }
};

std::ostream &operator<<(std::ostream &os, point_t p)
{
    return os << "[" << p.x << ", " << p.y << "]";
}

point_t operator *(point_t p, int fac)
{
    return point_t(p.x * fac, p.y * fac);
}

point_t operator +(point_t p1, point_t p2)
{
    return point_t(p1.x + p2.x, p1.y + p2.y);
}

bool operator ==(point_t p1, point_t p2)
{
    return p1.x == p2.x and p1.y == p2.y;
}

const point_t directions[4] = {point_t{1, 0}, point_t{1, 1}, point_t{0, 1}, point_t{-1, 1}};

enum gomoku_chess
{
    EMPTY,
    BLACK,
    WHITE
};

gomoku_chess oppof(gomoku_chess chess)
{
    assert(not chess == EMPTY);
    return chess == BLACK ? WHITE : BLACK;
}

class gomoku_board
{
  public:
    static const int WIDTH = 15;

    gomoku_chess chesses[WIDTH][WIDTH];

    gomoku_board()
    {
        for (int i = 0; i < WIDTH; i++)
            for (int j = 0; j < WIDTH; j++)
                chesses[i][j] = EMPTY;
    }

    gomoku_board(gomoku_board const &board)
    {
        for (int i = 0; i < WIDTH; i++)
            for (int j = 0; j < WIDTH; j++)
                chesses[i][j] = board.chesses[i][j];
    }

    static constexpr inline bool outbox(int x, int y)
    {
        return x < 0 or x >= WIDTH or y < 0 or y >= WIDTH;
    }

    static inline bool outbox(point_t pos)
    {
        return outbox(pos.x, pos.y);
    }

    gomoku_chess getchess(int x, int y) const
    {
        assert(not outbox(x, y));
        return chesses[x][y];
    }

    gomoku_chess getchess(point_t pos) const
    {
        return getchess(pos.x, pos.y);
    }

    void setchess(int x, int y, gomoku_chess chess)
    {
        assert(not outbox(x, y));
        chesses[x][y] = chess;
    }

    void setchess(point_t pos, gomoku_chess chess)
    {
        setchess(pos.x, pos.y, chess);
    }
};

} // namespace nara

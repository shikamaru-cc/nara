#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>

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

point_t operator*(point_t p, int fac)
{
    return point_t{p.x * fac, p.y * fac};
}

point_t operator+(point_t p1, point_t p2)
{
    return point_t{p1.x + p2.x, p1.y + p2.y};
}

const point_t directions[4] = {point_t{1, 0}, point_t{1, 1}, point_t{0, 1}, point_t{-1, 1}};

bool operator==(point_t p1, point_t p2)
{
    return p1.x == p2.x && p1.y == p2.y;
}

int idx_of_dir(point_t dir)
{
    if (dir == point_t{1, 0})
        return 0;
    if (dir == point_t{1, 1})
        return 1;
    if (dir == point_t{0, 1})
        return 2;
    if (dir == point_t{-1, 1})
        return 3;
    assert(false);
}

using points_t = std::vector<point_t>;

points_t mkline(point_t center, point_t dir, int fac_max)
{
    points_t points;
    for (int fac = -fac_max; fac <= fac_max; fac++)
    {
        points.push_back({center.x + fac * dir.x, center.y + fac * dir.y});
    }
    return points;
}

points_t mkline(point_t center, point_t dir)
{
    return mkline(center, dir, 4);
}

enum gomoku_chess
{
    EMPTY,
    BLACK,
    WHITE
};

gomoku_chess oppof(gomoku_chess chess)
{
    assert(chess == BLACK || chess == WHITE);
    return chess == BLACK ? WHITE : BLACK;
}

class gomoku_board
{
  private:
    static const int _WIDTH = 15;

  protected:
  public:
    static const int WIDTH = 15;

    int winner;

    gomoku_chess chesses[_WIDTH][_WIDTH];

    gomoku_board()
    {
        winner = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            for (int j = 0; j < WIDTH; j++)
            {
                chesses[i][j] = EMPTY;
            }
        }
    }

    gomoku_board(gomoku_board const &board)
    {
        winner = board.winner;
        for (int i = 0; i < WIDTH; i++)
        {
            for (int j = 0; j < WIDTH; j++)
            {
                chesses[i][j] = board.chesses[i][j];
            }
        }
    }

    static constexpr inline bool outbox(int x, int y)
    {
        return x < 0 || x >= WIDTH || y < 0 || y >= WIDTH;
    }

    static inline bool outbox(point_t pos)
    {
        return outbox(pos.x, pos.y);
    }

    gomoku_chess getchess(int x, int y) const
    {
        if (outbox(x, y))
        {
            // logger << boost::stacktrace::stacktrace() << std::endl;
            // logger.flush();
            assert(false);
        }
        // assert(!outbox(x, y));
        return chesses[x][y];
    }

    gomoku_chess getchess(point_t pos) const
    {
        return getchess(pos.x, pos.y);
    }

    void setchess(int x, int y, gomoku_chess chess)
    {
        assert(!outbox(x, y));
        if (winner != 0)
            return;
        chesses[x][y] = chess;
    }

    void setchess(point_t pos, gomoku_chess chess)
    {
        setchess(pos.x, pos.y, chess);
    }
};

} // namespace nara

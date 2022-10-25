#pragma once

namespace nara {

struct point_t {
    int x;
    int y;
    point_t(int _x = 0, int _y = 0): x(_x), y(_y) {}
};

const point_t directions[4] = {
    point_t{1, 0}, point_t{1, 1}, point_t{0, 1}, point_t{-1, 1}
};

bool operator == (point_t p1, point_t p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

using points_t = std::vector<point_t>;

points_t mkline(point_t center, point_t dir) {
    points_t points;
    for(int fac = -4; fac <= 4; fac++) {
        points.push_back({
            center.x + fac * dir.x, center.y + fac * dir.y
        });
    }
    return points;
}

enum gomoku_chess { EMPTY, OUTBX, BLACK, WHITE };

gomoku_chess oppof(gomoku_chess chess) {
    // assert(chess == BLACK || chess == WHITE);
    return chess == BLACK ? WHITE : BLACK;
}

class gomoku_board {
public:
    static const int WIDTH = 15;

    gomoku_chess chesses[WIDTH][WIDTH];

    int winner;

    gomoku_board() {
        winner = 0;
        for(int i = 0; i < WIDTH; i++) {
            for(int j = 0; j < WIDTH; j++) {
                chesses[i][j] = EMPTY;
            }
        }
    }

    static constexpr inline bool outbox(int x, int y) {
        return x < 0 || x >= WIDTH || y < 0 || y >= WIDTH;
    }

    static inline bool outbox(point_t pos) {
        return outbox(pos.x, pos.y);
    }

    gomoku_chess getchess(int x, int y) const {
        return outbox(x, y) ? OUTBX : chesses[x][y];
    }

    gomoku_chess getchess(point_t pos) const {
        return getchess(pos.x, pos.y);
    }

    void setchess(int x, int y, gomoku_chess chess) {
        assert(x >= 0 && x < WIDTH);
        assert(y >= 0 && y < WIDTH);
        chesses[x][y] = chess;
    }

    void setchess(point_t pos, gomoku_chess chess) {
        setchess(pos.x, pos.y, chess);
    }

    void setchess_and_test(int x, int y, gomoku_chess chess) {
        if(winner != 0) return;
        setchess(x, y, chess);
        winner = testpoint(x, y);
    }

    bool iswin(points_t const & points, int start, int stop) {
        assert(stop - start == 5);

        auto begin = points.begin() + start;
        auto end = points.begin() + stop;

        gomoku_chess one = getchess(*begin);

        if(one == EMPTY)
            return false;

        for(auto point = begin + 1; point != end; point++) {
            if(getchess(*point) != one)
                return false;
        }

        return true;
    }

    points_t testwin(points_t const & points) {
        assert(points.size() == 9);
        for(int i = 0; i < 5; i++) {
            if(iswin(points, i, i+5))
                return points_t(points.begin()+i, points.begin()+i+5);
        }
        return points_t{};
    }

    // 1 for BLACK WIN, -1 for WHITE WIN, 0 for NO WINNER
    int testpoint(int x, int y) {
        if(getchess(x, y) == EMPTY)
            return 0;

        point_t center{x, y};
        for(auto & dir : directions) {
            auto points = mkline(center, dir);
            auto winset = testwin(points);
            if(winset.empty())
                continue;
            return getchess(winset[0]) == BLACK ? 1 : -1;
        }

        return 0;
    }

    void debugpoint(points_t points) {
        for(auto const & point : points) {
            std::cout << '[' << point.x << ", " << point.y << ']';
        }
        std::cout << '\n';
    }

    int evalwinner(void) {
        for(int i = 0; i < WIDTH; i++) {
            for(int j = 0; j < WIDTH; j++) {
                int res = testpoint(i, j);
                if(res != 0)
                    return res;
            }
        }
        return 0;
    }
};

} // namespace nara

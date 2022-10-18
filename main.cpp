#include <cassert>
#include <ncurses.h>

#include <vector>
#include <iostream>
#include <fstream>

#include "bench.hpp"

std::ofstream logger;

struct point_t {
    int x;
    int y;
    point_t(int _x, int _y): x(_x), y(_y) {}
};

const point_t directions[4] = {
    point_t{1, 0}, point_t{1, 1}, point_t{0, 1}, point_t{1, -1}
};

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

enum gomoku_chess { EMPTY, BLACK, WHITE };

class gomoku_board {
private:
    inline bool outbox(int x, int y) const {
        return x < 0 || x >= WIDTH || y < 0 || y >= WIDTH;
    }

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

    gomoku_chess getchess(int x, int y) const {
        return outbox(x, y) ? EMPTY : chesses[x][y];
    }

    gomoku_chess getchess(point_t point) const {
        return getchess(point.x, point.y);
    }

    void setchess(int x, int y, gomoku_chess chess) {
        assert(x >= 0 && x < WIDTH);
        assert(y >= 0 && y < WIDTH);

        if(winner != 0) return;

        chesses[x][y] = chess;

        auto elapsed = benchmark([&](){ winner = testpoint(x, y); });
        logger << "eval elapsed " << elapsed << '\n';
        logger.flush();
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

class gomoku_ai {
public:
    point_t getnext(gomoku_board const & board, gomoku_chess chess) {
        for(int i = 0; i < gomoku_board::WIDTH; i++) {
            for(int j = 0; j < gomoku_board::WIDTH; j++) {
                if(board.getchess(i, j) == EMPTY)
                    return point_t{i, j};
            }
        }
        return point_t{-1, -1};
    }
};

struct cursor_t {
    int x;
    int y;

    cursor_t(int _x = 0, int _y = 0): x(_x), y(_y) {}
    void goup(void)    { if(x > 0) x--; }
    void godown(void)  { if(x < gomoku_board::WIDTH) x++; }
    void goleft(void)  { if(y > 0) y--; }
    void goright(void) { if(y < gomoku_board::WIDTH - 1) y++; }
};

void display(gomoku_board const & board, cursor_t const & cursor) {
    clear();
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        for(int j = 0; j < gomoku_board::WIDTH; j++) {
            switch (board.chesses[i][j]) {
            case EMPTY: printw("+"); break;
            case BLACK: printw("X"); break;
            case WHITE: printw("O"); break;
            }
            printw(" ");
        }
        printw("\n");
    }
    move(cursor.x, cursor.y * 2);
    refresh();
}

enum gomoku_action { UP, DOWN, LEFT, RIGHT, CHOOSE, QUIT, NONE };

gomoku_action getaction(void) {
    switch (getch()) {
    case 'q':
    case 'Q':
        return QUIT;

    case ' ':
        return CHOOSE;

    // handle arrow keys, code from https://stackoverflow.com/a/11432632
    case '\033': {
        getch(); // skip the [
        switch(getch()) { // the real value
        case 'A': return UP;
        case 'B': return DOWN;
        case 'C': return RIGHT;
        case 'D': return LEFT;
        default:  return NONE;
        }
    }

    default:
        return NONE;
    }
}

gomoku_chess applyaction(
    cursor_t & cursor,
    gomoku_board & board,
    gomoku_action action,
    gomoku_chess chess
) {
    switch (action) {
    case UP:
        cursor.goup();
        break;
    case DOWN:
        cursor.godown();
        break;
    case LEFT:
        cursor.goleft();
        break;
    case RIGHT:
        cursor.goright();
        break;
    case CHOOSE:
        if(board.getchess(cursor.x, cursor.y) == EMPTY) {
            board.setchess(cursor.x, cursor.y, chess);
            return (chess == BLACK) ? WHITE : BLACK;
        }
    default:
        {}
    }
    return chess;
}

int main() {
    logger.open("LOG");

    gomoku_board board = gomoku_board();
    cursor_t cursor;

    initscr();
    display(board, cursor);

    /* player mode
    gomoku_action action = getaction();
    gomoku_chess chess = BLACK;
    while(action != QUIT) {
        chess = applyaction(board, cursor, action, chess);
        display(board, cursor);

        if(board.winner != 0) {
            move(22, 0);
            if(board.winner == 1)
                printw("Black win!");
            else
                printw("White win!");
            refresh();
            getaction();
            break;
        }

        action = getaction();
    }
    */

    // ai mode
    gomoku_ai ai = gomoku_ai();

    gomoku_chess my_chess = BLACK;
    gomoku_chess ai_chess = WHITE;

    gomoku_action action = getaction();
    while(action != QUIT) {
        applyaction(cursor, board, action, my_chess);

        if(action == CHOOSE) {
            point_t ai_next = ai.getnext(board, ai_chess);
            board.setchess(ai_next.x, ai_next.y, ai_chess);
        }

        display(board, cursor);
        if(board.winner != 0) {
            move(20, 0);
            if(board.winner == 1)
                printw("Black win!");
            else
                printw("White win!");
            refresh();
            getaction();
            break;
        }

        action = getaction();
    }

    logger.close();
    endwin();
    return 0;
}

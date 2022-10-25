#include <cassert>
#include <ncurses.h>

#include <string>
#include <fstream>
#include <iostream>

#include "ai.hpp"
#include "bench.hpp"
#include "board.hpp"
#include "evaluate.hpp"

std::ofstream logger;

struct cursor_t {
    int x;
    int y;

    cursor_t(int _x = 0, int _y = 0): x(_x), y(_y) {}
    void set(int _x, int _y) { x = _x; y = _y; }
    void goup(void)    { if(x > 0) x--; }
    void godown(void)  { if(x < nara::gomoku_board::WIDTH) x++; }
    void goleft(void)  { if(y > 0) y--; }
    void goright(void) { if(y < nara::gomoku_board::WIDTH - 1) y++; }
};

void display(nara::gomoku_board const & board, cursor_t const & cursor) {
    clear();
    for(int i = 0; i < nara::gomoku_board::WIDTH; i++) {
        for(int j = 0; j < nara::gomoku_board::WIDTH; j++) {
            switch (board.chesses[i][j]) {
            case nara::EMPTY: printw("-"); break;
            case nara::BLACK: printw("X"); break;
            case nara::WHITE: printw("O"); break;
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

void applyaction(
    cursor_t & cursor,
    nara::gomoku_board & board,
    gomoku_action action,
    nara::gomoku_chess chess
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
        if(board.getchess(cursor.x, cursor.y) == nara::EMPTY) {
            board.setchess_and_test(cursor.x, cursor.y, chess);
        }
    default:
        {}
    }
}

int main() {
    logger.open("LOG");

    nara::gen_lines();

    nara::gomoku_board board = nara::gomoku_board();
    cursor_t cursor;

    // ai mode

    std::cout << "Please choose your chess, b for X, others for O: ";
    std::string choose;
    std::cin >> choose;
    std::cout << choose;

    nara::gomoku_chess my_chess;
    nara::gomoku_chess ai_chess;

    my_chess = (choose == "b") ? nara::BLACK : nara::WHITE;
    ai_chess = nara::oppof(my_chess);

    nara::gomoku_ai ai = nara::gomoku_ai(ai_chess);

    initscr();
    display(board, cursor);

    nara::gomoku_chess turn = nara::BLACK;
    while(board.winner == 0) {
        if(turn == ai_chess) {
            nara::point_t ai_next;

            ai_next = ai.getnext(board);

            if(board.getchess(ai_next) != nara::EMPTY)
                assert(false);

            board.setchess_and_test(ai_next.x, ai_next.y, ai_chess);
            cursor.set(ai_next.x, ai_next.y);
        } else { // player turn
            for(;;) {
                gomoku_action action = getaction();
                if(action == QUIT)
                    goto quit;
                applyaction(cursor, board, action, my_chess);
                display(board, cursor);
                if(action == CHOOSE)
                    break;
            }
        }
        display(board, cursor);

        /*
        move(17, 0);
        auto [results_blk, results_wht] = __evaluate_board(board);

        nara::eval_result res_blk;
        for(auto & res : results_blk) {
            res_blk.score += res.score;
            res_blk.pattern_cnt[nara::SICK2] += res.pattern_cnt[nara::SICK2];
            res_blk.pattern_cnt[nara::FLEX2] += res.pattern_cnt[nara::FLEX2];
            res_blk.pattern_cnt[nara::SICK3] += res.pattern_cnt[nara::SICK3];
            res_blk.pattern_cnt[nara::FLEX3] += res.pattern_cnt[nara::FLEX3];
            res_blk.pattern_cnt[nara::SICK4] += res.pattern_cnt[nara::SICK4];
            res_blk.pattern_cnt[nara::FLEX4] += res.pattern_cnt[nara::FLEX4];
            res_blk.pattern_cnt[nara::FLEX5] += res.pattern_cnt[nara::FLEX5];
        }

        printw("result score: %d\n", res_blk.score);
        printw("SICK2: %d\n", res_blk.pattern_cnt[nara::SICK2]);
        printw("FLEX2: %d\n", res_blk.pattern_cnt[nara::FLEX2]);
        printw("SICK3: %d\n", res_blk.pattern_cnt[nara::SICK3]);
        printw("FLEX3: %d\n", res_blk.pattern_cnt[nara::FLEX3]);
        printw("SICK4: %d\n", res_blk.pattern_cnt[nara::SICK4]);
        printw("FLEX4: %d\n", res_blk.pattern_cnt[nara::FLEX4]);
        printw("FLEX5: %d\n", res_blk.pattern_cnt[nara::FLEX5]);

        nara::eval_result res_wht;
        for(auto & res : results_wht) {
            res_wht.score += res.score;
            res_wht.pattern_cnt[nara::SICK2] += res.pattern_cnt[nara::SICK2];
            res_wht.pattern_cnt[nara::FLEX2] += res.pattern_cnt[nara::FLEX2];
            res_wht.pattern_cnt[nara::SICK3] += res.pattern_cnt[nara::SICK3];
            res_wht.pattern_cnt[nara::FLEX3] += res.pattern_cnt[nara::FLEX3];
            res_wht.pattern_cnt[nara::SICK4] += res.pattern_cnt[nara::SICK4];
            res_wht.pattern_cnt[nara::FLEX4] += res.pattern_cnt[nara::FLEX4];
            res_wht.pattern_cnt[nara::FLEX5] += res.pattern_cnt[nara::FLEX5];
        }

        printw("result score: %d\n", res_wht.score);
        printw("SICK2: %d\n", res_wht.pattern_cnt[nara::SICK2]);
        printw("FLEX2: %d\n", res_wht.pattern_cnt[nara::FLEX2]);
        printw("SICK3: %d\n", res_wht.pattern_cnt[nara::SICK3]);
        printw("FLEX3: %d\n", res_wht.pattern_cnt[nara::FLEX3]);
        printw("SICK4: %d\n", res_wht.pattern_cnt[nara::SICK4]);
        printw("FLEX4: %d\n", res_wht.pattern_cnt[nara::FLEX4]);
        printw("FLEX5: %d\n", res_wht.pattern_cnt[nara::FLEX5]);

        refresh();
        */

        turn = nara::oppof(turn);
    }

    display(board, cursor);
    move(20, 0);
    if(board.winner == 1)
        printw("Black win!");
    else
        printw("White win!");
    refresh();
    getaction();

quit:
    logger.close();
    endwin();
    return 0;
}

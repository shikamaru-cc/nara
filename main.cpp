#include <cassert>
#include <ncurses.h>

#include <tuple>
#include <ranges>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

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
    void godown(void)  { if(x < gomoku_board::WIDTH) x++; }
    void goleft(void)  { if(y > 0) y--; }
    void goright(void) { if(y < gomoku_board::WIDTH - 1) y++; }
};

void display(gomoku_board const & board, cursor_t const & cursor) {
    clear();
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        for(int j = 0; j < gomoku_board::WIDTH; j++) {
            switch (board.chesses[i][j]) {
            case EMPTY: printw("-"); break;
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

struct line_t {
    point_t ori;
    point_t dir;
};

// for debug
static int last_eval_times;

auto evaluate_line(gomoku_board const & board, line_t line) {
    last_eval_times++;

    // auto start = std::chrono::system_clock::now();

    std::vector<nara::gomoku_chess> chesses;

    point_t ori= line.ori;
    point_t dir = line.dir;
    point_t p = ori;
    while(!gomoku_board::outbox(p.x, p.y)) {
        char c;
        switch(board.getchess(p.x, p.y)) {
        case BLACK: chesses.push_back(nara::BLACK); break;
        case WHITE: chesses.push_back(nara::WHITE); break;
        case EMPTY: chesses.push_back(nara::EMPTY); break;
        }
        p = {p.x + dir.x, p.y + dir.y};
    }

    /*
    auto end = std::chrono::system_clock::now();
    logger << "evaluate_line use"
           << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
           << "\n";
    logger.flush();
    */

    return nara::evaluate(chesses);
}

std::vector<line_t> all_lines;

void gen_lines() {
    point_t origin, dir;

    // line '-'
    dir = {0, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {i, 0};
        all_lines.push_back(line_t{origin, dir});
    }

    // line '|'
    dir = {1, 0};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {0, i};
        all_lines.push_back(line_t{origin, dir});
    }

    // line '/'
    dir = {-1, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {i, 0};
        all_lines.push_back(line_t{origin, dir});
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++) {
        origin = {14, i};
        all_lines.push_back(line_t{origin, dir});
    }

    // line '\'
    dir = {1, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {i, 0};
        all_lines.push_back(line_t{origin, dir});
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++) {
        origin = {14, i};
        all_lines.push_back(line_t{origin, dir});
    }
}

auto __evaluate_board(gomoku_board const & board) {
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

int evaluate_board(gomoku_board const & board, gomoku_chess iam) {
    auto [results_blk, results_wht] = __evaluate_board(board);

    int score_blk = 0;
    for(auto const & res : results_blk)
        score_blk += res.score;

    int score_wht = 0;
    for(auto const & res : results_wht)
        score_wht += res.score;

    return (iam == BLACK) ? score_blk - score_wht : score_wht - score_blk;
}

struct score_board {
private:
    // ori.x ori.y dir
    nara::eval_result _score_blk[15][15][4];
    nara::eval_result _score_wht[15][15][4];

    int _dirty[15][15][4];

    int _total_blk;
    int _total_wht;

    int idx_of_dir(point_t dir) {
        if(dir == point_t{0, 1}) return 0;
        if(dir == point_t{1, 0}) return 1;
        if(dir == point_t{1, 1}) return 2;
        if(dir == point_t{-1, 1}) return 3;
        assert(false);
    }

    point_t origin_at_line(point_t pos, point_t dir) {
        for(;;) {
           point_t back_pos = {pos.x - dir.x, pos.y - dir.y};
           if(gomoku_board::outbox(back_pos))
               return pos;
           pos = back_pos;
        }
    }

    std::vector<line_t> lines_contain_pos(point_t pos) {
        std::vector<line_t> lines;
        for(auto dir : directions) {
            lines.emplace_back(line_t{origin_at_line(pos, dir), dir});
        }
        return lines;
    }

public:
    gomoku_board _board;

    point_t last_pos;

    score_board() = delete;

    score_board(gomoku_board const & board): _board(board) {
        _total_blk = _total_wht = 0;
        for(auto line : all_lines) {
            auto [res_blk, res_wht] = evaluate_line(_board, line);
            _score_blk[line.ori.x][line.ori.y][idx_of_dir(line.dir)] = res_blk;
            _score_wht[line.ori.x][line.ori.y][idx_of_dir(line.dir)] = res_wht;
            _total_blk += res_blk.score;
            _total_wht += res_wht.score;
        }
    }

    score_board(
        score_board const & prev_sboard, gomoku_chess next_chess, point_t next_pos
    ) {
        _board = prev_sboard._board;
        last_pos = next_pos;

        std::ranges::copy(
            &prev_sboard._score_blk[0][0][0],
            &prev_sboard._score_blk[0][0][0] + 15 * 15 * 4,
            &_score_blk[0][0][0]
        );
        std::ranges::copy(
            &prev_sboard._score_wht[0][0][0],
            &prev_sboard._score_wht[0][0][0] + 15 * 15 * 4,
            &_score_wht[0][0][0]
        );
        _total_blk = prev_sboard._total_blk;
        _total_wht = prev_sboard._total_wht;

        _board.setchess(next_pos, next_chess);

        for(auto & line : lines_contain_pos(next_pos)) {
            _total_blk -=
                _score_blk[line.ori.x][line.ori.y][idx_of_dir(line.dir)].score;

            _total_wht -=
                _score_wht[line.ori.x][line.ori.y][idx_of_dir(line.dir)].score;

            auto [res_blk, res_wht] = evaluate_line(_board, line);
            _score_blk[line.ori.x][line.ori.y][idx_of_dir(line.dir)] = res_blk;
            _score_wht[line.ori.x][line.ori.y][idx_of_dir(line.dir)] = res_wht;

            _total_blk += res_blk.score;
            _total_wht += res_wht.score;
        }
    }

    gomoku_chess getchess(point_t pos) const {
        return _board.getchess(pos);
    }

    int score_blk() const { return _total_blk - _total_wht; }
    int score_wht() const { return _total_wht - _total_blk; }
};

auto is_empty_at(score_board const & sboard) {
    return [&](point_t pos) { return sboard.getchess(pos) == EMPTY; };
}

class gomoku_ai {
private:
    // my chess
    gomoku_chess mine;

    // candidates are all chess positions but sort by priority
    points_t candidates;

    // prior of a candidate depends on its distance from board center.
    int prior_of(point_t p) {
        static const point_t board_center =
            { gomoku_board::WIDTH / 2, gomoku_board::WIDTH / 2 };

        int dx = p.x - board_center.x;
        int dy = p.y - board_center.y;
        return -(dx * dx + dy * dy);
    }

    // TODO: init candidates at compile time
    void init_candidates(void) {
        for(int i = 0; i < gomoku_board::WIDTH; i++)
            for(int j = 0; j < gomoku_board::WIDTH; j++)
                candidates.push_back({i, j});

        auto compare_priority = [&](point_t p1, point_t p2){
            return prior_of(p1) > prior_of(p2);
        };

        std::ranges::sort(candidates, compare_priority);
    }

    struct choose_t {
        point_t pos;
        int score_blk;
        int score_wht;
        int score;
    };

    std::vector<point_t>
    line_for_pos(point_t pos, point_t dir) {
        std::vector<point_t> line;
        for(int fac = -4; fac <= 4; fac++) {
            line.emplace_back(point_t{
                pos.x + fac * dir.x, pos.y + fac * dir.y
            });
        }
        return line;
    }

    std::vector<nara::gomoku_chess>
    gen_chesses(gomoku_board const & board, std::vector<point_t> line) {
        std::vector<nara::gomoku_chess> chesses;
        for(auto & p : line) {
            switch(board.getchess(p)) {
            case BLACK: chesses.push_back(nara::BLACK); break;
            case WHITE: chesses.push_back(nara::WHITE); break;
            case EMPTY: chesses.push_back(nara::EMPTY); break;
            // case OUTBX: chesses.push_back(nara::OUTBX); break;
            }
        }
        return chesses;
    }

    choose_t gen_choose(gomoku_board const & board, point_t pos) {
        assert(board.getchess(pos) == EMPTY);
        choose_t choose{pos, 0, 0, 0};
        for(auto dir : directions) {
            auto line = line_for_pos(pos, dir);
            auto chesses = gen_chesses(board, line);
            auto [rblk, rwht] = evaluate(chesses);
            choose.score_blk += rblk.score;
            choose.score_wht += rwht.score;
        }
        choose.score = (mine == BLACK)
            ? choose.score_blk - choose.score_wht
            : choose.score_wht - choose.score_blk;

        return choose;
    }

    static bool with_score(choose_t choose) {
        return choose.score_blk != 0 || choose.score_wht != 0;
    }

    auto choose_generator(gomoku_board const & board) {
        return [&](point_t pos){ return gen_choose(board, pos); };
    }

    std::tuple<point_t, int> alphabeta(
        score_board const & sboard, gomoku_chess next,
        int alpha, int beta, bool ismax, int depth
    ) {
        depth_cnt[depth]++;

        if(depth == 0) {
            if(mine == BLACK)
                return std::make_tuple(sboard.last_pos, sboard.score_blk());
            else
                return std::make_tuple(sboard.last_pos, sboard.score_wht());
        }

        std::vector<choose_t> chooses;
        std::ranges::copy(
            candidates |
            std::views::filter(is_empty_at(sboard)) |
            std::views::transform(choose_generator(sboard._board)) |
            std::views::filter(with_score),
            std::back_inserter(chooses)
        );

        if(chooses.empty()) {
            std::ranges::copy(
                candidates |
                std::views::filter(is_empty_at(sboard)) |
                std::views::transform(choose_generator(sboard._board)) |
                std::views::take(9),
                std::back_inserter(chooses)
            );
        }

        point_t bestpos;

        if(ismax) {
            int score = -10000000;
            for(auto & choose : chooses) {

                auto[_ , _score] =
                    alphabeta(score_board(sboard, next, choose.pos), oppof(next), alpha, beta, false, depth - 1);

                if(_score > score)
                    bestpos = choose.pos;

                score = std::max(score, _score);
                alpha = std::max(alpha, score);

                if(beta <= alpha)
                    break;
            }
            return std::make_tuple(bestpos, score);
        }

        // ismin
        int score = 10000000;
        for(auto & choose : chooses) {

            auto[_ , _score] =
                alphabeta(score_board(sboard, next, choose.pos), oppof(next), alpha, beta, true, depth - 1);

            if(_score < score)
                bestpos = choose.pos;

            score = std::min(score, _score);
            beta = std::min(beta, score);

            if(beta <= alpha)
                break;
        }
        return std::make_tuple(bestpos, score);
    }

public:
    // just for debug
    int depth_cnt[4];

    gomoku_ai(gomoku_chess chess): mine(chess) { init_candidates(); }

    point_t getnext(gomoku_board const & board) {
        for(int i = 0; i < 4; i++)
            depth_cnt[i] = 0;

        auto [pos, score] = alphabeta(score_board(board), mine, -10000000, 10000000, true, 4);

        logger << " depth0: "  << depth_cnt[0]
               << " depth1: " << depth_cnt[1]
               << " depth2: " << depth_cnt[2]
               << " depth3: " << depth_cnt[3] << "\n";
        logger.flush();

        return pos;
    }
};

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
            board.setchess_and_test(cursor.x, cursor.y, chess);
            return (chess == BLACK) ? WHITE : BLACK;
        }
    default:
        {}
    }
    return chess;
}

int main() {
    logger.open("LOG");

    gen_lines();

    gomoku_board board = gomoku_board();
    cursor_t cursor;

    // ai mode

    std::cout << "Please choose your chess, b for X, others for O: ";
    std::string choose;
    std::cin >> choose;
    std::cout << choose;

    gomoku_chess my_chess;
    gomoku_chess ai_chess;

    my_chess = (choose == "b") ? BLACK : WHITE;
    ai_chess = oppof(my_chess);

    gomoku_ai ai = gomoku_ai(ai_chess);

    initscr();
    display(board, cursor);

    gomoku_chess turn = BLACK;
    while(board.winner == 0) {
        if(turn == ai_chess) {
            point_t ai_next;

            ::last_eval_times = 0;
            auto elapsed = benchmark([&](){ ai_next = ai.getnext(board); });
            logger << "get next elapsed " << elapsed << ", eval times " << last_eval_times << '\n';
            logger.flush();

            if(board.getchess(ai_next) != EMPTY)
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

        turn = oppof(turn);
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

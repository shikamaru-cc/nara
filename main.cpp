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
    assert(chess == BLACK || chess == WHITE);
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

    auto start = std::chrono::system_clock::now();

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

    auto end = std::chrono::system_clock::now();
    logger << "evaluate_line use"
           << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
           << "\n";
    logger.flush();

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

        std::vector<point_t> poses;
        std::ranges::copy(
            candidates | std::views::filter(is_empty_at(sboard)),
            std::back_inserter(poses)
        );

        point_t bestpos;

        if(ismax) {
            int score = -10000000;
            for(auto & pos : poses) {

                auto[_ , _score] =
                    alphabeta(score_board(sboard, next, pos), oppof(next), alpha, beta, false, depth - 1);

                if(_score > score)
                    bestpos = pos;

                score = std::max(score, _score);
                alpha = std::max(alpha, score);

                if(beta <= alpha)
                    break;
            }
            return std::make_tuple(bestpos, score);
        }

        // ismin
        int score = 10000000;
        for(auto & pos : poses) {

            auto[_ , _score] =
                alphabeta(score_board(sboard, next, pos), oppof(next), alpha, beta, true, depth - 1);

            if(_score < score)
                bestpos = pos;

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

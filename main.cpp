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

    gomoku_chess getchess(int x, int y) const {
        return outbox(x, y) ? OUTBX : chesses[x][y];
    }

    gomoku_chess getchess(point_t point) const {
        return getchess(point.x, point.y);
    }

    void setchess(int x, int y, gomoku_chess chess) {
        assert(x >= 0 && x < WIDTH);
        assert(y >= 0 && y < WIDTH);
        chesses[x][y] = chess;
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

using pos_t = std::tuple<int, int>;
constexpr int xof(pos_t p) { return std::get<0>(p); }
constexpr int yof(pos_t p) { return std::get<1>(p); }

// line_t consists of pos_t origin and pos_t dir
using line_t = std::tuple<pos_t, pos_t>;

const pos_t dir_vec[4] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };

auto evaluate_line(gomoku_board const & board, line_t line) {

    std::vector<nara::gomoku_chess> chesses;

    pos_t origin = std::get<0>(line);
    pos_t dir = std::get<1>(line);
    pos_t p = origin;
    while(!gomoku_board::outbox(xof(p), yof(p))) {
        char c;
        switch(board.getchess(xof(p), yof(p))) {
        case BLACK: chesses.push_back(nara::BLACK); break;
        case WHITE: chesses.push_back(nara::WHITE); break;
        case EMPTY: chesses.push_back(nara::EMPTY); break;
        }
        p = {xof(p) + xof(dir), yof(p) + yof(dir)};
    }

    return nara::evaluate(chesses);
}

auto __evaluate_board(gomoku_board const & board) {
    std::vector<nara::eval_result> results_blk;
    std::vector<nara::eval_result> results_wht;

    pos_t origin, dir;

    // line '-'
    dir = {0, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {i, 0};
        auto [res_blk, res_wht] = evaluate_line(board, {origin, dir});
        results_blk.push_back(std::move(res_blk));
        results_wht.push_back(std::move(res_wht));
    }

    // line '|'
    dir = {1, 0};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {0, i};
        auto [res_blk, res_wht] = evaluate_line(board, {origin, dir});
        results_blk.push_back(std::move(res_blk));
        results_wht.push_back(std::move(res_wht));
    }

    // line '/'
    dir = {-1, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {i, 0};
        auto [res_blk, res_wht] = evaluate_line(board, {origin, dir});
        results_blk.push_back(std::move(res_blk));
        results_wht.push_back(std::move(res_wht));
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++) {
        origin = {14, i};
        auto [res_blk, res_wht] = evaluate_line(board, {origin, dir});
        results_blk.push_back(std::move(res_blk));
        results_wht.push_back(std::move(res_wht));
    }

    // line '\'
    dir = {1, 1};
    for(int i = 0; i < gomoku_board::WIDTH; i++) {
        origin = {i, 0};
        auto [res_blk, res_wht] = evaluate_line(board, {origin, dir});
        results_blk.push_back(std::move(res_blk));
        results_wht.push_back(std::move(res_wht));
    }
    // i == 1 here to avoid re-eval origin {0, 0}
    for(int i = 1; i < gomoku_board::WIDTH; i++) {
        origin = {14, i};
        auto [res_blk, res_wht] = evaluate_line(board, {origin, dir});
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

// gomoku_evaler is used for evaluating pattern
class gomoku_evaler {
public:
    // desc_notion is the descriptor for describing line pattern
    // X for self, O for opp, E for empty
    enum desc_notation { X, O, E };

    static char notation2ch(desc_notation n) {
        switch(n) {
        case X:  return 'X';
        case O:  return 'O';
        default: return 'E';
        }
    }

    // desc_t contains a vector of desc_notion to denote a pattern
    using desc_t = std::vector<desc_notation>;

    std::string desc_to_string(desc_t const & desc) {
        std::string ret;
        std::ranges::copy(
            desc | std::views::transform(notation2ch),
            std::back_inserter(ret)
        );
        return ret;
    }

    friend desc_t operator + (desc_t d1, desc_t d2) {
        for(auto n : d2)
            d1.push_back(n);
        return d1;
    }

    friend std::ostream & operator << (std::ostream & os, desc_t desc) {
        for(auto notation : desc) {
            switch(notation) {
            case X: os << 'X'; break;
            case O: os << 'O'; break;
            case E: os << 'E'; break;
            }
        }
        return os;
    }

    // Following lists the pattern name and pattern for evaluating progress
    // SICK2: OXXE   EXXO
    // FLEX2: EXXE   EXEXE
    // SICK3: EXXXO  OXXXE  OXXEXE EXXEXO OXEXXE EXEXXO
    // FLEX3: EXXXE  EXXEXE EXEXXE
    // SICK4: EXXXXO OXXXXE XXEXX
    // FLEX4: EXXXXE
    // FLEX5: XXXXX
    enum pattern_name { SICK2 = 0, FLEX2, SICK3, FLEX3, SICK4, FLEX4, FLEX5 };

    struct pattern_t {
        int score;
        std::vector<desc_t> descs;

        pattern_t() = delete;
        pattern_t(int _score): score(_score) {}
        void add_desc(desc_t desc) { descs.push_back(desc); }
    };

    pattern_t patterns[7] = {10, 20, 100, 1000, 2000, 10000, 100000};

    std::unordered_map<std::string, int> desc_scores;

    void init_patterns(void) {
        patterns[SICK2].add_desc(desc_t{O, X, X, E});
        patterns[SICK2].add_desc(desc_t{E, X, X, O});

        patterns[FLEX2].add_desc(desc_t{E, X, X, E});
        patterns[FLEX2].add_desc(desc_t{E, X, E, X, E});

        patterns[SICK3].add_desc(desc_t{E, X, X, X, O});
        patterns[SICK3].add_desc(desc_t{O, X, X, X, E});
        patterns[SICK3].add_desc(desc_t{O, X, X, E, X, E});
        patterns[SICK3].add_desc(desc_t{E, X, X, E, X, O});
        patterns[SICK3].add_desc(desc_t{O, X, E, X, X, E});
        patterns[SICK3].add_desc(desc_t{E, X, E, X, X, O});

        patterns[FLEX3].add_desc(desc_t{E, X, X, X, E});
        patterns[FLEX3].add_desc(desc_t{E, X, X, E, X, E});
        patterns[FLEX3].add_desc(desc_t{E, X, E, X, X, E});

        patterns[SICK4].add_desc(desc_t{E, X, X, X, X, O});
        patterns[SICK4].add_desc(desc_t{O, X, X, X, X, E});
        patterns[SICK4].add_desc(desc_t{X, X, E, X, X});

        patterns[FLEX4].add_desc(desc_t{E, X, X, X, X, E});

        patterns[FLEX5].add_desc(desc_t{X, X, X, X, X});
    }

    std::vector<desc_t> project(
        std::vector<desc_t> & descs1, std::vector<desc_t> & descs2
    ) {
        std::vector<desc_t> ret;
        for(auto d1 : descs1)
            for(auto d2 : descs2)
                ret.push_back(d1 + d2);
        return ret;
    }

    void init_pattern_map(void) {
        std::vector<desc_t> base({desc_t{X}, desc_t{O}, desc_t{E}});
        std::vector<desc_t> descs = base;

        for(int i = 0; i < 8; i++)
            descs = project(descs, base);

        for(auto & desc : descs)
            desc_scores.emplace(desc_to_string(desc), eval_line(desc));
    }

    gomoku_evaler() {
        init_patterns();
        auto elapsed = benchmark([&](){ this->init_pattern_map(); });
        logger << elapsed << '\n';
        logger.flush();
    }

    bool match_desc(desc_t to_match, desc_t desc) {
        // start point of iter
        int start = (to_match.size() > 4) ? 0 : 4 - (to_match.size() - 1);

        for(int i = start; i <= 4; i++) {
            if(std::ranges::equal(to_match,
                std::ranges::subrange(desc.begin() + i, desc.end())))
                return true;
        }

        return false;
    }

    bool match_pattern(pattern_t const & pattern, desc_t line_desc) {
        for(auto desc : pattern.descs)
            if(match_desc(desc, line_desc))
                return true;
        return false;
    }

    auto desc_tester(desc_t desc) {
        return [desc, this](pattern_t const & p){
            return this->match_pattern(p, desc);
        };
    }

    int eval_line(desc_t line_desc) {

        auto matched = std::views::reverse(patterns)
                     | std::views::filter(desc_tester(line_desc))
                     | std::views::take(1);

        return matched.front().score;
    }

    int get_line_score(desc_t line_desc) {
        return desc_scores.at(desc_to_string(line_desc));
    }
};

class gomoku_ai {
private:
    // my chess
    gomoku_chess mine;

    // candidates are all chess positions but sort by priority
    points_t candidates;

    gomoku_evaler evaler;

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

    gomoku_evaler::desc_t points_to_desc(
        gomoku_board const & board, points_t points
    ) {
        assert(points.size() == 9);

        auto mid_chess = board.getchess(points[4]);

        assert(mid_chess == BLACK || mid_chess == WHITE);

        gomoku_evaler::desc_t desc;
        for(auto & point : points) {
            auto chess = board.getchess(point);
            if(chess == EMPTY)
                desc.push_back(gomoku_evaler::E);
            else if(chess == mid_chess)
                desc.push_back(gomoku_evaler::X);
            else // opp or outbox
                desc.push_back(gomoku_evaler::O);
        }
        return desc;
    }

    int evaluate_pos(gomoku_board const & board, point_t pos) {
        _last_eval_times++;

        if(board.getchess(pos) == EMPTY) return 0;

        int score = 0;
        for(auto & dir : directions) {
            auto points = mkline(pos, dir);
            auto line_desc = points_to_desc(board, points);
            score += evaler.get_line_score(line_desc);
        }
        return score;
    }

    int _last_eval_times;

    int evaluate(gomoku_board const & board) {
        int me_total = 0;
        int op_total = 0;
        for(int i = 0; i < gomoku_board::WIDTH; i++) {
            for(int j = 0; j < gomoku_board::WIDTH; j++) {
                auto chess = board.getchess(i, j);
                if(chess == mine)
                    me_total += evaluate_pos(board, {i, j});
                else
                    op_total += evaluate_pos(board, {i, j});
            }
        }
        return me_total - op_total;
    }

    struct score_board {
        gomoku_board board;
        gomoku_chess mine;
        int score[gomoku_board::WIDTH][gomoku_board::WIDTH];

        int getscore() const {
            int myscore = 0;
            int opscore = 0;
            for(int i = 0; i < gomoku_board::WIDTH; i++) {
                for(int j = 0; j < gomoku_board::WIDTH; j++) {
                    if(board.getchess(i, j) == mine)
                        myscore += score[i][j];
                    else if(board.getchess(i, j) == oppof(mine))
                        opscore += score[i][j];
                }
            }
            return myscore - opscore;
        }
    };

    score_board new_score_board(gomoku_board const & board) {
        score_board ret;
        ret.board = board;
        ret.mine = mine;
        for(int i = 0; i < gomoku_board::WIDTH; i++) {
            for(int j = 0; j < gomoku_board::WIDTH; j++) {
                ret.score[i][j] = evaluate_pos(board, {i, j});
            }
        }
        return ret;
    }

    score_board new_score_board(
        score_board const & sboard, point_t next_pos, gomoku_chess next_chess
    ) {
        score_board ret;
        ret.board = sboard.board;
        ret.mine = sboard.mine;
        std::copy(
            &sboard.score[0][0],
            &sboard.score[0][0] + gomoku_board::WIDTH * gomoku_board::WIDTH,
            &ret.score[0][0]
        );

        ret.board.setchess(next_pos.x, next_pos.y, next_chess);

        /*
        for(auto dir : directions) {
            for(int fac = -4; fac <= 4; fac++) {
                point_t pos = {next_pos.x + fac * dir.x, next_pos.y + fac * dir.y};
                if(sboard.board.getchess(pos) != OUTBX)
                    ret.score[pos.x][pos.y] = evaluate_pos(ret.board, pos);
            }
        }
        */

        return ret;
    }

    std::tuple<point_t, int> search(
        score_board const & curr_sboard, gomoku_chess next, int depth
    ) {
        int max_score = -10000000;
        int min_score = 10000000;
        point_t max_pos, min_pos;

        // if(depth == 1)
            // logger << "========begin==========\n";

        for(auto & candidate : candidates) {
            int score;
            point_t pos{candidate.x, candidate.y};

            if(curr_sboard.board.getchess(pos) != EMPTY)
                continue;

            score_board next_sboard = new_score_board(curr_sboard, pos, next);

            if(depth == 1) {
                // score = next_sboard.score;
                score = next_sboard.getscore();
                /*
                int score2 = evaluate(next_sboard.board);
                if(score != score2) {
                    display(next_sboard.board, {0, 0});
                    for(int i = 0; i < gomoku_board::WIDTH; i++) {
                        for(int j = 0; j < gomoku_board::WIDTH; j++) {
                            int score_new = next_sboard.score[i][j];
                            int score_old = evaluate_pos(next_sboard.board, point_t{i, j});
                            if(score_new != score_old) {
                                move(22, 0);
                                printw("[%d, %d] old %d new %d\n", i, j, score_old, score_new);
                            }
                        }
                    }
                    getch();
                }
                */
            } else {
                auto [_, _score] = search(next_sboard, oppof(next), depth - 1);
                score = _score;
            }

            if(score > max_score) {
                max_score = score;
                max_pos = pos;
            }
            if(score < min_score) {
                min_score = score;
                min_pos = pos;
            }
        }
        if(depth == 1) {
            // logger << "========end==========\n";
            logger.flush();
        }

        return (depth % 2 == 0) ? std::make_tuple(max_pos, max_score)
                : std::make_tuple(min_pos, min_score);
    }

    int alphabeta(
        score_board const & curr_sboard, gomoku_chess next,
        int alpha, int beta, bool ismax, int depth
    ) {
        /*
        logger << "alphabeta " << alpha << " " << beta
               << ((ismax)? " max " : " min ") << depth << "\n";
        logger.flush();
        */

        depth_cnt[depth]++;

        if(depth == 0) {
            int score = 0;
            auto elap = benchmark([&, this](){
                score = ::evaluate_board(curr_sboard.board, this->mine);
            });
            logger << "evaluate_board elapsed " << elap << '\n';
            logger.flush();
            return score;
        }

        if(ismax) {
            int score = -10000000;
            for(auto & candidate : candidates) {
                point_t pos{candidate.x, candidate.y};

                if(curr_sboard.board.getchess(pos) != EMPTY)
                    continue;

                score_board next_sboard = new_score_board(curr_sboard, pos, next);

                int _score = alphabeta(
                    next_sboard, oppof(next), alpha, beta, false, depth - 1
                );

                score = std::max(score, _score);
                alpha = std::max(alpha, score);

                if(beta <= alpha)
                    break;
            }
            return score;
        }
        // ismin
        int score = 10000000;
        for(auto & candidate : candidates) {
            point_t pos{candidate.x, candidate.y};

            if(curr_sboard.board.getchess(pos) != EMPTY)
                continue;

            score_board next_sboard = new_score_board(curr_sboard, pos, next);

            int _score = alphabeta(
                next_sboard, oppof(next), alpha, beta, true, depth - 1
            );

            score = std::min(score, _score);
            beta = std::min(beta, score);

            if(beta <= alpha)
                break;
        }
        return score;
    }

    // root alpha beta search node
    point_t alphabeta_search(
        score_board const & curr_sboard, gomoku_chess next, int depth
    ) {
        assert(depth % 2 == 0);

        int alpha = -10000000;
        int beta = 10000000;

        point_t bestpos{-1, -1};
        int score = -10000000;
        for(auto & candidate : candidates) {
            point_t pos{candidate.x, candidate.y};

            if(curr_sboard.board.getchess(pos) != EMPTY)
                continue;

            score_board next_sboard = new_score_board(curr_sboard, pos, next);

            int _score = alphabeta(
                next_sboard, oppof(next), alpha, beta, false, depth - 1
            );

            bestpos = (_score > score) ? pos : bestpos;

            score = std::max(score, _score);
            alpha = std::max(alpha, score);

            if(beta <= alpha)
                break;
        }
        return bestpos;
    }

public:
    // just for debug
    int depth_cnt[4];

    gomoku_ai(gomoku_chess chess): mine(chess) { init_candidates(); }

    point_t getnext(gomoku_board const & board) {
        _last_eval_times = 0;

        for(int i = 0; i < 4; i++)
            depth_cnt[i] = 0;

        score_board sboard = new_score_board(board);
        point_t pos = alphabeta_search(sboard, mine, 2);

        /*
        auto [pos_old, score_old] = search(sboard, mine, 2);
        logger << "pos[" << pos.x << ", " << pos.y << "]"
               << " pos_old[" << pos_old.x << ", " << pos_old.y << "]\n";
        */
        // logger << "pos[" << pos.x << ", " << pos.y << "]\n";

        /*
        logger << " depth0: "  << depth_cnt[0]
               << " depth1: " << depth_cnt[1]
               << " depth2: " << depth_cnt[2]
               << " depth3: " << depth_cnt[3] << "\n";

        logger.flush();
        */

        /*
        score_board sboard = new_score_board(board);
        auto [pos, score] = search(sboard, mine, 2);
        */
        return pos;
    }

    int last_eval_times() { return _last_eval_times; }
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

            auto elapsed = benchmark([&](){ ai_next = ai.getnext(board); });
            logger << "get next elapsed " << elapsed << ", eval times " << ai.last_eval_times() << '\n';
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

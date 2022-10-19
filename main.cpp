#include <cassert>
#include <ncurses.h>

#include <tuple>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "bench.hpp"

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

// gomoku_evaler is used for evaluating pattern
class gomoku_evaler {
public:
    // desc_notion is the descriptor for describing line pattern
    // X for self, O for opp, E for empty
    enum desc_notation { X, O, E };

    // desc_t contains a vector of desc_notion to denote a pattern
    using desc_t = std::vector<desc_notation>;

    std::string desc_to_string(desc_t const & desc) {
        std::string str;
        for(auto const & n : desc) {
            switch(n) {
            case X: str += "X"; break;
            case O: str += "O"; break;
            case E: str += "E"; break;
            }
        }
        return str;
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
    const int pattern_score[7] = {10, 20, 100, 1000, 2000, 10000, 100000};

    std::vector<desc_t> patterns[7];

    std::unordered_map<std::string, int> pattern_scores;

    void init_patterns(void) {
        patterns[SICK2].push_back(desc_t{O, X, X, E});
        patterns[SICK2].push_back(desc_t{E, X, X, O});

        patterns[FLEX2].push_back(desc_t{E, X, X, E});
        patterns[FLEX2].push_back(desc_t{E, X, E, X, E});

        patterns[SICK3].push_back(desc_t{E, X, X, X, O});
        patterns[SICK3].push_back(desc_t{O, X, X, X, E});
        patterns[SICK3].push_back(desc_t{O, X, X, E, X, E});
        patterns[SICK3].push_back(desc_t{E, X, X, E, X, O});
        patterns[SICK3].push_back(desc_t{O, X, E, X, X, E});
        patterns[SICK3].push_back(desc_t{E, X, E, X, X, O});

        patterns[FLEX3].push_back(desc_t{E, X, X, X, E});
        patterns[FLEX3].push_back(desc_t{E, X, X, E, X, E});
        patterns[FLEX3].push_back(desc_t{E, X, E, X, X, E});

        patterns[SICK4].push_back(desc_t{E, X, X, X, X, O});
        patterns[SICK4].push_back(desc_t{O, X, X, X, X, E});
        patterns[SICK4].push_back(desc_t{X, X, E, X, X});

        patterns[FLEX4].push_back(desc_t{E, X, X, X, X, E});

        patterns[FLEX5].push_back(desc_t{X, X, X, X, X});
    }

    std::vector<desc_t> add_projection(std::vector<desc_t> & descs) {
        std::vector<desc_t> ret;
        for(auto desc : descs) {
            desc.push_back(X);
            ret.push_back(desc);
            desc.pop_back();

            desc.push_back(O);
            ret.push_back(desc);
            desc.pop_back();

            desc.push_back(E);
            ret.push_back(desc);
            desc.pop_back();
        }
        return ret;
    }

    void init_pattern_map(void) {
        init_patterns();

        std::vector<desc_t> descs;
        descs.push_back(desc_t{X});
        descs.push_back(desc_t{O});
        descs.push_back(desc_t{E});

        for(int i = 0; i < 8; i++)
            descs = add_projection(descs);

        for(auto & desc : descs) {
            pattern_scores.emplace(desc_to_string(desc), eval_line(desc));
        }
    }

    gomoku_evaler() { init_pattern_map(); }

    bool same_desc(desc_t d1, desc_t d2) {
        if(d1.size() != d2.size()) return false;
        for(int i = 0; i < d1.size(); i++)
            if(d1[i] != d2[i]) return false;
        return true;
    }

    std::vector<desc_t> slide_desc(desc_t desc, int slide_size) {
        assert(desc.size() == 9 && desc.size() > slide_size);

        desc_t to_slide;
        if(slide_size > 4)
            to_slide = desc;
        else
            to_slide = desc_t(
                desc.begin() + 4 - (slide_size -1), desc.begin() + 4 + slide_size
            );

        std::vector<desc_t> ret;
        for(int i = 0; i < to_slide.size() - slide_size + 1; i++) {
            ret.push_back(
                desc_t(to_slide.begin()+i, to_slide.begin()+i+slide_size)
            );
        }
        return ret;
    }

    bool match_desc(desc_t to_match, desc_t desc) {
        std::vector<desc_t> slides = slide_desc(desc, to_match.size());
        for(auto slide : slides)
            if(same_desc(to_match, slide))
                return true;
        return false;
    }

    bool match_pattern(std::vector<desc_t> const & pattern, desc_t line_desc) {
        for(auto desc : pattern)
            if(match_desc(desc, line_desc))
                return true;
        return false;
    }

    int eval_line(desc_t line_desc) {
        for(int i = FLEX5; i >= 0; i--)
            if(match_pattern(patterns[i], line_desc))
                return pattern_score[i];
        return 0;
    }

    int get_line_score(desc_t line_desc) {
        return pattern_scores.at(desc_to_string(line_desc));
    }
};

class gomoku_ai {
private:
    gomoku_chess mine;

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

        std::sort(candidates.begin(), candidates.end(), compare_priority);
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
        int score = 0;
        for(auto & dir : directions) {
            auto points = mkline(pos, dir);
            auto line_desc = points_to_desc(board, points);
            score += evaler.get_line_score(line_desc);
        }
        return score;
    }

    int evaluate(gomoku_board const & board) {
        int me_total = 0;
        int op_total = 0;
        for(int i = 0; i < gomoku_board::WIDTH; i++) {
            for(int j = 0; j < gomoku_board::WIDTH; j++) {
                auto chess = board.getchess(i, j);
                if(chess == EMPTY)
                    continue;
                if(chess == mine)
                    me_total += evaluate_pos(board, {i, j});
                else
                    op_total += evaluate_pos(board, {i, j});
            }
        }
        return me_total - op_total;
    }

    std::tuple<point_t, int> search(
        gomoku_board & board, gomoku_chess next, int depth
    ) {
        int max_score = -10000000;
        int min_score = 10000000;
        point_t max_pos, min_pos;

        for(auto & candidate : candidates) {
            int score;
            point_t pos{candidate.x, candidate.y};

            if(board.getchess(pos) != EMPTY)
                continue;

            board.setchess(pos.x, pos.y, next);

            if(depth == 1) {
                score = evaluate(board);
            } else {
                auto [_, _score] = search(board, oppof(next), depth - 1);
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

            board.setchess(pos.x, pos.y, EMPTY);
        }

        return (depth % 2 == 0) ? std::make_tuple(max_pos, max_score)
                : std::make_tuple(min_pos, min_score);
    }

public:
    gomoku_ai(gomoku_chess chess): mine(chess) { init_candidates(); }

    point_t getnext(gomoku_board const & board) {
        auto board_copy = gomoku_board(board);
        auto [pos, score] = search(board_copy, mine, 2 /* search depth */);
        return pos;
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
            logger << "get next elapsed " << elapsed << '\n';
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
        turn = oppof(turn);
    }

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

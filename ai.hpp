#pragma once

#include <tuple>
#include <ranges>
#include <vector>
#include <algorithm>

#include "log.hpp"
#include "debug.hpp"
#include "bench.hpp"
#include "board.hpp"
#include "evaluate.hpp"

namespace nara {

struct score_board {
private:
    // ori.x ori.y dir
    nara::eval_result _score_blk[15][15][4];
    nara::eval_result _score_wht[15][15][4];

    int _dirty[15][15][4];

    int _total_blk;
    int _total_wht;

    int idx_of_dir(point_t dir) const
    {
        if(dir == point_t{0, 1}) return 0;
        if(dir == point_t{1, 0}) return 1;
        if(dir == point_t{1, 1}) return 2;
        if(dir == point_t{-1, 1}) return 3;
        assert(false);
    }

    point_t origin_at_line(point_t pos, point_t dir)
    {
        for(;;)
        {
           point_t back_pos = {pos.x - dir.x, pos.y - dir.y};
           if(gomoku_board::outbox(back_pos))
               return pos;
           pos = back_pos;
        }
    }

    std::vector<line_t> lines_contain_pos(point_t pos)
    {
        std::vector<line_t> lines;
        for(auto dir : directions)
            lines.emplace_back(line_t{origin_at_line(pos, dir), dir});
        return lines;
    }

public:
    gomoku_board _board;

    point_t last_pos;

    score_board() = delete;

    score_board(gomoku_board const & board): _board(board)
    {
        _total_blk = _total_wht = 0;
        for(auto line : all_lines)
        {
            auto [res_blk, res_wht] = evaluate_line(_board, line);
            _score_blk[line.ori.x][line.ori.y][idx_of_dir(line.dir)] = res_blk;
            _score_wht[line.ori.x][line.ori.y][idx_of_dir(line.dir)] = res_wht;
            _total_blk += res_blk.score;
            _total_wht += res_wht.score;
        }
    }

    score_board(
        score_board const & prev_sboard,
        gomoku_chess next_chess,
        point_t next_pos)
    {
        _board = gomoku_board(prev_sboard._board);
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

        setchess(next_pos, next_chess);
    }

    void setchess(point_t next_pos, gomoku_chess next_chess)
    {
        last_pos = next_pos;
        _board.setchess(next_pos, next_chess);
        for(auto & line : lines_contain_pos(next_pos))
        {
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

    gomoku_chess getchess(point_t pos) const
    {
        return _board.getchess(pos);
    }

    eval_result line_result(gomoku_chess chess, point_t ori, point_t dir) const
    {
        return (chess == BLACK)
            ? _score_blk[ori.x][ori.y][idx_of_dir(dir)]
            : _score_wht[ori.x][ori.y][idx_of_dir(dir)];
    }

    eval_result line_result(gomoku_chess chess, line_t line) const
    {
        return line_result(chess, line.ori, line.dir);
    }

    int total_score(gomoku_chess chess) const
    {
        return (chess == BLACK) ? _total_blk : _total_wht;
    }

    int score_blk() const { return _total_blk - _total_wht; }
    int score_wht() const { return _total_wht - _total_blk; }

    gomoku_chess winner() const
    {
        if (_total_blk >= pattern_score[FLEX5])
            return BLACK;
        else if (_total_wht >= pattern_score[FLEX5])
            return WHITE;
        return EMPTY;
    }

    template <typename VP>
    std::vector<point_t> get_to_points(gomoku_chess chess, VP vp) const
    {
        std::vector<point_t> ret;
        auto & res = (chess == BLACK) ? _score_blk : _score_wht;
        for(auto line : all_lines)
        {
            auto idxs =
                res[line.ori.x][line.ori.y][idx_of_dir(line.dir)].*vp;
            for(int i : idxs)
                ret.emplace_back(point_t{
                    line.ori.x + i * line.dir.x,
                    line.ori.y + i * line.dir.y
                });
        }
        return ret;
    }

    std::vector<point_t> to_sick4_points(gomoku_chess chess) const
    {
        return get_to_points(chess, &eval_result::to_sick4);
    }

    std::vector<point_t> to_flex4_points(gomoku_chess chess) const
    {
        return get_to_points(chess, &eval_result::to_flex4);
    }

    std::vector<point_t> to_five_points(gomoku_chess chess) const
    {
        return get_to_points(chess, &eval_result::to_five);
    }

    std::tuple<eval_result, eval_result> summary() const
    {
        eval_result blk, wht;
        for(auto line : all_lines)
        {
            auto res_blk = line_result(BLACK, line);
            auto res_wht = line_result(WHITE, line);
            for(int i = SICK2; i <= FLEX5; i++)
            {
                blk.score += res_blk.score;
                wht.score += res_wht.score;
                blk.pattern_cnt[i] += res_blk.pattern_cnt[i];
                wht.pattern_cnt[i] += res_wht.pattern_cnt[i];
            }
        }
        return std::make_tuple(blk, wht);
    }
};

void debug_display(gomoku_board const & board, score_board const & sboard)
{
    logger << "======================" << std::endl;
    for(int i = 0; i < gomoku_board::WIDTH; i++)
    {
        for(int j = 0; j < gomoku_board::WIDTH; j++)
        {
            switch (board.getchess(i, j)) {
            case BLACK: logger << "X"; break;
            case WHITE: logger << "O"; break;
            case EMPTY: logger << "-"; break;
            }
            logger << " ";
        }
        logger << std::endl;
    }

    auto [res_blk, res_wht] = sboard.summary();

    logger << "result score: "<< sboard.total_score(BLACK);
    logger << "SICK2: "<< res_blk.pattern_cnt[SICK2];
    logger << "FLEX2: "<< res_blk.pattern_cnt[FLEX2];
    logger << "SICK3: "<< res_blk.pattern_cnt[SICK3];
    logger << "FLEX3: "<< res_blk.pattern_cnt[FLEX3];
    logger << "SICK4: "<< res_blk.pattern_cnt[SICK4];
    logger << "FLEX4: "<< res_blk.pattern_cnt[FLEX4];
    logger << "FLEX5: "<< res_blk.pattern_cnt[FLEX5];
    logger << "to SICK4: ";
    for(auto p : sboard.to_sick4_points(BLACK))
    {
        logger << p << " ";
    }
    logger << "to FLEX4: ";
    for(auto p : sboard.to_flex4_points(BLACK))
    {
        logger << p << " ";
    }
    logger << "to FIVE: ";
    for(auto p : sboard.to_five_points(BLACK))
    {
        logger << p << " ";
    }
    logger << std::endl;

    logger << "result score: "<< sboard.total_score(WHITE);
    logger << "SICK2: "<< res_wht.pattern_cnt[SICK2];
    logger << "FLEX2: "<< res_wht.pattern_cnt[FLEX2];
    logger << "SICK3: "<< res_wht.pattern_cnt[SICK3];
    logger << "FLEX3: "<< res_wht.pattern_cnt[FLEX3];
    logger << "SICK4: "<< res_wht.pattern_cnt[SICK4];
    logger << "FLEX4: "<< res_wht.pattern_cnt[FLEX4];
    logger << "FLEX5: "<< res_wht.pattern_cnt[FLEX5];
    logger << "to SICK4: ";
    for(auto p : sboard.to_sick4_points(BLACK))
    {
        logger << p << " ";
    }
    logger << "to FLEX4: ";
    logger << std::endl;
    for(auto p : sboard.to_flex4_points(WHITE))
    {
        logger << p << " ";
    }
    logger << std::endl;
    logger << "to FIVE: ";
    for(auto p : sboard.to_five_points(WHITE))
    {
        logger << p << " ";
    }
    logger << std::endl;

    logger << "======================" << std::endl;

    logger.flush();
}

class gomoku_ai {
private:
    // my chess
    gomoku_chess mine;

    // candidates are all chess positions but sort by priority
    points_t candidates;

    // prior of a candidate depends on its distance from board center.
    int prior_of(point_t p)
    {
        static const point_t board_center =
            { gomoku_board::WIDTH / 2, gomoku_board::WIDTH / 2 };

        int dx = p.x - board_center.x;
        int dy = p.y - board_center.y;
        return -(dx * dx + dy * dy);
    }

    // TODO: init candidates at compile time
    void init_candidates(void)
    {
        for(int i = 0; i < gomoku_board::WIDTH; i++)
            for(int j = 0; j < gomoku_board::WIDTH; j++)
                candidates.emplace_back(point_t{i, j});

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
    line_for_pos(point_t pos, point_t dir, int max_fac)
    {
        std::vector<point_t> line;
        for(int fac = -max_fac; fac <= max_fac; fac++)
        {
            line.emplace_back(
                point_t{ pos.x + fac * dir.x, pos.y + fac * dir.y }
            );
        }
        return line;
    }

    std::vector<point_t>
    line_for_pos(point_t pos, point_t dir)
    {
        return line_for_pos(pos, dir, 4);
    }

    std::vector<nara::gomoku_chess>
    gen_chesses(gomoku_board const & board, std::vector<point_t> line)
    {
        std::vector<nara::gomoku_chess> chesses;
        for(auto & p : line)
        {
            if (!board.outbox(p))
                chesses.push_back(board.getchess(p));
        }
        return chesses;
    }

    choose_t gen_choose(gomoku_board const & board, point_t pos)
    {
        assert(board.getchess(pos) == EMPTY);
        choose_t choose{pos, 0, 0, 0};
        for(auto dir : directions)
        {
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

    static bool with_score(choose_t choose)
    {
        return choose.score_blk != 0 || choose.score_wht != 0;
    }

    auto choose_generator(gomoku_board const & board)
    {
        return [&](point_t pos){ return gen_choose(board, pos); };
    }

    auto is_empty_at(score_board const & sboard)
    {
        return [&](point_t pos) { return sboard.getchess(pos) == EMPTY; };
    }

    auto has_neigh_at(score_board const & sboard)
    {
        return [&](point_t pos) {
            for(auto dir : directions)
            {
                auto line = line_for_pos(pos, dir, 2);
                for(auto p : line)
                {
                    if (gomoku_board::outbox(p)) continue;
                    if (sboard.getchess(p) != EMPTY) return true;
                }
            }
            return false;
        };
    }

    std::vector<choose_t>
    gen_chooses(score_board const & sboard, gomoku_chess next)
    {
        std::vector<choose_t> chooses;

        auto choose_transformer = choose_generator(sboard._board);

        // fast choose path

        auto my_to_five = sboard.to_five_points(next);
        if (!my_to_five.empty()) {
            std::ranges::copy(
                my_to_five | std::views::transform(choose_transformer),
                std::back_inserter(chooses)
            );
            return chooses;
        }

        auto opp_to_five = sboard.to_five_points(oppof(next));
        if (!opp_to_five.empty()) {
            std::ranges::copy(
                opp_to_five | std::views::transform(choose_transformer),
                std::back_inserter(chooses)
            );
            return chooses;
        }

        auto my_to_flex4  = sboard.to_flex4_points(next);
        auto opp_to_flex4 = sboard.to_flex4_points(oppof(next));

        if (!my_to_flex4.empty()) {
            std::ranges::copy(
                my_to_flex4 | std::views::transform(choose_transformer),
                std::back_inserter(chooses)
            );
            return chooses;
        }

        if (!opp_to_flex4.empty()) {
            // debug_display(sboard._board, sboard);
            auto my_to_sick4 = sboard.to_sick4_points(next);
            std::vector<std::vector<point_t>> v{ opp_to_flex4, my_to_sick4 };
            std::ranges::copy(
                std::ranges::join_view(v) |
                std::views::transform(choose_transformer),
                std::back_inserter(chooses)
            );
            if (!chooses.empty()) return chooses;
        }

        // search and sort chooses

        std::ranges::copy(
            candidates |
            std::views::filter(is_empty_at(sboard)) |
            std::views::filter(has_neigh_at(sboard)) |
            std::views::transform(choose_transformer),
            std::back_inserter(chooses)
        );

        if (chooses.empty()) {
            std::ranges::copy(
                candidates |
                std::views::filter(is_empty_at(sboard)) |
                std::views::transform(choose_transformer) |
                std::views::take(9),
                std::back_inserter(chooses)
            );
        }

        return chooses;
    }

    std::tuple<point_t, int> alphabeta(
        score_board const & sboard, gomoku_chess next,
        int alpha, int beta, bool ismax, int depth)
    {
        if (depth < 20)
            depth_cnt[depth]++;

        if (depth == 0) {
            if(mine == BLACK)
                return std::make_tuple(sboard.last_pos, sboard.score_blk());
            else
                return std::make_tuple(sboard.last_pos, sboard.score_wht());
        }

        auto chooses = gen_chooses(sboard, next);

        if (depth == 4) {
            for (auto & choose : chooses)
            {
                logger << choose.pos << " ";
            }
            logger << "\n";
            logger.flush();
        }

        point_t bestpos;

        if(ismax) {
            int score = -10000000;
            for(auto & choose : chooses)
            {
                auto next_sboard = score_board(sboard, next, choose.pos);
                if (next_sboard.winner() == next)
                    return std::make_tuple(choose.pos, score_win);

                auto[_ , _score] =
                    alphabeta(next_sboard, oppof(next), alpha, beta, false, depth - 1);

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
        for(auto & choose : chooses)
        {
            auto next_sboard = score_board(sboard, next, choose.pos);
            if (next_sboard.winner() == next)
                return std::make_tuple(choose.pos, -score_win);

            auto[_ , _score] =
                alphabeta(next_sboard, oppof(next), alpha, beta, true, depth - 1);

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
    int depth_cnt[20];

    gomoku_ai(gomoku_chess chess): mine(chess) { init_candidates(); }

    point_t getnext(gomoku_board const & board)
    {
        for(int i = 0; i < 20; i++)
            depth_cnt[i] = 0;

        last_eval_times = 0;

        // auto [pos, score] = alphabeta(score_board(board), mine, -10000000, 10000000, true, 4);

        auto start = std::chrono::system_clock::now();

        auto [pos, score] = alphabeta(score_board(board), mine, -10000000, 10000000, true, 4);

        auto end = std::chrono::system_clock::now();

        logger << "get next use "
               << std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
               << " eval times: " << last_eval_times
               << '\n';

        logger << "depth0: "  << depth_cnt[0]
               << " depth1: " << depth_cnt[1]
               << " depth2: " << depth_cnt[2]
               << " depth3: " << depth_cnt[3] << "\n";

        logger.flush();

        return pos;
    }
};

} // namespace nara

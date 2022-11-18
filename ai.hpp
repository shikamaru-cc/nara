#pragma once

#include <cassert>
#include <algorithm>
#include <array>
#include <tuple>
#include <ranges>
#include <unordered_map>

#include "log.hpp"
#include "eval.hpp"
#include "board.hpp"
#include "zobrist.hpp"

namespace nara
{

class gomoku_ai
{
  private:

    gomoku_chess mine;

    gomoku_board board;

    chess_state states[15][15];

    int depth_tracker[50];

    int cache_hit;

    struct search_res_t
    {
        int depth;
        int score;
        point_t p;

        search_res_t(int _depth, int _score, point_t _p): depth(_depth), score(_score), p(_p) {}
    };

    zobrist_t zob;

    std::unordered_map<zobrist_t, search_res_t, zobrist_hash, zobrist_eq> zob_table;

    static constexpr auto initial_chooses()
    {
        std::array<std::tuple<int, int>, 15*15> res;
        int x = 0;
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 15; j++)
                res[x++] = std::make_tuple(i, j);
        return res;
    }

    void setchess(point_t pos, gomoku_chess chess)
    {
        board.setchess(pos, chess);
        zob[pos.x][pos.y] = zobrist_val(pos, chess);

        for (int dir = 0; dir < 4; dir++)
        {
            for (int fac = -4; fac <= 4; fac++)
            {
                auto p = pos + dir * fac;
                if (fac == 0 or board.outbox(p))
                    continue;
                states[p.x][p.y].update_chess(chess, dir, -fac);
            }
        }
    }

    std::vector<point_t> gen_chooses(gomoku_board const& board, gomoku_chess next)
    {
        static constexpr auto all_points = initial_chooses();

        std::vector<point_t> me_five, op_five;
        std::vector<point_t> me_flex4, op_flex4;
        std::vector<point_t> me_b4b4, op_b4b4;
        std::vector<point_t> me_b4f3, op_b4f3;
        std::vector<point_t> me_2flex3, op_2flex3;
        std::vector<point_t> me_block4, me_flex3;

        std::vector<point_t> ret;

        auto to_point = [&](std::tuple<int, int> p)
        {
            return point_t{std::get<0>(p), std::get<1>(p)};
        };

        for (auto p : all_points | std::views::transform(to_point))
        {
            auto state = states[p.x][p.y];

            if (board.getchess(p) != EMPTY)
                continue;

            if (not state.has_neighbor())
                continue;

            auto we_have = state.cats_for(next);
            auto op_have = state.cats_for(oppof(next));

            if (we_have[FIVE])
            {
                me_five.push_back(p);
            }
            if (op_have[FIVE])
            {
                op_five.push_back(p);
            }

            if (we_have[FLEX4])
            {
                me_flex4.push_back(p);
            }
            if (op_have[FLEX4])
            {
                op_flex4.push_back(p);
            }

            if (we_have[BLOCK4] > 1)
            {
                me_b4b4.push_back(p);
            }
            if (op_have[BLOCK4] > 1)
            {
                op_b4b4.push_back(p);
            }

            if (we_have[BLOCK4] and we_have[FLEX3])
            {
                me_b4f3.push_back(p);
            }
            if (op_have[BLOCK4] and op_have[FLEX3])
            {
                op_b4f3.push_back(p);
            }

            if (we_have[FLEX3] > 1)
            {
                me_2flex3.push_back(p);
            }
            if (op_have[FLEX3] > 1)
            {
                op_2flex3.push_back(p);
            }

            if (we_have[BLOCK4])
            {
                me_block4.push_back(p);
            }
            if (we_have[FLEX3])
            {
                me_flex3.push_back(p);
            }

            ret.push_back(p);
        }

        if (not me_five.empty()) return me_five;

        if (not op_five.empty()) return op_five;

        if (not me_flex4.empty()) return me_flex4;

        if (not me_b4b4.empty()) return me_b4b4;

        if (not me_b4f3.empty()) return me_b4f3;

        if (not op_flex4.empty())
        {
            auto ret = op_flex4;
            ret.insert(ret.end(), me_block4.begin(), me_block4.end());
            return ret;
        }

        if (not op_b4b4.empty())
        {
            auto ret = op_b4b4;
            ret.insert(ret.end(), me_block4.begin(), me_block4.end());
            return ret;
        }

        if (not op_b4f3.empty())
        {
            auto ret = op_b4f3;
            ret.insert(ret.end(), me_block4.begin(), me_block4.end());
            return ret;
        }

        if (not me_2flex3.empty()) return me_2flex3;

        if (not op_2flex3.empty())
        {
            auto ret = op_2flex3;
            ret.insert(ret.end(), me_block4.begin(), me_block4.end());
            ret.insert(ret.end(), me_flex3.begin(), me_flex3.end());
            return ret;
        }

        if (ret.empty())
        {
            ret.push_back({7,7});
        }

        auto sorter = [this, next](point_t p1, point_t p2)
        {
            return this->states[p1.x][p1.y].rankof(next) > this->states[p2.x][p2.y].rankof(next);
        };

        auto start = std::chrono::system_clock::now();

        std::ranges::sort(ret, sorter);

        auto end = std::chrono::system_clock::now();
        logger << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start) << std::endl;
        logger.flush();

        return ret;
    }

    int evaluate(gomoku_chess chess)
    {
        int val = 0;
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 15; j++)
                if (board.getchess(i, j) == chess)
                    val += states[i][j].rankof(chess);
        return val;
    }

  public:
    gomoku_ai(gomoku_chess chess): mine(chess) { init_zobrist(); }

    search_res_t
    alphabeta(point_t last_move, gomoku_chess next, int alpha, int beta, bool ismax, int depth)
    {
        if (depth < 50) depth_tracker[depth]++;

        // cache hit
        if (zob_table.find(zob) != zob_table.end())
        {
            auto res = zob_table.at(zob);
            if (res.depth >= depth)
            {
                cache_hit++;
                return res;
            }
        }

        if (depth == 0)
            return search_res_t(depth, evaluate(mine) - evaluate(oppof(mine)), last_move);

        auto chooses = gen_chooses(board, next);

        // if (chooses.size() == 1)
        // {
        //     setchess(chooses[0], next);
        //     auto res = alphabeta(chooses[0], oppof(next), alpha, beta, false, 0);
        //     setchess(chooses[0], EMPTY);
        //     return res;
        // }

        point_t bestpos;
        zobrist_t bestzob;

        if (ismax)
        {
            int score = score_lose;
            for (auto & choose : chooses)
            {
                setchess(choose, next);

                // win
                if (states[choose.x][choose.y].has_category(next, FIVE))
                {
                    auto res = search_res_t(depth, score_win, choose);
                    zob_table.insert_or_assign(zob, res);
                    setchess(choose, EMPTY);
                    return res;
                }

                auto res = alphabeta(choose, oppof(next), alpha, beta, false, depth - 1);

                if(res.score > score)
                {
                    bestpos = choose;
                    bestzob = zob;
                }

                setchess(choose, EMPTY);

                score = std::max(score, res.score);
                alpha = std::max(alpha, score);

                if(beta <= alpha)
                    break;
            }
            auto res = search_res_t(depth, score, bestpos);
            zob_table.insert_or_assign(bestzob, res);
            return res;
        }

        // ismin
        int score = score_win;
        for (auto & choose : chooses)
        {
            setchess(choose, next);

            // win
            if (states[choose.x][choose.y].has_category(next, FIVE))
            {
                auto res = search_res_t(depth, score_lose, choose);
                zob_table.insert_or_assign(zob, res);
                setchess(choose, EMPTY);
                return res;
            }

            auto res = alphabeta(choose, oppof(next), alpha, beta, true, depth - 1);

            if(res.score < score)
            {
                bestpos = choose;
                bestzob = zob;
            }

            setchess(choose, EMPTY);

            score = std::min(score, res.score);
            beta = std::min(beta, score);

            if(beta <= alpha)
                break;
        }
        auto res = search_res_t(depth, score, bestpos);
        zob_table.insert_or_assign(bestzob, res);
        return res;
    }

    void reset_board(gomoku_board const& _board)
    {
        for(int i = 0; i < 15; i++)
            for(int j = 0; j < 15; j++)
                board.chesses[i][j] = _board.chesses[i][j];
    }

    void reset_states()
    {
        for(int i = 0; i < 15; i++)
            for(int j = 0; j < 15; j++)
                states[i][j] = get_state(board, i, j);
    }

    void reset_zob()
    {
        for(int i = 0; i < 15; i++)
            for(int j = 0; j < 15; j++)
                zob[i][j] = zobrist_val(i, j, board.getchess(i, j));
    }

    void reset_tracker()
    {
        cache_hit = 0;
        for (int i = 0; i < 50; i++)
            depth_tracker[i] = 0;
    }

    point_t get_next(gomoku_board const& _board)
    {
        static const int max_depth = 8;

        reset_board(_board);
        reset_states();
        reset_zob();
        reset_tracker();
        
        auto res = alphabeta({0, 0}, mine, score_lose, score_win, true, max_depth);

        int node_total = 0;
        for (int i = 0; i <= max_depth; i++)
        {
            node_total += depth_tracker[i];
            logger << "depth" << "[" << i << "]: " << depth_tracker[i] << ' ';
        }
        logger << std::endl;

        logger << "node total: " << node_total
               << " cache hit: " << cache_hit
               << " hit rate " << (double)cache_hit / (double)node_total
               << std::endl;

        logger.flush();

        return res.p;
    }
};

} // namespace nara

#pragma once

#include <cassert>
#include <algorithm>
#include <array>
#include <tuple>
#include <ranges>

#include "log.hpp"
#include "eval.hpp"
#include "board.hpp"

namespace nara
{

class gomoku_ai
{
  private:

    gomoku_chess mine;

    gomoku_board board;

    chess_state states[15][15];

    int depth_tracker[50];

    static constexpr auto initial_chooses()
    {
        std::array<std::tuple<int, int>, 15*15> res;
        int x = 0;
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 15; j++)
                res[x++] = std::make_tuple(i, j);
        return res;
    }

    void update_state(point_t pos)
    {
        if (!board.outbox(pos))
            states[pos.x][pos.y] = get_state(board, pos);
    }

    void setchess(point_t pos, gomoku_chess chess)
    {
        board.setchess(pos, chess);
        for (auto dir : directions)
            for (int fac = -4; fac <= 4; fac++)
                update_state(pos + dir * fac);
    }

    // template <typename SS>
    // std::vector<point_t> states_to_points(SS const & ss)
    // {
    //     std::vector<point_t> ret;
    //     for (auto s : ss)
    //         ret.push_back(s.pos);
    //     return ret;
    // }

    template <std::ranges::range SS>
    std::vector<point_t> states_to_points(SS ss)
    {
        std::vector<point_t> ret;
        for (auto s : ss | std::views::all)
            ret.push_back(s.pos);
        return ret;
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

        std::ranges::sort(ret, sorter);

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
    gomoku_ai(gomoku_chess chess): mine(chess) {}

    std::tuple<point_t, int>
    alphabeta(point_t last_move, gomoku_chess next, int alpha, int beta, bool ismax, int depth)
    {
        if (depth < 50) depth_tracker[depth]++;

        if (depth == 0)
            return std::make_tuple(last_move, evaluate(mine) - evaluate(oppof(mine)));

        auto chooses = gen_chooses(board, next);

        point_t bestpos;

        if (ismax)
        {
            int score = score_lose;
            for (auto & choose : chooses)
            {
                setchess(choose, next);

                // win
                if (states[choose.x][choose.y].has_category(next, FIVE))
                {
                    setchess(choose, EMPTY);
                    return std::make_tuple(choose, score_win);
                }

                auto[_ , _score] = alphabeta(choose, oppof(next), alpha, beta, false, depth - 1);

                setchess(choose, EMPTY);

                if(_score > score)
                    bestpos = choose;

                score = std::max(score, _score);
                alpha = std::max(alpha, score);

                if(beta <= alpha)
                    break;
            }
            return std::make_tuple(bestpos, score);
        }

        // ismin
        int score = score_win;
        for (auto & choose : chooses)
        {
            setchess(choose, next);

            // win
            if (states[choose.x][choose.y].has_category(next, FIVE))
            {
                setchess(choose, EMPTY);
                return std::make_tuple(choose, score_lose);
            }

            auto[_ , _score] = alphabeta(choose, oppof(next), alpha, beta, true, depth - 1);

            setchess(choose, EMPTY);

            if(_score < score)
                bestpos = choose;

            score = std::min(score, _score);
            beta = std::min(beta, score);

            if(beta <= alpha)
                break;
        }
        return std::make_tuple(bestpos, score);
    }

    point_t get_next(gomoku_board const& _board)
    {
        board.winner = _board.winner;
        for(int i = 0; i < 15; i++)
            for(int j = 0; j < 15; j++)
                board.chesses[i][j] = _board.chesses[i][j];

        for(int i = 0; i < 15; i++)
            for(int j = 0; j < 15; j++)
                states[i][j] = get_state(board, i, j);

        for (int i = 0; i < 50; i++)
            depth_tracker[i] = 0;
        
        auto [pos, score] = alphabeta({0, 0}, mine, score_lose, score_win, true, 4);

        for (int i = 0; i < 50; i++)
        {
            if (depth_tracker[i] == 0) break;
            logger << "depth" << "[" << i << "]: " << depth_tracker[i] << ' ';
        }
        logger << std::endl;
        logger.flush();

        return pos;
    }
};

} // namespace nara

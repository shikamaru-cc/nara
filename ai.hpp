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

        auto to_point = [&](std::tuple<int, int> p)
        {
            return point_t{std::get<0>(p), std::get<1>(p)};
        };

        auto is_empty = [&](point_t p)
        {
            return board.getchess(p) == EMPTY;
        };

        auto to_state = [this](point_t p)
        {
            return this->states[p.x][p.y];
        };

        auto has_neighbor = [](chess_state s)
        {
            return s.has_neighbor();
        };

        auto candidates = all_points
                        | std::views::transform(to_point)
                        | std::views::filter(is_empty)
                        | std::views::transform(to_state)
                        | std::views::filter(has_neighbor);
        
        std::vector<point_t> ret;

        // TODO: fast path

        // me have five ?
        auto has_five = [next](chess_state s)
        {
            return s.has_category(next, FIVE);
        };

        ret = states_to_points(candidates | std::views::filter(has_five));
        if (not ret.empty())
            return ret;

        // opp has five ?
        auto opp_five = [next](chess_state s)
        {
            return s.has_category(oppof(next), FIVE);
        };

        ret = states_to_points(candidates | std::views::filter(opp_five));
        if (not ret.empty())
            return ret;

        auto opp_flex4 = [next](chess_state s)
        {
            return s.has_category(oppof(next), FLEX4);
        };

        ret = states_to_points(candidates | std::views::filter(opp_flex4));
        if (not ret.empty())
            return ret;

        // sort candidates
        auto sorter = [next](chess_state s1, chess_state s2)
        {
            return s1.rankof(next) > s2.rankof(next);
        };

        std::vector<chess_state> unsorted;
        std::ranges::copy(candidates, std::back_inserter(unsorted));
        std::ranges::sort(unsorted, sorter);

        ret = states_to_points(unsorted);

        if (ret.empty())
            ret.push_back({7, 7});

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
                board.setchess(choose, next);

                // win
                if (states[choose.x][choose.y].has_category(next, FIVE))
                {
                    board.setchess(choose, EMPTY);
                    return std::make_tuple(choose, score_win);
                }

                auto[_ , _score] = alphabeta(choose, oppof(next), alpha, beta, false, depth - 1);

                board.setchess(choose, EMPTY);

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
            board.setchess(choose, next);

            // win
            if (states[choose.x][choose.y].has_category(next, FIVE))
            {
                board.setchess(choose, EMPTY);
                return std::make_tuple(choose, score_lose);
            }

            auto[_ , _score] = alphabeta(choose, oppof(next), alpha, beta, true, depth - 1);

            board.setchess(choose, EMPTY);

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

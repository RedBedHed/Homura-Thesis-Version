//
// Created by evcmo on 8/12/2021.
//

#pragma once
#ifndef HOMURA_BACKTRACK_H
#define HOMURA_BACKTRACK_H

#include "MoveMake.h"
#include "Zobrist.h"
#include "Eval.h"

namespace Homura {

    // Using...
    using std::chrono::milliseconds;
    using std::chrono::time_point;
    using std::chrono::system_clock;
    using namespace MoveFactory;
    using namespace std::chrono;

    /**
     * Late Move Pruning margins by depth.
     */
    constexpr uint8_t lmpMargins[] = 
    { 0, 8, 13, 17, 21, 25 };

    /**
     * The R value for null-move searches.
     */
    constexpr int32_t NULL_R = 2;

    /**
     * Reverse Futility (Static Null Move Pruning)
     * maximum remaining depth.
     */
    constexpr int32_t RFP_RD = 5;

    /**
     * Null Move Pruning minimum remaining depth.
     */
    constexpr int32_t NMP_RD = 2;

    /**
     * Razoring maximum remaining depth.
     */
    constexpr int32_t RAZ_RD = 2;

    /**
     * Internal Iterative Deepening minimum remaining
     * depth.
     */
    constexpr int32_t IID_RD = 4;

    /**
     * Internal Iterative Deepening R value.
     */
    constexpr int32_t IID_R  = 3; 

    /**
     * Late Move Pruning maximum remaining depth.
     */
    constexpr int32_t LMP_RD = 5;

    /**
     * Futility Pruning maximum remaining depth.
     */
    constexpr int32_t FUT_RD = 8;

    /**
     * Late Move Reductions minimum remaining depth.
     */
    constexpr int32_t LMR_RD = 2;

    /**
     * Basic node types enumerated.
     */
    enum NodeType : uint8_t { ROOT, IID, PV, NONPV };

    /**
     * A method to indicate whether the search should
     * abort.
     * 
     * @param time the time allotted
     * @param epoch the starting timestamp
     * @return whether the difference between the
     * current timestamp and the starting timestamp
     * is greater than or equal to the time allotted.
     */
    inline bool abort(int time, timer_t epoch) {
        auto clock = 
        (system_clock::now() - epoch);
        long long ms = duration_cast
        <milliseconds>(clock).count();
        return ms >= time;
    }

    /**
     * A method to measure the elapsed time of
     * the search.
     * 
     * @param epoch the starting timestamp
     * @return the difference between the current 
     * timestamp and the starting timestamp
     */
    inline long long elapsed(timer_t epoch) {
        auto clock = 
        (system_clock::now() - epoch);
        return duration_cast<milliseconds>
        (clock).count();
    }

    /**
     * A full, classical iterative deepening
     * search, for science.
     */
    // Move search(Board*, char*, control&, int);
    
    /**
     * A classical backtracking Alpha-Beta search.
     */
    template<Alliance A, NodeType NT, bool DO_NULL = true>
    int32_t alphaBeta
        (
        Board*, 
        int,
        int,
        int32_t, 
        int32_t,
        control*
        );

    /**
     * A classical backtracking quiescence search.
     */
    template<Alliance A>
    int32_t quiescence
        (
        Board*, 
        int,
        int,
        int32_t, 
        int32_t,
        control*
        );
}
#endif //HOMURA_BACKTRACK_H
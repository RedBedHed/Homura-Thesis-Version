//
// Created by evcmo on 7/4/2021.
//

#pragma once
#ifndef HOMURA_MOVEMAKE_H
#define HOMURA_MOVEMAKE_H

#include "ChaosMagic.h"
#include "Board.h"
#include "Move.h"
#include <cassert>
#include <algorithm>
#include <mutex>

namespace Homura {

    /**
     * TODO: MOVE BACK TO ANON NAMESPACE IN CPP !!!
     *
     * A function to find attackers on the fly.
     *
     * @tparam A    the alliance of the piece
     *              on the square under attack
     * @tparam PT   the piece type
     * @param board the current game board
     * @param sq    the square under attack
     */
    template<Alliance A, PieceType PT> [[nodiscard]]
    constexpr uint64_t attacksOn(Board* const board, const int sq) {
        static_assert(A == White || A == Black);
        static_assert(PT >= Pawn && PT <= NullPT);

        constexpr const Alliance us = A, them = ~us;

        // Initialize constants.
        const uint64_t theirQueens = board->getPieces<them, Queen>(),
                       target      = PT == NullPT? 0: board->getPieces<us, PT>(),
                       allPieces   = board->getAllPieces() & ~target;

        // Calculate and return a bitboard representing all attackers.
        return (attackBoard<Rook>(allPieces, sq)   &
               (board->getPieces<them, Rook>()   | theirQueens)) |
               (attackBoard<Bishop>(allPieces, sq) &
               (board->getPieces<them, Bishop>() | theirQueens)) |
               (SquareToKnightAttacks[sq]   & board->getPieces<them, Knight>()) |
               (SquareToPawnAttacks[us][sq] & board->getPieces<them, Pawn>())   |
               (SquareToKingAttacks[sq]     & board->getPieces<them, King>());
    }

    /**
     * A function to calculate check for our king given a
     * bitboard representing the pieces that attack its
     * square.
     *
     * @param checkBoard a bitboard of all pieces that attack
     * our king
     * @return whether or not our king is in check
     */
    constexpr CheckType
    calculateCheck(uint64_t checkBoard) {
        return !checkBoard ? None:
               (checkBoard & (checkBoard - 1)) ?
               DoubleCheck: Check;
    }

    namespace MoveFactory {

        /**
         * <summary>
         *  <p><br/>
         * A function to generate moves for the given
         * board according to the given filter type,
         * by populating the given list.
         *  </p>
         *  <p>
         *  The filter type must be one of the following:
         *   <ul>
         *    <li>
         *     <b><i>All</i></b>
         *     <p>
         *  This filter will cause the function to
         *  return a list of all legal moves, both
         *  passive and aggressive.
         *     </p>
         *    </li>
         *    <li>
         *     <b><i>Passive</i></b>
         *     <p>
         *  This filter will cause the function to
         *  return a list of all legal non-capture
         *  moves.
         *     </p>
         *    </li>
         *    <li>
         *     <b><i>Aggressive</i></b>
         *     <p>
         *  This filter will cause the function to
         *  return a list of all legal capture moves.
         *     </p>
         *    </li>
         *   </ul>
         *  </p>
         * </summary>
         *
         * @tparam FT the filter type
         * @param board the current game board
         * @param moves an empty list of moves
         */
        template<FilterType FT>
        int generateMoves(Board*, Move*);
        template<FilterType FT>
        int generateMoves(Board*, Move*, Alliance);

        using std::sort;

        constexpr uint8_t val[7][7] = 
        {{ 5,  3,  4,  4,  2,  1, 0}, // Victim P - P R N B Q K 0
         {25, 23, 24, 24, 22, 21, 0}, // Victim R - P R N B Q K 0
         {15, 13, 14, 14, 12, 11, 0}, // Victim N - P R N B Q K 0
         {15, 13, 14, 14, 12, 11, 0}, // Victim B - P R N B Q K 0
         {35, 33, 34, 34, 32, 31, 0}, // Victim Q - P R N B Q K 0
         { 0,  0,  0,  0,  0,  0, 0}, // Victim K - P R N B Q K 0
         { 0,  0,  0,  0,  0,  0, 0}};

        typedef std::chrono::system_clock::time_point timer_t;
        using std::mutex;

        /**
         * A struct containing the search
         * controls and relevant tables.
         */
        struct control final {
            timer_t epoch;
            uint64_t history[2][64][64];
            int64_t evals[MaxDepth];
            int64_t NODES;
            int32_t MAX_DEPTH;
            int32_t NULL_PLY;
            int32_t Q_PLY;
            int32_t time;
            Move killers[MaxDepth][2];
            Move pvMove;
            Move bestMove;
            Move iidMoves[MaxDepth];

            control();

            template<Alliance> void updateHistory(int,int,int);
            template<Alliance> void raiseHistory(int,int,int);
            template<Alliance> void removeHistory(int,int,int);
            void clearHistory();
            void ageHistory();
            template<Alliance> uint64_t getHistory(int, int);
            void addKiller(int, Move);
            bool isKiller(int, Move);
        };

        /**
         * <summary>
         *  <p><br/>
         * A MoveList is a list of legal moves. A
         * MoveList generates and sorts its moves upon
         * instantiation.
         *  </p>
         *  <p>
         *  The search type must be one of the following:
         *   <ul>
         *    <li>
         *     <b><i>AB</i></b>
         *     <p>
         *  This type will generate all moves sorted
         *  according to the following scheme.
         *      <ol>
         *       <li>PV move</li>
         *       <li>MVV-LVA attacks</li>
         *       <li>Killer quiets</li>
         *       <li>History quiets</li>
         *      </ol>
         *     </p>
         *    </li>
         *    <li>
         *     <b><i>Q</i></b>
         *     <p>
         *  This type will generate attack moves only,
         *  Sorted by MVV-LVA.
         *     </p>
         *    </li>
         *    <li>
         *     <b><i>MCTS</i></b>
         *     <p>
         *  This type will generate all moves without
         *  any sorting whatsoever.
         *     </p>
         *    </li>
         *   </ul>
         *  </p>
         * </summary> 
         * 
         * @tparam ST We want to generate moves differently
         * in different contexts. This template param allows
         * us to provide the context when creating a MoveList.
         */
        template<SearchType ST>
        class MoveList final {
        private:

            /**
             * The backing array for the
             * move list. Moves are small
             * and the MoveList is intended
             * to be stack-allocated. A
             * capacity of 256 gives us some 
             * breathing room without too much
             * waste
             */
            Move m[256];

            /**
             * The size of this MoveList.
             */
            uint16_t size;
        public:

            /**
             * A public constructor for a MoveList.
             */
            explicit MoveList(Board*, control*, int);
            explicit MoveList(Board*);

            /**
             * A method to expose the pointer to the
             * base of the MoveList.
             * 
             * @return a pointer to the base of the
             * MoveList
             */
            constexpr Move* begin()
            { return m; }

            /**
             * A method to expose the pointer to the
             * end of the MoveList (the index just 
             * after the last element in the array).
             * 
             * @return a pointer to the end of the
             * MoveList
             */
            constexpr Move* end()
            { return m + size; }

            /**
             * A method to expose the length of the
             * MoveList.
             * 
             * @return the length (size) of the move
             * list.
             */
            constexpr uint32_t length()
            { return size; }
        };
    }
}


#endif //HOMURA_MOVEMAKE_H

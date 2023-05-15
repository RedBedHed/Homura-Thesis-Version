//
// Created by evcmo on 12/29/2021.
//
#pragma once
#ifndef HOMURA_ZOBRIST_H
#define HOMURA_ZOBRIST_H

#include <ostream>
#include <cassert>
#include "ChaosMagic.h"
#include "Move.h"
#include "Utility.h"

namespace Homura {

    /**
     * Sebastiano Vigna's PRNG
     * (From Stockfish)
     */
    template <uint64_t T>
    class RandGen final {

        uint64_t x;
    public:

        explicit constexpr
        RandGen() :
        x(T == 0? time(nullptr): T)
        {  }

        constexpr uint64_t rand() { 
            x ^= x >> 12,
            x ^= x << 25,
            x ^= x >> 27;
            return
            x * 2685821657736338717LL;
        }
    };

    namespace Zobrist {
        enum EntryType : uint8_t 
        { lower, exact, upper, undef };
        
        struct Entry final {
            uint64_t key;
            int64_t value;
            int64_t clock;
            EntryType type;   
            Move move;
            uint8_t depth;
        };

        void init();
        void destroy();
        void reset();
        template<MoveType> uint64_t get(int);
        template<Alliance, PieceType> uint64_t get(int);
        template<Alliance> uint64_t side();
        uint64_t side(Alliance);
        template<Alliance> uint64_t get(PieceType, int);
        void clearTrans();
        void store(uint64_t, int64_t, EntryType, uint8_t, Move, int64_t);
        Entry* retrieve(uint64_t, int64_t);
        int64_t adjustForMate(int64_t score, int ply);
    }
}

#endif //HOMURA_ZOBRIST_H

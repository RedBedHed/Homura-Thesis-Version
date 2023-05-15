//
// Created by evcmo on 12/29/2021.
//

#include "Zobrist.h"

namespace Homura::Zobrist {
    namespace {

        uint64_t BySquare[64][12];
        uint64_t ByEnPassant[64];
        uint64_t CastlingRights[16];
        uint64_t BlackToMove;
        uint64_t WhiteToMove;

        constexpr int tt_size = 1000001;

        Entry* transTable;

        inline Entry* storage
            (
            uint64_t key, 
            uint8_t depth, 
            int64_t clock
            ) 
        {
            const uint64_t slot = (key % tt_size);
            Entry *e1 = transTable + slot;

            if (e1->key == key)        return e1;

            Entry *e2 = transTable + (slot ^ 1U);

            if (e2->key == key)        return e2;

            if (e1->depth < e2->depth) return e1;

            // An idea inspired by Leorik.
            // If the new depth is greater
            // than e1's depth or if e1 is 
            // old (or some combination), 
            // we want to replace the 
            // entry. However, if e2 is
            // getting very old, we
            // should probably replace it.
            // We've already confirmed that
            // it is at most as deep as e1.
            int age1 = clock - e1->clock, age2 = clock - e2->clock;
            if ((depth + (age1 >> 1U)) > (e1->depth + (age2 >> 2U)))
                return e1;
            return e2;
        }

        inline void initRandoms() 
        {
            RandGen<0> r;
            for (int sq = H1; sq <= A8; ++sq) {
                for(int p = 0; p < 12; ++p)
                    BySquare[sq][p] = r.rand();
                ByEnPassant[sq] = r.rand();
            }
            for(int i = 0; i < 16; ++i)
                CastlingRights[i] = r.rand();
            WhiteToMove = r.rand();
            BlackToMove = r.rand();
        }
    }

    void store
        (
        uint64_t key, 
        int64_t value, 
        EntryType type, 
        uint8_t depth,
        Move move,
        int64_t clock
        ) 
    {
        Entry* e = storage(key, depth, clock);
        e->value = value < -MinMate? -MateValue:
                   value >  MinMate?  MateValue:
                   value;
        e->key  =  key; e->type  =  type; e->depth = depth;
        e->move = move; e->clock = clock;
    }

    Entry* retrieve
        (
        uint64_t index, 
        int64_t clock
        ) 
    {
        const uint64_t slot = (index % tt_size);
        Entry* e = transTable + slot;

        if(e->key == index) {
            e->clock = clock; return e;
        }
        
        e = transTable + (slot ^ 1U);

        if(e->key == index) {
            e->clock = clock; return e;
        }
        
        return nullptr;
    }

    void clearTrans() 
    {
        Entry* k = transTable, 
        *const e = transTable + tt_size;
        while(k < e) 
            *k++ = {0, 0, 0, undef, NullMove, uint8_t(-1)};
    }

    void init() 
    {
        initRandoms();
        transTable = new Entry[tt_size]; 
        clearTrans();
    }
    
    void destroy() { delete[] transTable; }

    void reset() { clearTrans(); }

    template <>
    uint64_t get<EnPassant>(const int sq)
    { return ByEnPassant[sq]; }

    template <>
    uint64_t get<Castling>(const int cr)
    { return CastlingRights[cr]; }

    template<Alliance A, PieceType PT>
    uint64_t get(const int sq)
    { return BySquare[sq][(A * 6) + PT]; }

    template uint64_t get<White, Pawn>(int);
    template uint64_t get<White, Rook>(int);
    template uint64_t get<White, Knight>(int);
    template uint64_t get<White, Bishop>(int);
    template uint64_t get<White, Queen>(int);
    template uint64_t get<White, King>(int);
    template uint64_t get<Black, Pawn>(int);
    template uint64_t get<Black, Rook>(int);
    template uint64_t get<Black, Knight>(int);
    template uint64_t get<Black, Bishop>(int);
    template uint64_t get<Black, Queen>(int);
    template uint64_t get<Black, King>(int);

    template<Alliance A>
    uint64_t side()
    { return A == White? WhiteToMove: BlackToMove; }

    template uint64_t side<White>();
    template uint64_t side<Black>();

    uint64_t side(Alliance a)
    { return ((int) ~a) * WhiteToMove + ((int) a) * BlackToMove; }

    template<Alliance A>
    uint64_t get(const PieceType pt, const int sq)
    { assert(pt != NullPT); return BySquare[sq][(A * 6) + pt]; }

    template uint64_t get<White>(PieceType, int);
    template uint64_t get<Black>(PieceType, int);
}

//
// Created by evcmo on 6/2/2021.
//

#pragma once
#ifndef HOMURA_CHAOSMAGIC_H
#define HOMURA_CHAOSMAGIC_H

#include <cstdint>

// Check to see if we are compiling for a
// 64-bit cpu.
#if INTPTR_MAX == INT32_MAX
#   error "CPU architecture not supported."
#endif

// BMI2 - has pext
#define USE_BMI2

// Popcnt
#define USE_POPCNT

// If this is a Microsoft compiler,
// include the Microsoft intrinsic library.
#if defined(_MSC_VER)
#   include <intrin.h>
#endif

// If this BMI2 is supported, set the
// HasBMI flag and include the BMI2
// intrinsic library.
// Define HASH and PEXT macros.
#if defined(USE_BMI2)
#   include <immintrin.h>
// Magic hash.
#   define HASH(bb, m, mn, sa) \
    _pext_u64(bb, m)
#else
    // Magic hash.
#   define HASH(bb, m, mn, sa) \
    (int) (((bb & m) * mn) >> sa)
#endif

#include <iostream>
#include <memory>
#include <cassert>
#include <string>
#include <mutex>
#include "Paths.h"
#include "Rays.h"
#include "Utility.h"

namespace Homura {

    /** The alliances, enumerated. */
    enum Alliance : uint8_t
    { White, Black };

    /** The castle types, enumerated. */
    enum CastleType : uint8_t
    { KingSide, QueenSide };

    /** The castling rights, enumerated. */
    enum CastlingRight : uint8_t
    { WhiteKingSide, WhiteQueenSide, BlackKingSide, BlackQueenSide };

    /** The check types, enumerated. */
    enum CheckType : uint8_t
    { None, Check, DoubleCheck };

    /** The piece types, enumerated. */
    enum PieceType : uint8_t
    { Pawn, Rook, Knight, Bishop, Queen, King, NullPT };

    /** The move types, enumerated. */
    enum MoveType : uint8_t
    { FreeForm, EnPassant, Castling, PawnJump };

    /** The filter types, enumerated. */
    enum FilterType : uint8_t
    { Aggressive, Passive, All };

    /** The search types, enumerated. */
    enum SearchType : uint8_t
    { MCTS, AB, Q };

    /** A table to convert a move type to a string. */
    constexpr const char* MoveTypeToString[] =
    { "FreeForm", "EnPassant", "CastlingRights", "PawnJump" };

    /** A table to convert a piece type to a string. */
    constexpr const char* PieceTypeToString[] =
    { "Pawn", "Rook", "Knight", "Bishop", "Queen", "King", "NullPT" };

    /** The board squares, enumerated */
    enum Square : uint8_t
    {
        H1, G1, F1, E1, D1, C1, B1, A1,
        H2, G2, F2, E2, D2, C2, B2, A2,
        H3, G3, F3, E3, D3, C3, B3, A3,
        H4, G4, F4, E4, D4, C4, B4, A4,
        H5, G5, F5, E5, D5, C5, B5, A5,
        H6, G6, F6, E6, D6, C6, B6, A6,
        H7, G7, F7, E7, D7, C7, B7, A7,
        H8, G8, F8, E8, D8, C8, B8, A8,
        NullSQ
    };

    /** String representations of each board square. */
    constexpr const char* SquareToString[] = {
        "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
        "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
        "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
        "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
        "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
        "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
        "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
        "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8",
        "NullSQ"
    };

    /** The board directions, enumerated. */
    enum Direction : int8_t
    {
        North =  8,
        South = -North,
        East  =  1,
        West  = -East,
        NorthEast = North + East,
        NorthWest = North + West,
        SouthEast = South + East,
        SouthWest = South + West
    };

    /**
     * An operator overload to apply the not operator
     * to an Alliance.
     *
     * @param a the alliance to negate
     * @return the complement of the given alliance,
     * if { White, Black } is the universal set
     */
    constexpr Alliance operator~(const Alliance& a)
    { return Alliance(a ^ Black ^ White); }

    /**
     * <summary>
     *  <p><br/>
     * A fancy magic entry follows the fancy magic
     * bitboard scheme discussed in length at
     *   <a href=
     * "https://www.chessprogramming.org/Magic_Bitboards">
     * chessprogramming.org
     *   </a>
     *  </p>
     *  <p>
     * A fancy magic entry holds a table of attack boards
     * associated with blocker boards for a particular
     * square, allowing the client to map blocker boards
     * directly to move boards with an internal hashing
     * function.
     *  </p>
     * </summary>
     * @class FancyMagic
     * @author Ellie Moore
     * @version 05.30.2021
     */
    class FancyMagic final {
    private:

        /**
         * @private
         * A pointer to a list to hold all attackBoards
         * for this FancyMagic.
         */
        const uint64_t* const attackBoards;

#       ifndef USE_BMI2
        /**
         * @private
         * A shift amount to use in attack lookup
         * to ensure the maximum hash key is within
         * the bounds of the attackBoards array.
         */
        const int shiftAmount;

        /**
         * @private
         * A magic number to map every possible
         * occupancy to the correct attack board
         * in the attackBoards database.
         */
        const uint64_t magicNumber;
#       endif

        /**
         * @private
         * A mask to derive a blocker board from
         * the current game board.
         */
        const uint64_t mask;

    public:

        /**
         * @public
         * A deleted copy constructor for
         * FancyMagic.
         */
        FancyMagic(const FancyMagic&) = delete;

        /**
         * @public
         * A deleted move constructor for
         * FancyMagic.
         */
        FancyMagic(FancyMagic&&) = delete;

        /**
         * @public
         * A public destructor for FancyMagic.
         */
        ~FancyMagic() = default;

        /**
         * A method to lookup the attack board
         * associated with the given game board.
         * @copydoc FancyMagic::getAttacks()
         * @param blockerBoard the blocker board for
         * which to retrieve the attack board
         * @return the attack board corresponding to
         * the given blocker board
         */
        [[nodiscard]]
        inline uint64_t
        getAttacks(const uint64_t blockerBoard) const {
            return attackBoards[HASH(
                    blockerBoard, mask,
                    magicNumber, shiftAmount
            )];
        }

        /**
         * <summary>
         *  <p><br/>
         * A builder pattern for the FancyMagic class
         * to allow the client flexibility and readability
         * during the instantiation of a FancyMagic Object.
         *  </p>
         * </summary>
         * @class Builder
         * @author Ellie Moore
         * @version 06.07.2021
         */
        class Builder final {
        private:

            /**
             * The attackBoards array for this builder.
             */
            uint64_t* const attacks;

            /**
             * The shift amount for this builder.
             */
            int shiftAmount;

            /**
             * The magic number for this builder.
             */
            uint64_t magicNumber;

            /**
             * The blocker mask for this builder.
             */
            uint64_t mask;

            /**
             * Fancy magic is Builder'x private
             * bff.
             */
            friend class FancyMagic;
        public:

            /**
             * @public
             * A public constructor for Builder.
             */
            explicit Builder(uint64_t*);

            /**
             * @public
             * A public copy constructor for Builder.
             */
            Builder(const Builder&) = default;

            /**
             * @public
             * A public move constructor for Builder.
             */
            Builder(Builder&&) = default;

            /**
             * A method to set the magic number
             * for this Builder.
             */
            Builder& setMagicNumber(uint64_t);

            /**
             * A method to set the blocker mask
             * for this Builder.
             */
            Builder& setMask(uint64_t);

            /**
             * A method to set the shift amount
             * for this Builder.
             */
            Builder& setShiftAmount(int);

            /**
             * A method to place an attack in this
             * builder'x attack array at a location
             * determined by the FancyMagic hash
             * method.
             */
            Builder& placeAttacks(uint64_t, uint64_t);

            /**
             * A method to instantiate an immutable
             * FancyMagic from the data stored in this
             * Builder.
             */
            FancyMagic* build();

            /**
             * @public
             * A public destructor for Builder.
             */
            ~Builder() = default;
        };
    private:

        /**
         * @private
         * A private constructor for FancyMagic.
         */
        explicit FancyMagic(const Builder&);
    };

    namespace Witchcraft {

        /** CastlingRights destination squares, as shorts. */
        constexpr short WhiteKingsideRookOrigin       = H1;
        constexpr short WhiteKingsideRookDestination  = F1;
        constexpr short WhiteKingsideKingDestination  = G1;
        constexpr short WhiteQueensideRookOrigin      = A1;
        constexpr short WhiteQueensideRookDestination = D1;
        constexpr short WhiteQueensideKingDestination = C1;
        constexpr short BlackKingsideRookOrigin       = H8;
        constexpr short BlackKingsideRookDestination  = F8;
        constexpr short BlackKingsideKingDestination  = G8;
        constexpr short BlackQueensideRookOrigin      = A8;
        constexpr short BlackQueensideRookDestination = D8;
        constexpr short BlackQueensideKingDestination = C8;

        /**
         * Starting positions and useful masks.
         */
        constexpr uint64_t BlackPawnsStartPosition
                = 0x00FF000000000000L;
        constexpr uint64_t BlackRooksStartPosition
                = 0x8100000000000000L;
        constexpr uint64_t BlackKnightsStartPosition
                = 0x4200000000000000L;
        constexpr uint64_t BlackBishopsStartPosition
                = 0x2400000000000000L;
        constexpr uint64_t BlackQueenStartPosition
                = 0x1000000000000000L;
        constexpr uint64_t BlackKingStartPosition
                = 0x0800000000000000L;
        constexpr uint64_t WhitePawnsStartPosition
                = 0x000000000000FF00L;
        constexpr uint64_t WhiteRooksStartPosition
                = 0x0000000000000081L;
        constexpr uint64_t WhiteKnightsStartPosition
                = 0x0000000000000042L;
        constexpr uint64_t WhiteBishopsStartPosition
                = 0x0000000000000024L;
        constexpr uint64_t WhiteQueenStartPosition
                = 0x0000000000000010L;
        constexpr uint64_t WhiteKingStartPosition
                = 0x0000000000000008L;
        constexpr uint64_t BlackPawnJumpSquares
                = 0x0000FF0000000000L;
        constexpr uint64_t WhitePawnJumpSquares
                = 0x0000000000FF0000L;
        constexpr uint64_t NotEastFile
                = 0x7F7F7F7F7F7F7F7FL;
        constexpr uint64_t NotWestFile
                = 0xFEFEFEFEFEFEFEFEL;
        constexpr uint64_t NotEdges
                = 0x007E7E7E7E7E7E00L;
        constexpr uint64_t NotEdgeFiles
                = 0x7E7E7E7E7E7E7E7EL;
        constexpr uint64_t NotEdgeRanks
                = 0x00FFFFFFFFFFFF00L;
        constexpr uint64_t FullBoard
                = 0xFFFFFFFFFFFFFFFFL;
        constexpr uint64_t WhiteQueensideMask
                = 0x0000000000000070L;
        constexpr uint64_t BlackQueensideMask
                = 0x7000000000000000L;
        constexpr uint64_t WhiteKingsideMask
                = 0x0000000000000006L;
        constexpr uint64_t BlackKingsideMask
                = 0x0600000000000000L;
        constexpr uint64_t WhiteQueensidePath
                = 0x0000000000000030L;
        constexpr uint64_t BlackQueensidePath
                = 0x3000000000000000L;
        constexpr uint64_t WhiteKingsidePath
                = 0x0000000000000006L;
        constexpr uint64_t BlackKingsidePath
                = 0x0600000000000000L;
        constexpr uint64_t WhiteQueensideRookMask
                = 0x0000000000000090L;
        constexpr uint64_t BlackQueensideRookMask
                = 0x9000000000000000L;
        constexpr uint64_t WhiteKingsideRookMask
                = 0x0000000000000005L;
        constexpr uint64_t BlackKingsideRookMask
                = 0x0500000000000000L;
        constexpr uint64_t WhiteEnPassantRank
                = 0x000000FF00000000L;
        constexpr uint64_t BlackEnPassantRank
                = 0x00000000FF000000L;
        constexpr uint64_t WhitePrePromotionMask
                = 0x00FF000000000000L;
        constexpr uint64_t BlackPrePromotionMask
                = 0x000000000000FF00L;
        constexpr uint64_t LightSquares
                = 0x55AA55AA55AA55AAL;
        constexpr uint64_t DarkSquares
                = 0xAA55AA55AA55AA55L;

        /** A map from squares to bit boards. */
        constexpr uint64_t SquareToBitBoard[] = {
                0x0000000000000001L, 0x0000000000000002L,
                0x0000000000000004L, 0x0000000000000008L,
                0x0000000000000010L, 0x0000000000000020L,
                0x0000000000000040L, 0x0000000000000080L,
                0x0000000000000100L, 0x0000000000000200L,
                0x0000000000000400L, 0x0000000000000800L,
                0x0000000000001000L, 0x0000000000002000L,
                0x0000000000004000L, 0x0000000000008000L,
                0x0000000000010000L, 0x0000000000020000L,
                0x0000000000040000L, 0x0000000000080000L,
                0x0000000000100000L, 0x0000000000200000L,
                0x0000000000400000L, 0x0000000000800000L,
                0x0000000001000000L, 0x0000000002000000L,
                0x0000000004000000L, 0x0000000008000000L,
                0x0000000010000000L, 0x0000000020000000L,
                0x0000000040000000L, 0x0000000080000000L,
                0x0000000100000000L, 0x0000000200000000L,
                0x0000000400000000L, 0x0000000800000000L,
                0x0000001000000000L, 0x0000002000000000L,
                0x0000004000000000L, 0x0000008000000000L,
                0x0000010000000000L, 0x0000020000000000L,
                0x0000040000000000L, 0x0000080000000000L,
                0x0000100000000000L, 0x0000200000000000L,
                0x0000400000000000L, 0x0000800000000000L,
                0x0001000000000000L, 0x0002000000000000L,
                0x0004000000000000L, 0x0008000000000000L,
                0x0010000000000000L, 0x0020000000000000L,
                0x0040000000000000L, 0x0080000000000000L,
                0x0100000000000000L, 0x0200000000000000L,
                0x0400000000000000L, 0x0800000000000000L,
                0x1000000000000000L, 0x2000000000000000L,
                0x4000000000000000L, 0x8000000000000000L
        };

        /**
         * Directions for the bishop in an iterable
         * format.
         */
        constexpr Direction BishopDirections[] =
        { NorthWest, SouthWest, SouthEast, NorthEast };

        /**
         * Directions for the rook in an iterable
         * format.
         */
        constexpr Direction RookDirections[] =
        { North, West, South, East };

        /**
         * The eight files of the board.
         */
        constexpr uint64_t Files[] = {
                0x0101010101010101L, 0x0202020202020202L,
                0x0404040404040404L, 0x0808080808080808L,
                0x1010101010101010L, 0x2020202020202020L,
                0x4040404040404040L, 0x8080808080808080L
        };

        /**
         * The eight ranks of the board.
         */
        constexpr uint64_t Ranks[] = {
                0x00000000000000FFL, 0x000000000000FF00L,
                0x0000000000FF0000L, 0x00000000FF000000L,
                0x000000FF00000000L, 0x0000FF0000000000L,
                0x00FF000000000000L, 0xFF00000000000000L
        };

        /**
         * The DeBruijn constant.
         */
        constexpr uint64_t DeBruijn64 = 0x03F79D71B4CB0A89L;

        /**
         * The DeBruijn map from hash key to integer
         * square index.
         */
        constexpr uint8_t DeBruijnTable[] = {
                0,   1, 48,  2, 57, 49, 28,  3,
                61, 58, 50, 42, 38, 29, 17,  4,
                62, 55, 59, 36, 53, 51, 43, 22,
                45, 39, 33, 30, 24, 18, 12,  5,
                63, 47, 56, 27, 60, 41, 37, 16,
                54, 35, 52, 21, 44, 32, 23, 11,
                46, 26, 40, 15, 34, 20, 31, 10,
                25, 14, 19,  9, 13,  8,  7,  6
        };

        /**
         * The sizes of individual "Fancy Magic" attack pieceSquareTables
         * for a rook on the associated square.
         */
        constexpr short int FancyRookSizes[] = {
                4096, 2048, 2048, 2048, 2048, 2048, 2048, 4096,
                2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
                2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
                2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
                2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
                2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
                2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
                4096, 2048, 2048, 2048, 2048, 2048, 2048, 4096
        };

        /**
         * The sizes of individual "Fancy Magic" attack pieceSquareTables
         * for a bishop on the associated square.
         */
        constexpr short int FancyBishopSizes[] = {
                64, 32,  32,  32,  32,  32, 32, 64,
                32, 32,  32,  32,  32,  32, 32, 32,
                32, 32, 128, 128, 128, 128, 32, 32,
                32, 32, 128, 512, 512, 128, 32, 32,
                32, 32, 128, 512, 512, 128, 32, 32,
                32, 32, 128, 128, 128, 128, 32, 32,
                32, 32,  32,  32,  32,  32, 32, 32,
                64, 32,  32,  32,  32,  32, 32, 64
        };

        /**
         * <summary>
         *  <p>
         * A method to initialize the full Witchcraft
         * namespace, particularly the hidden attack
         * databases. This method initializes on-the-fly
         * as the attack databases are far too substantial
         * to be printed and, consequently, cannot be
         * known at compile time. This method is intended
         * to be invoked only once, at the beginning of
         * main. However, it may be invoked following any
         * successful call to destroy.
         *  </p>
         * </summary>
         *
         * @warning
         * <b>
         * Calling this method twice in a row will trigger
         * an assertion failure.
         * </b>
         *
         * @see Witchcraft::destroy()
         */
        void init();

        /**
         * <summary>
         *  <p>
         * A method to tear down the witchcraft namespace,
         * particularly the magic attack databases. This
         * method is intended to be invoked only once, at
         * the end of main. However, it may be invoked
         * following any successful call to init.
         *  </p>
         * </summary>
         *
         * @warning
         * <b>
         * Calling this method twice in a row will trigger
         * an assertion failure.
         * </b>
         *
         * @see Witchcraft::init()
         */
        void destroy();

        /**
         * A method to print a bit board to the console, for
         * debugging purposes.
         */
        void bb(uint64_t);

        /**
         * A method to return an attack bitboard for
         * the given square and the given piece type,
         * according to the layout of the given bitboard.
         *
         * @param board the bitboard representing all
         * pieces on the current game board
         * @param sq the square of the rook to move
         * @return an attack bitboard for the given square
         * and piece type according to the layout of the
         * given bitboard
         */
        template <PieceType> uint64_t attackBoard(uint64_t, int);
        template <PieceType> uint64_t attackBoard(int);
        template<Alliance, PieceType> uint64_t attackBoard(int);

        /**
         * A method to shift the given bitboard left or
         * right by the absolute value of the direction,
         * according to its sign.
         *
         * @param b The board to shift
         * @tparam D The direction and amount to shift with
         */
        template<Direction D>
        constexpr uint64_t shift(const uint64_t b) {
            static_assert(
                    D == North     || D == South     ||
                    D == East      || D == West      ||
                    D == NorthEast || D == NorthWest ||
                    D == SouthEast || D == SouthWest
            );
            return D > 0 ? b << D : b >> -D;
        }

        /**
         * A method to getPieces the file of the current square
         * (square mod 8).
         *
         * @param square the square to calculate the file of
         * @return the file of the given square
         */
        constexpr int fileOf(unsigned int square)
        { return (int)(square & 7U); }

        /**
         * A method to getPieces the rank of the current square
         * (square div 8).
         *
         * @param the square to calculate the rank of
         * @return the rank of the given square
         */
        constexpr int rankOf(unsigned int square)
        { return (int)(square >> 3U); }

        /**
         * A method to check if a king move in the given
         * direction is within the boundaries of the board.
         *
         * @param origin the origin of the move
         * @param direction the direction in which to move
         * @return whether or not a move in the specified
         * direction will be within the bounds of the board.
         */
        constexpr bool withinBounds(const int origin,
                                    const int direction) {
            const int x = origin + direction;
            return x >= H1 && x <= A8 && (
                abs(fileOf(x) - fileOf(origin)) < 2 ||
                abs(direction) == North
            );
        }

        /**
         * A method to compute the absolute value of an
         * integer, without branching.
         *
         * @param x the number to take the absolute value of
         */
        constexpr int abs(const int x) {
            return x - (signed int)
                (((unsigned int)x << 1U) &
                ((unsigned int)x >> 31U));
        }

        /**
         * A method to count the high bits in the given unsigned
         * long using an algorithm featured in Hacker'x Delight.
         * This algorithm is based on the mathematical formula
         * high bit count = x - (SUM from root=0 to root=inf (x >> root))
         *
         * @param x the long which contains the bits to count
         * @return the number of high bits in the given ulong
         */
        constexpr int highBitCount(uint64_t x) {
#       ifndef USE_POPCNT
            // Count bits in each 4-bit section.
            uint64_t root =
                (x >> 1U) & 0x7777777777777777UL;
            x -= root;
            root = (root >> 1U) & 0x7777777777777777UL;
            x -= root;
            root = (root >> 1U) & 0x7777777777777777UL;
            x -= root;
            // add 4-bit sums into byte sums.
            x = (x + (x >> 4U)) & 0x0F0F0F0F0F0F0F0FUL;
            // Add the byte sums.
            x *= 0x0101010101010101UL;
            return (short)(x >> 56U);
#       elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
            return (int)_mm_popcnt_u64(x);
#       else
            return __builtin_popcountll(x);
#       endif
        }

        /**
         * A method to "scan" the given unsigned long
         * from least significant bit to most significant
         * bit, reporting the index of the fist encountered
         * high bit.
         *
         * @author Martin Läuter (1997),
         * @author Charles E. Leiserson,
         * @author Harald Prokop,
         * @author Keith H. Randall
         * @param l the long to scan
         * @return the integer index of the first high bit
         * starting from the least significant side.
         */
        inline int bitScanFwd(const uint64_t l) {
            assert(l != 0);
#       if defined(__GNUC__)
            return __builtin_ctzll(l);
#       elif defined(_MSC_VER)
            unsigned long r;
            _BitScanForward64(&r, l);
            return (int) r;
#       else
            return DeBruijnTable[(int)
                (((l & (uint64_t)-(int64_t)l) * DeBruijn64) >> 58U)
            ];
#       endif
        }

        /**
         * A mapping of indices to the west-to-east diagonals
         * which they occupy
         */
        constexpr uint64_t WestToEastDiagonals[] = {
                0x8040201008040201L, 0x0080402010080402L,
                0x0000804020100804L, 0x0000008040201008L,
                0x0000000080402010L, 0x0000000000804020L,
                0x0000000000008040L, 0x0000000000000080L,
                0x4020100804020100L, 0x8040201008040201L,
                0x0080402010080402L, 0x0000804020100804L,
                0x0000008040201008L, 0x0000000080402010L,
                0x0000000000804020L, 0x0000000000008040L,
                0x2010080402010000L, 0x4020100804020100L,
                0x8040201008040201L, 0x0080402010080402L,
                0x0000804020100804L, 0x0000008040201008L,
                0x0000000080402010L, 0x0000000000804020L,
                0x1008040201000000L, 0x2010080402010000L,
                0x4020100804020100L, 0x8040201008040201L,
                0x0080402010080402L, 0x0000804020100804L,
                0x0000008040201008L, 0x0000000080402010L,
                0x0804020100000000L, 0x1008040201000000L,
                0x2010080402010000L, 0x4020100804020100L,
                0x8040201008040201L, 0x0080402010080402L,
                0x0000804020100804L, 0x0000008040201008L,
                0x0402010000000000L, 0x0804020100000000L,
                0x1008040201000000L, 0x2010080402010000L,
                0x4020100804020100L, 0x8040201008040201L,
                0x0080402010080402L, 0x0000804020100804L,
                0x0201000000000000L, 0x0402010000000000L,
                0x0804020100000000L, 0x1008040201000000L,
                0x2010080402010000L, 0x4020100804020100L,
                0x8040201008040201L, 0x0080402010080402L,
                0x0100000000000000L, 0x0201000000000000L,
                0x0402010000000000L, 0x0804020100000000L,
                0x1008040201000000L, 0x2010080402010000L,
                0x4020100804020100L, 0x8040201008040201L
        };

        /**
         * A mapping of indices to the east-to-west diagonals
         * which they occupy
         */
        constexpr uint64_t EastToWestDiagonals[] = {
                0x0000000000000001L, 0x0000000000000102L,
                0x0000000000010204L, 0x0000000001020408L,
                0x0000000102040810L, 0x0000010204081020L,
                0x0001020408102040L, 0x0102040810204080L,
                0x0000000000000102L, 0x0000000000010204L,
                0x0000000001020408L, 0x0000000102040810L,
                0x0000010204081020L, 0x0001020408102040L,
                0x0102040810204080L, 0x0204081020408000L,
                0x0000000000010204L, 0x0000000001020408L,
                0x0000000102040810L, 0x0000010204081020L,
                0x0001020408102040L, 0x0102040810204080L,
                0x0204081020408000L, 0x0408102040800000L,
                0x0000000001020408L, 0x0000000102040810L,
                0x0000010204081020L, 0x0001020408102040L,
                0x0102040810204080L, 0x0204081020408000L,
                0x0408102040800000L, 0x0810204080000000L,
                0x0000000102040810L, 0x0000010204081020L,
                0x0001020408102040L, 0x0102040810204080L,
                0x0204081020408000L, 0x0408102040800000L,
                0x0810204080000000L, 0x1020408000000000L,
                0x0000010204081020L, 0x0001020408102040L,
                0x0102040810204080L, 0x0204081020408000L,
                0x0408102040800000L, 0x0810204080000000L,
                0x1020408000000000L, 0x2040800000000000L,
                0x0001020408102040L, 0x0102040810204080L,
                0x0204081020408000L, 0x0408102040800000L,
                0x0810204080000000L, 0x1020408000000000L,
                0x2040800000000000L, 0x4080000000000000L,
                0x0102040810204080L, 0x0204081020408000L,
                0x0408102040800000L, 0x0810204080000000L,
                0x1020408000000000L, 0x2040800000000000L,
                0x4080000000000000L, 0x8000000000000000L
        };

        /**
         * An immutable map from integer position to a
         * knight move board with high bits representing
         * destinations.
         */
        constexpr uint64_t SquareToKnightAttacks[] = {
                0x0000000000020400L, 0x0000000000050800L,
                0x00000000000A1100L, 0x0000000000142200L,
                0x0000000000284400L, 0x0000000000508800L,
                0x0000000000A01000L, 0x0000000000402000L,
                0x0000000002040004L, 0x0000000005080008L,
                0x000000000A110011L, 0x0000000014220022L,
                0x0000000028440044L, 0x0000000050880088L,
                0x00000000A0100010L, 0x0000000040200020L,
                0x0000000204000402L, 0x0000000508000805L,
                0x0000000A1100110AL, 0x0000001422002214L,
                0x0000002844004428L, 0x0000005088008850L,
                0x000000A0100010A0L, 0x0000004020002040L,
                0x0000020400040200L, 0x0000050800080500L,
                0x00000A1100110A00L, 0x0000142200221400L,
                0x0000284400442800L, 0x0000508800885000L,
                0x0000A0100010A000L, 0x0000402000204000L,
                0x0002040004020000L, 0x0005080008050000L,
                0x000A1100110A0000L, 0x0014220022140000L,
                0x0028440044280000L, 0x0050880088500000L,
                0x00A0100010A00000L, 0x0040200020400000L,
                0x0204000402000000L, 0x0508000805000000L,
                0x0A1100110A000000L, 0x1422002214000000L,
                0x2844004428000000L, 0x5088008850000000L,
                0xA0100010A0000000L, 0x4020002040000000L,
                0x0400040200000000L, 0x0800080500000000L,
                0x1100110A00000000L, 0x2200221400000000L,
                0x4400442800000000L, 0x8800885000000000L,
                0x100010A000000000L, 0x2000204000000000L,
                0x0004020000000000L, 0x0008050000000000L,
                0x00110A0000000000L, 0x0022140000000000L,
                0x0044280000000000L, 0x0088500000000000L,
                0x0010A00000000000L, 0x0020400000000000L
        };

        /**
         * An immutable map from integer square to a king
         * move board with high bits representing destinations.
         */
        constexpr uint64_t SquareToKingAttacks[] = {
                0x0000000000000302L, 0x0000000000000705L,
                0x0000000000000E0AL, 0x0000000000001C14L,
                0x0000000000003828L, 0x0000000000007050L,
                0x000000000000E0A0L, 0x000000000000C040L,
                0x0000000000030203L, 0x0000000000070507L,
                0x00000000000E0A0EL, 0x00000000001C141CL,
                0x0000000000382838L, 0x0000000000705070L,
                0x0000000000E0A0E0L, 0x0000000000C040C0L,
                0x0000000003020300L, 0x0000000007050700L,
                0x000000000E0A0E00L, 0x000000001C141C00L,
                0x0000000038283800L, 0x0000000070507000L,
                0x00000000E0A0E000L, 0x00000000C040C000L,
                0x0000000302030000L, 0x0000000705070000L,
                0x0000000E0A0E0000L, 0x0000001C141C0000L,
                0x0000003828380000L, 0x0000007050700000L,
                0x000000E0A0E00000L, 0x000000C040C00000L,
                0x0000030203000000L, 0x0000070507000000L,
                0x00000E0A0E000000L, 0x00001C141C000000L,
                0x0000382838000000L, 0x0000705070000000L,
                0x0000E0A0E0000000L, 0x0000C040C0000000L,
                0x0003020300000000L, 0x0007050700000000L,
                0x000E0A0E00000000L, 0x001C141C00000000L,
                0x0038283800000000L, 0x0070507000000000L,
                0x00E0A0E000000000L, 0x00C040C000000000L,
                0x0302030000000000L, 0x0705070000000000L,
                0x0E0A0E0000000000L, 0x1C141C0000000000L,
                0x3828380000000000L, 0x7050700000000000L,
                0xE0A0E00000000000L, 0xC040C00000000000L,
                0x0203000000000000L, 0x0507000000000000L,
                0x0A0E000000000000L, 0x141C000000000000L,
                0x2838000000000000L, 0x5070000000000000L,
                0xA0E0000000000000L, 0x40C0000000000000L
        };

        /**
         * An immutable map from integer square to rook
         * blocker mask.
         */
        constexpr uint64_t SquareToRookBlockerMask[] = {
                0x000101010101017EL, 0x000202020202027CL,
                0x000404040404047AL, 0x0008080808080876L,
                0x001010101010106EL, 0x002020202020205EL,
                0x004040404040403EL, 0x008080808080807EL,
                0x0001010101017E00L, 0x0002020202027C00L,
                0x0004040404047A00L, 0x0008080808087600L,
                0x0010101010106E00L, 0x0020202020205E00L,
                0x0040404040403E00L, 0x0080808080807E00L,
                0x00010101017E0100L, 0x00020202027C0200L,
                0x00040404047A0400L, 0x0008080808760800L,
                0x00101010106E1000L, 0x00202020205E2000L,
                0x00404040403E4000L, 0x00808080807E8000L,
                0x000101017E010100L, 0x000202027C020200L,
                0x000404047A040400L, 0x0008080876080800L,
                0x001010106E101000L, 0x002020205E202000L,
                0x004040403E404000L, 0x008080807E808000L,
                0x0001017E01010100L, 0x0002027C02020200L,
                0x0004047A04040400L, 0x0008087608080800L,
                0x0010106E10101000L, 0x0020205E20202000L,
                0x0040403E40404000L, 0x0080807E80808000L,
                0x00017E0101010100L, 0x00027C0202020200L,
                0x00047A0404040400L, 0x0008760808080800L,
                0x00106E1010101000L, 0x00205E2020202000L,
                0x00403E4040404000L, 0x00807E8080808000L,
                0x007E010101010100L, 0x007C020202020200L,
                0x007A040404040400L, 0x0076080808080800L,
                0x006E101010101000L, 0x005E202020202000L,
                0x003E404040404000L, 0x007E808080808000L,
                0x7E01010101010100L, 0x7C02020202020200L,
                0x7A04040404040400L, 0x7608080808080800L,
                0x6E10101010101000L, 0x5E20202020202000L,
                0x3E40404040404000L, 0x7E80808080808000L
        };

        /**
         * An immutable map from integer square to bishop
         * blocker mask.
         */
        constexpr uint64_t SquareToBishopBlockerMask[] = {
                0x0040201008040200L, 0x0000402010080400L,
                0x0000004020100A00L, 0x0000000040221400L,
                0x0000000002442800L, 0x0000000204085000L,
                0x0000020408102000L, 0x0002040810204000L,
                0x0020100804020000L, 0x0040201008040000L,
                0x00004020100A0000L, 0x0000004022140000L,
                0x0000000244280000L, 0x0000020408500000L,
                0x0002040810200000L, 0x0004081020400000L,
                0x0010080402000200L, 0x0020100804000400L,
                0x004020100A000A00L, 0x0000402214001400L,
                0x0000024428002800L, 0x0002040850005000L,
                0x0004081020002000L, 0x0008102040004000L,
                0x0008040200020400L, 0x0010080400040800L,
                0x0020100A000A1000L, 0x0040221400142200L,
                0x0002442800284400L, 0x0004085000500800L,
                0x0008102000201000L, 0x0010204000402000L,
                0x0004020002040800L, 0x0008040004081000L,
                0x00100A000A102000L, 0x0022140014224000L,
                0x0044280028440200L, 0x0008500050080400L,
                0x0010200020100800L, 0x0020400040201000L,
                0x0002000204081000L, 0x0004000408102000L,
                0x000A000A10204000L, 0x0014001422400000L,
                0x0028002844020000L, 0x0050005008040200L,
                0x0020002010080400L, 0x0040004020100800L,
                0x0000020408102000L, 0x0000040810204000L,
                0x00000A1020400000L, 0x0000142240000000L,
                0x0000284402000000L, 0x0000500804020000L,
                0x0000201008040200L, 0x0000402010080400L,
                0x0002040810204000L, 0x0004081020400000L,
                0x000A102040000000L, 0x0014224000000000L,
                0x0028440200000000L, 0x0050080402000000L,
                0x0020100804020000L, 0x0040201008040200L
        };

        /**
         * A method to return a board containing all squares
         * on the diagonal, horizontal, or vertical path that
         * bridges the given squares. If the two squares cannot
         * be bridged, this method will return 0.
         *
         * @namespace Homura::Witchcraft
         * @param from the origin of the path
         * @param to the destination of the path
         * @return a bitboard containing all squares on the
         * path that bridges the given squares, or zero if
         * nonesuch
         */
        inline uint64_t
        pathBoard(const int from, const int to)
        { return Paths[from][to]; }

        /**
         * A method to return a board containing all squares
         * on the diagonal, horizontal, or vertical ray that
         * intersect the given squares. If the two squares
         * are non-linear, this method will return 0.
         *
         * @namespace Homura::Witchcraft
         * @param from the origin of the path
         * @param to the destination of the path
         * @return a bitboard containing all squares on the
         * ray that intersects the given squares, or zero if
         * nonesuch
         */
        inline uint64_t
        rayBoard(const int from, const int to)
        { return Rays[from][to]; }

        constexpr uint64_t SquareToPawnAttacks[][BoardLength] = {
                {

                        0x0000000000000200L,0x0000000000000500L,
                        0x0000000000000A00L,0x0000000000001400L,
                        0x0000000000002800L,0x0000000000005000L,
                        0x000000000000A000L,0x0000000000004000L,
                        0x0000000000020000L,0x0000000000050000L,
                        0x00000000000A0000L,0x0000000000140000L,
                        0x0000000000280000L,0x0000000000500000L,
                        0x0000000000A00000L,0x0000000000400000L,
                        0x0000000002000000L,0x0000000005000000L,
                        0x000000000A000000L,0x0000000014000000L,
                        0x0000000028000000L,0x0000000050000000L,
                        0x00000000A0000000L,0x0000000040000000L,
                        0x0000000200000000L,0x0000000500000000L,
                        0x0000000A00000000L,0x0000001400000000L,
                        0x0000002800000000L,0x0000005000000000L,
                        0x000000A000000000L,0x0000004000000000L,
                        0x0000020000000000L,0x0000050000000000L,
                        0x00000A0000000000L,0x0000140000000000L,
                        0x0000280000000000L,0x0000500000000000L,
                        0x0000A00000000000L,0x0000400000000000L,
                        0x0002000000000000L,0x0005000000000000L,
                        0x000A000000000000L,0x0014000000000000L,
                        0x0028000000000000L,0x0050000000000000L,
                        0x00A0000000000000L,0x0040000000000000L,
                        0x0200000000000000L,0x0500000000000000L,
                        0x0A00000000000000L,0x1400000000000000L,
                        0x2800000000000000L,0x5000000000000000L,
                        0xA000000000000000L,0x4000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                },
                {
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000000L,0x0000000000000000L,
                        0x0000000000000002L,0x0000000000000005L,
                        0x000000000000000AL,0x0000000000000014L,
                        0x0000000000000028L,0x0000000000000050L,
                        0x00000000000000A0L,0x0000000000000040L,
                        0x0000000000000200L,0x0000000000000500L,
                        0x0000000000000A00L,0x0000000000001400L,
                        0x0000000000002800L,0x0000000000005000L,
                        0x000000000000A000L,0x0000000000004000L,
                        0x0000000000020000L,0x0000000000050000L,
                        0x00000000000A0000L,0x0000000000140000L,
                        0x0000000000280000L,0x0000000000500000L,
                        0x0000000000A00000L,0x0000000000400000L,
                        0x0000000002000000L,0x0000000005000000L,
                        0x000000000A000000L,0x0000000014000000L,
                        0x0000000028000000L,0x0000000050000000L,
                        0x00000000A0000000L,0x0000000040000000L,
                        0x0000000200000000L,0x0000000500000000L,
                        0x0000000A00000000L,0x0000001400000000L,
                        0x0000002800000000L,0x0000005000000000L,
                        0x000000A000000000L,0x0000004000000000L,
                        0x0000020000000000L,0x0000050000000000L,
                        0x00000A0000000000L,0x0000140000000000L,
                        0x0000280000000000L,0x0000500000000000L,
                        0x0000A00000000000L,0x0000400000000000L,
                        0x0002000000000000L,0x0005000000000000L,
                        0x000A000000000000L,0x0014000000000000L,
                        0x0028000000000000L,0x0050000000000000L,
                        0x00A0000000000000L,0x0040000000000000L,
                }
        };
    } // namespace Witchcraft
}

#endif //HOMURA_CHAOSMAGIC_H


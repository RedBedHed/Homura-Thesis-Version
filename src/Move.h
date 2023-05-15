//
// Created by evcmo on 6/6/2021.
//

#pragma once
#ifndef HOMURA_MOVE_H
#define HOMURA_MOVE_H

#include <cstdint>
#include <cassert>
#include <iostream>
#include <ostream>
#include <string>
#include "ChaosMagic.h"

namespace Homura {

    /**
     * <summary>
     *  <p>
     * A Move object represents a chess move in a 16-bit format.
     *  </p>
     *  <p>
     *   <ul>
     *    <li>
     * bits 5-0 : origin square (0-63, decimal)
     *    </li>
     *    <li>
     * bits 11-6: destination square (0-63, decimal)
     *    </li>
     *    <li>
     * bits 13-12 : move type or promotion piece type (0-3, decimal)
     *    </li>
     *    <li>
     * bits 15-14 : promotion flag (0 or 2, decimal)
     *    </li>
     *   </ul>
     *  </p>
     * </summary>
     * @class Move
     * @author Ellie Moore
     * @version 07.31.2021
     */
    class Move final {
    private:

        /** Useful masks. */
        static constexpr uint16_t To         = 0x003FU;
        static constexpr uint16_t From       = 0x0FC0U;
        static constexpr uint16_t Type       = 0x3000U;
        static constexpr uint16_t Promotion  = 0x8000U;
        static constexpr uint16_t PromoPiece = 0x3000U;

        /**
         * A 16-bit, unsigned integer to store the move data.
         */
        uint16_t manifest;

    public:

        /**
         * @public
         * A public constructor for a Move.
         *
         * @param manifest an integer containing all move data.
         */
        constexpr
        Move(const unsigned int manifest) : manifest(manifest)
        { }

        /**
         * @public
         * A public, default constructor for a Move.
         */
        constexpr Move() : manifest(0)
        { }

        /**
         * @public
         * A public copy constructor for a Move.
         *
         * @param move the move to copy
         */
        Move(const Move& move) = default;

        /**
         * @public
         * A public destructor for a Move.
         */
        ~Move() = default;

        /**
         * A method to get an integer that contains all data
         * for this move.
         *
         * @return an integer containing all data
         * for this move.
         */
        [[nodiscard]]
        constexpr int getManifest() const
        { return manifest; }

        /**
         * @public
         * @static
         * A static factory for a move, no promotion.
         *
         * @tparam MT the move type
         * @param from the origin square
         * @param to the destination square
         * @return a new Move
         */
        template<MoveType MT>
        static constexpr Move make(unsigned int from, unsigned int to) {
            static_assert(MT >= FreeForm && MT <= PawnJump);
            return (MT << 12U) + (from << 6U) + to;
        }

        /**
         * @public
         * @static
         * A static factory for a move with a FreeForm
         * move type, no promotion
         *
         * @param from the origin square
         * @param to the destination square
         * @return a new Move
         */
        static constexpr Move
        make(const unsigned int from, const unsigned int to)
        { return ((from << 6U) + to); }

        /**
         * @public
         * @static
         * A static factory for a promotion move.
         *
         * @tparam PT the promotion piece type
         * @param from the origin square
         * @param to the destination square
         * @return a new Move
         */
        template<PieceType PT>
        static constexpr Move
        makePromotion(const unsigned int from, const unsigned int to) {
            static_assert(PT > Pawn && PT < King);
            return 
                Promotion + ((PT - Rook) << 12U) + ((from << 6U) + to);
        }

        /**
         * A method to expose the destination of this move.
         *
         * @return the destination of this move.
         */
        [[nodiscard]]
        constexpr uint32_t destination() const
        { return manifest & To; }

        /**
         * A method to expose the origin of this move.
         *
         * @return the origin of this move.
         */
        [[nodiscard]]
        constexpr uint32_t origin() const
        { return (manifest & From) >> 6U; }

        /**
         * A method to expose the type of this move.
         * The client should determine whether or not the
         * promotion flag is set before calling this method.
         * This can be accomplished with a call to isPromotion()
         *
         * @see Move::isPromotion()
         * @return the type of this move.
         */
        [[nodiscard]]
        constexpr int moveType() const
        { return (manifest & Type) >> 12U; }

        /**
         * A method to expose the promotion piece of this move.
         * The client should determine whether or not the
         * promotion flag is set before calling this method.
         * This can be accomplished with a call to isPromotion()
         *
         * @see Move::isPromotion()
         * @return the promotion piece of this move.
         */
        [[nodiscard]]
        constexpr int promotionPiece() const
        { return ((manifest & PromoPiece) >> 12U) + Rook; }

        /**
         * A method to indicate whether or not this move is a
         * promotion move.
         *
         * @return whether or not this move is a promotion move.
         */
        [[nodiscard]]
        constexpr bool isPromotion() const
        { return (manifest & Promotion) >> 15U; }

        /**
         * An operator overload for the equality operator.
         *
         * @param other the Move to compare against this Move
         * @return whether or not this Move equals the given
         * Move
         */
        constexpr bool operator==(const Move& other) const
        { return manifest == other.manifest; }

        /**
         * An operator overload for the inequality operator.
         *
         * @param other the Move to compare against this Move
         * @return whether or not this Move doesn't equal the
         * given Move
         */
        constexpr bool operator!=(const Move& other) const
        { return manifest != other.manifest; }

        constexpr bool operator*() const
        { return manifest; }

        /**
         * An operator overload for the assignment operator.
         *
         * @param mx the Move to copy into this Move
         * @return a reference to this move
         */
        constexpr Move& operator=(const Move& mx) = default;

        static constexpr char pieceChar[] = {'r', 'n', 'b', 'q'};

        /**
         * An operator overload for the insertion operator
         * between an ostream and a Move.
         *
         * @param out the ostream to hold the Move in string
         * format
         * @param m the Move to be represented in string format
         * @return a reference to the stream, for chaining purposes
         */
        friend std::ostream&
        operator<<(std::ostream& out, const Move& m) {
            out
                << SquareToString[m.origin()]
                << SquareToString[m.destination()];
            if(!m.isPromotion()) return out;
            return out << pieceChar[(m.manifest & PromoPiece) >> 12U];
        }
    };

    /** A null move. */
    constexpr Move NullMove = 0U;
}

#endif //HOMURA_MOVE_H
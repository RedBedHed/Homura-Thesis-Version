//
// Created by evcmo on 6/2/2021.
//
#include "ChaosMagic.h"

namespace Homura {
    namespace Witchcraft {

        // To abbreviate.
        using std::string;
        using std::mutex;
        using std::lock_guard;
        using std::cout;

        namespace {

            /*
             * The following magic numbers were generated using a
             * special pseudo-random long generator by Sebastiano
             * Vigna and a brute-force verification algorithm.
             */
#           ifndef USE_BMI2

            /**
             * A list of empirically determined magic numbers
             * which can be used to hash game boards and look up
             * rook attackBoards for each square on the board.
             * The hash keys generated using these numbers map
             * each blocker board to its corresponding move
             * board with only constructive collisions.
             */
            constexpr uint64_t RookMagicNumbers[] = {
                    0x0A80004000801220L, 0x4140200040001002L,
                    0x0200104200220880L, 0x4180100008008004L,
                    0x0200200200100408L, 0x0200011004020008L,
                    0x0400080100900204L, 0x0580008002407100L,
                    0x0202800084204008L, 0x0001402000401000L,
                    0x3100808010002000L, 0x0019002210000900L,
                    0x3000800800800400L, 0x0301000900020400L,
                    0x100B000421001200L, 0x00208004801B4100L,
                    0x0000888004400020L, 0x6000404010002008L,
                    0x3100808010002000L, 0x0001050020D00028L,
                    0x1040808008000402L, 0x001E008004000280L,
                    0x8380010100040200L, 0x2110020030804401L,
                    0x1240400280208000L, 0x0020200040100040L,
                    0x0000100080200081L, 0x0000100080080080L,
                    0x9010080080800400L, 0x1060040080800200L,
                    0x0800020080800100L, 0x0C010C2200045081L,
                    0x29404000A1800180L, 0x8400400081802001L,
                    0x2102008022004010L, 0x0008100080800800L,
                    0x9010080080800400L, 0x0900800400800200L,
                    0x8010010804001002L, 0x2018004102002084L,
                    0x3780002000444000L, 0x000041201000C000L,
                    0x00C0100020008080L, 0x0001001000210008L,
                    0x1000080004008080L, 0x0002000204008080L,
                    0x0000581081040012L, 0x0400110C42820004L,
                    0x29404000A1800180L, 0x0020200040100040L,
                    0x8004104220820600L, 0x1000080280500280L,
                    0x1000080004008080L, 0x0002000204008080L,
                    0x0120911028020400L, 0x0000028112640200L,
                    0x00008002204A1101L, 0x0004108040010A21L,
                    0x0000E20019118142L, 0x0900201000040901L,
                    0x8002000508209002L, 0x0001000400020801L,
                    0x1800080102209044L, 0x4048240043802106L
            };

            /**
             * A list of empirically determined magic numbers
             * which can be used to hash game boards and look up
             * bishop attackBoards for each square on the board.
             * The hash keys generated using these numbers map
             * each blocker board to its corresponding move
             * board with only constructive collisions.
             */
            constexpr uint64_t BishopMagicNumbers[] = {
                    0x40106000A1160020L, 0x01280101021A0802L,
                    0x01C80A0042104002L, 0x8C02208A0000C404L,
                    0x160405A020820082L, 0xA029300820008024L,
                    0x0602010120110040L, 0x0001008044200440L,
                    0x0210401044110050L, 0x2000020404040044L,
                    0x09020800C10A0800L, 0x5010280481100082L,
                    0xD082020211604040L, 0x0612142208420203L,
                    0x0080084202104028L, 0x0004C04410841000L,
                    0x0010006020322084L, 0x0002088842082222L,
                    0x1014004208081300L, 0x0001028804110080L,
                    0x0004000822083104L, 0x0032400608200412L,
                    0x1080800848245000L, 0x2002900422013008L,
                    0x4205040860200461L, 0x0088080043900100L,
                    0x0005010350040820L, 0x4241080011004300L,
                    0x000900401C004049L, 0x0208160090208C04L,
                    0x8001010022009080L, 0x8000820100884400L,
                    0x4104100440400501L, 0x0048010820100220L,
                    0x0040802080100080L, 0x4040020080080080L,
                    0x8C0A020200440085L, 0x0030008020860202L,
                    0x0002420200040080L, 0x1007242080202208L,
                    0x1112080340040900L, 0x0082082422040482L,
                    0x0202010028020480L, 0x0082802018010904L,
                    0x0010882104008110L, 0x2602208106006102L,
                    0x0488083084014082L, 0x3001742410802040L,
                    0x0881010120218080L, 0x0382004108292000L,
                    0x1000410401040200L, 0x1000200042020044L,
                    0x8000208425040100L, 0x0800081110088800L,
                    0x0220021002009900L, 0x000948110C0B2081L,
                    0x1030820110010500L, 0x0100004042101040L,
                    0x4041408042009000L, 0x2200040A00840402L,
                    0x0020400120602480L, 0x40020420E0020C84L,
                    0x0000312208080880L, 0x48081010008A2A80L
            };
#           endif

            /**
             * <summary>
             *  <p><br/>
             * A method to instantiate a database of immutable
             * "magic entries" for each square. These entries
             * contain the data needed to map a square's blocker
             * boards to corresponding move boards for a given
             * piece type.
             *  </p>
             *
             *  <p>
             * This database is sized according to the "Fancy
             * Magic" scheme suggested by Pradu Kannan.
             *  </p>
             *
             *  <p>
             * A mapping of blocker boards to move boards must
             * be perfect in order to be used in move generation.
             * Mathematically speaking, a mapping from blocker
             * boards to move boards is a surjective function,
             * that is, each element in the co-domain has at
             * least one corresponding element in the domain.
             * Different blocker boards may map to the same move
             * board. Every move board has at least one blocker
             * board associated with it.
             *  </p>
             *
             *  <p>
             * The database initialized by this function maps
             * blocker boards to move boards using a special
             * hashing function with empirically determined
             * "magic numbers" which ensure that only purposeful
             * (memory-saving) collisions may occur.
             *  </p>
             * </summary>
             *
             * @param incantations a pointer to the array that
             * will hold the magics
             * @param attackTable a pointer to the array
             * that will hold the attacks
             * @param directions an iterable array of Directions
             * in which the piece type (rook or bishop) is allowed
             * to travel
             * @param blockerMask a pointer to an array of blocker
             * masks for the given piece type (rook or bishop)
             * @param magicNumbers a set of all magic numbers
             * for the given piece type
             * @param sizes the size, by square, of the
             * appropriate "Fancy Magic" attack table
             * @link
             *  <a href=
             *  "https://www.chessprogramming.org/Magic_Bitboards">
             *   chessprogramming.org
             *  </a>
             */
            void
            initFancyMagics(FancyMagic** incantations,
                            uint64_t* const attackTable,
                            const Direction *const directions,
                            const uint64_t *const blockerMask,
                            const uint64_t *const magicNumbers,
                            const short *const sizes) {

                // Start the attack pointer at the base of the
                // attack table.
                uint64_t* attackPointer = attackTable;

                // Iterate through every square.
                for (int sq = H1; sq <= A8; ++sq) {

                    // Save the blocker mask for this square.
                    uint64_t mask = blockerMask[sq];

                    // Fire up a Builder to make the fancy
                    // Magic entry.
                    FancyMagic::Builder magicBuilder(attackPointer);

                    magicBuilder
                        // Set the blocker mask for the magic.
                        .setMask(mask);

                    // If Pext isn't available.
#                   ifndef USE_BMI2
                    magicBuilder
                        // Set the shift amount for the
                        // magic.
                        .setShiftAmount(
                            BoardLength - highBitCount(mask)
                        )

                        // Set the magic number for the magic.
                        .setMagicNumber(magicNumbers[sq]);
#                   endif

                    /*
                     * Use the Kervinck "Carry Rippler" method
                     * to generate every possible permutation of
                     * the current mask starting with all bits
                     * low.
                     */
                    uint64_t blockerBoard = 0;
                    do {
                        /*
                         * Build an attack board, iterating
                         * through all four directions and
                         * trying out each move in the current
                         * direction until a blocker is reached.
                         * Add the blocker square to the attack
                         * board. The blocker may be friendly.
                         * The blocker may be hostile. The client
                         * generating moves from these attack
                         * boards must take care to remove target
                         * squares that contain friendly pieces.
                         */
                        uint64_t attackBoard = 0;
                        for (int i = 0; i < 4; ++i) {
                            int o = sq, d = directions[i];
                            uint64_t x;
                            do {
                                if (!withinBounds(o, d)) break;
                                attackBoard |= x =
                                    SquareToBitBoard[o += d];
                            } while ((x & blockerBoard) == 0);
                        }

                        // Place the attack board within the
                        // magic builder at an index calculated
                        // from the current blockerBoard.
                        magicBuilder.placeAttacks(
                                blockerBoard, attackBoard
                        );

                        /*
                         * Find the next permutation.
                         * Take the union of the current
                         * blocker board with the complement
                         * of the current blocker mask.
                         * Increment the resulting number
                         * causing the bits to "ripple"
                         * upwards with the carry.
                         * Take the intersection of the
                         * resulting bits and the current
                         * mask.
                         *
                         * Un-simplified expression :
                         * board = ((board | ~mask) + 1) & mask;
                         */
                        blockerBoard = (blockerBoard - mask) & mask;
                    } while (blockerBoard > 0);

                    // Move the attack pointer forward by the
                    // fancy magic size for this square.
                    attackPointer += sizes[sq];

                    // Instantiate the immutable magic and add
                    // it to the spell book.
                    *incantations++ = magicBuilder.build();
                }
            }

            /**
             * <summary>
             *  <p>
             * A database of magic entries that map rook
             * blocker boards to move boards via a perfect
             * hash function.
             *  </p>
             *
             *  <p>
             * The magic that this array references will
             * be injected at runtime with a call to
             * Witchcraft::init() and de-allocated with
             * a call to Witchcraft::destroy()
             *  </p>
             * </summary>
             *
             * @see Witchcraft::init()
             * @see Witchcraft::destroy()
             */
            FancyMagic* RookAttackWitchcraft[BoardLength];

            /**
             * <summary>
             *  <p>
             * A database of magic entries that map bishop
             * blocker boards to move boards via a perfect
             * hash function.
             *  </p>
             *
             *  <p>
             * The magic that this array references will
             * be injected at runtime with a call to
             * Witchcraft::init() and de-allocated with
             * a call to Witchcraft::destroy()
             *  </p>
             * </summary>
             *
             * @see Witchcraft::init()
             * @see Witchcraft::destroy()
             */
            FancyMagic* BishopAttackWitchcraft[BoardLength];

            namespace Cauldron {

                /**
                 * A table of rook attacks.
                 */
                uint64_t RookAttacks[102400];

                /**
                 * A table of bishop attacks.
                 */
                uint64_t BishopAttacks[5248];

                /**
                 * Whether or not the Witchcraft namespace has
                 * been initialized.
                 */
                bool initialized = false;

            } // namespace Cauldron
        } // namespace (anon)

        /* Witchcraft::init() */
        void init() {
            using namespace Cauldron;
            assert(!initialized);
#       ifndef USE_BMI2
            initFancyMagics(
                    RookAttackWitchcraft, RookAttacks,
                    RookDirections, SquareToRookBlockerMask,
                    RookMagicNumbers, FancyRookSizes
            );
            initFancyMagics(
                    BishopAttackWitchcraft, BishopAttacks,
                    BishopDirections, SquareToBishopBlockerMask,
                    BishopMagicNumbers, FancyBishopSizes
            );
#       else
            initFancyMagics(
                    RookAttackWitchcraft, RookAttacks,
                    RookDirections, SquareToRookBlockerMask,
                    nullptr, FancyRookSizes
            );
            initFancyMagics(
                    BishopAttackWitchcraft, BishopAttacks,
                    BishopDirections, SquareToBishopBlockerMask,
                    nullptr, FancyBishopSizes
            );
#       endif
            initialized = true;
        }

        /* Witchcraft::destroy() */
        void destroy() {
            using namespace Cauldron;
            assert(initialized);
            for(FancyMagic* fm: RookAttackWitchcraft)
                delete fm;
            for(FancyMagic* fm: BishopAttackWitchcraft)
                delete fm;
            initialized = false;
        }

        /*
         * Template specializations for Witchcraft::attackBoard()
         */

        template <> uint64_t
        attackBoard<Rook>(const uint64_t board, const int sq)
        { return RookAttackWitchcraft[sq]->getAttacks(board); }

        template <> uint64_t
        attackBoard<Knight>(const uint64_t board, const int sq)
        { return SquareToKnightAttacks[sq]; }

        template <> uint64_t
        attackBoard<Knight>(const int sq)
        { return SquareToKnightAttacks[sq]; }

        template <> uint64_t
        attackBoard<Bishop>(const uint64_t board, const int sq)
        { return BishopAttackWitchcraft[sq]->getAttacks(board); }

        template <> uint64_t
        attackBoard<Queen>(const uint64_t board, const int sq)
        { return RookAttackWitchcraft[sq]->getAttacks(board) |
                 BishopAttackWitchcraft[sq]->getAttacks(board); }

        template <> uint64_t
        attackBoard<King>(const uint64_t board, const int sq)
        { return SquareToKingAttacks[sq]; }

        template <> uint64_t
        attackBoard<King>(const int sq)
        { return SquareToKingAttacks[sq]; }

        template<Alliance A, PieceType PT>
        uint64_t attackBoard(const int sq)
        { return SquareToPawnAttacks[A][sq]; }

        // Explicit instantiations.
        template uint64_t attackBoard<White, Pawn>(int);
        template uint64_t attackBoard<Black, Pawn>(int);

        /**
         * @copydoc Witchcraft::bb()
         * @param p the bitboard to print
         */
        void bb(uint64_t p) {
            string sb;
            sb.reserve(136);
            for (
                int i = 0; i < BoardLength;
                i++, p >>= 1U
            ) {
                if (fileOf(i) == 0)
                    sb.push_back('\n');
                sb.push_back(p & 1U ? '1' : '-');
                sb.push_back(' ');
            }
            cout << sb;
        }
    } // namespace Witchcraft

    /**
     * @copydoc FancyMagic::FancyMagic()
     * @param b the builder to use in construction
     */
    FancyMagic::
    FancyMagic(const Builder& b) :
            attackBoards(b.attacks),
#           ifndef USE_BMI2
            shiftAmount(b.shiftAmount),
            magicNumber(b.magicNumber),
#           endif
            mask(b.mask)
    {  }

    /**
     * @copydoc FancyMagic::Builder::Builder()
     * @param size the size of the internal
     * attackBoards array.
     */
    FancyMagic::Builder::
    Builder(uint64_t* const attackPointer) :
            attacks(attackPointer),
            shiftAmount(0),
            magicNumber(0),
            mask(0)
    {  }

    /**
     * @copydoc
     * FancyMagic::Builder::setMagicNumber()
     * @param givenNumber the value to set the
     * magic number
     * @return the instance, by reference
     */
    inline FancyMagic::Builder&
    FancyMagic::Builder::
    setMagicNumber(const uint64_t givenNumber)
    { magicNumber = givenNumber; return *this; }

    /**
     * @copydoc
     * FancyMagic::Builder::setMask()
     * @param givenMask the value to set the
     * blocker mask
     * @return the instance, by reference
     */
    inline FancyMagic::Builder&
    FancyMagic::Builder::
    setMask(const uint64_t givenMask)
    { mask = givenMask; return *this; }

    /**
     * @copydoc
     * FancyMagic::Builder::setShiftAmount()
     * @param givenShift
     * @return the instance, by reference
     */
    inline FancyMagic::Builder&
    FancyMagic::Builder::
    setShiftAmount(const int givenShift)
    { shiftAmount = givenShift; return *this; }

    /**
     * @copydoc
     * FancyMagic::Builder::placeAttacks()
     * @param blockerBoard the bitboard to hash
     * @param attackBoard the attack board
     * to associate with the given bit board
     * @return the instance, by reference
     */
    inline FancyMagic::Builder&
    FancyMagic::Builder::
    placeAttacks(uint64_t blockerBoard,
                 uint64_t attackBoard) {
        attacks[HASH(
                blockerBoard, mask,
                magicNumber, shiftAmount
        )] = attackBoard;
        return *this;
    }

    /**
     * @copydoc FancyMagic::Builder::build()
     * @return a pointer to a new FancyMagic
     * instance on the heap
     */
    inline FancyMagic*
    FancyMagic::Builder::build()
    { return new FancyMagic(*this); }
}

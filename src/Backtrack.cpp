//
// Created by evcmo on 12/30/2021.
//

#include "Backtrack.h"

namespace Homura {

     ///////////////////////////////////////////////////////////
    /** 
    *** ALPHA BETA - BACKTRACKING IMPLEMENTATION             
    ***
    *** <summary>
    *** <p>
    *** A classical backtracking Alpha-Beta Search. This search
    *** exploits recursion to traverse necessary lines in a 
    *** memory-efficient depth-first manner.
    *** </p>
    ***
    *** <p>
    *** This search is depth-limited, evaluating with Quiescence
    *** Search at the horizon.
    *** </p>
    ***
    *** <p>
    ***  <b><i>Techniques Implemented:</i></b>
    ***  <ul>
    ***   <li>Principal Variation Search</li>
    ***   <li>Internal Iterative Deepening</li>
    ***   <li>Quiescence Search</li>
    ***   <li>Transposition Table</li>
    ***   <li>Static Null Move Pruning</li>
    ***   <li>Null Move Pruning</li>
    ***   <li>Razoring</li>
    ***   <li>Futility Pruning</li>
    ***   <li>Late Move Pruning</li>
    ***   <li>Late Move Reductions</li>
    ***   <li>Fail-Soft</li>
    ***  </ul>
    *** </p>
    *** </summary>
    ***
    *** @param b the board
    *** @param d the depth (ply)
    *** @param r the remaining dpeth
    *** @param a alpha
    *** @param o beta
    *** @param c the search controls
    *** @return the evaluation
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    template
    <Alliance A, NodeType NT, bool DO_NULL>
    int32_t alphaBeta
        (
        Board* const b,     /** Board           */
        const int d,        /** Depth (ply)     */
        const int r,        /** Remaining Depth */
        int32_t a,          /** Alpha           */
        int32_t o,          /** Beta            */
        control* const c    /** Search Controls */
        ) 
    {   
        /**
         * If we are out of time,
         * return.
         */
        const int32_t 
            el = elapsed(c->epoch);
        if(el >= c->time) 
            return 0;

        /** Count the nodes. */
        ++c->NODES;

        /**
         * If draw, return the
         * draw score;
         */
        if(NT != ROOT && 
            (!isMatePossible(b) 
            || repeating(b, d)))
            return contempt(b);

        /**
         * If the node isn't
         * a draw, but
         * we are at the
         * depth limit,
         * evaluate and return.
         */
        if(r <= 0) {

            /**
             * start a q-search, 
             * stand pat and try 
             * to attacks.
             */
            return 
                quiescence<A>
                (b, d, r, a, o, c);
        }

        /** Save alpha */
        const int32_t oa = a;
        Move ttmove = NullMove;

        /**
         * try retrieving 
         * this node from the 
         * transposition
         * table.
         */
        uint64_t key = 
        b->getState()->key;
        Entry* tt = 
            retrieve(key, el);
        
        /**
         * If the entry exists.
         */
        if(tt != nullptr && 
            tt->move != NullMove) {

            /**
             * If the entry is valid
             * and we are not at the
             * root of a normal or
             * IID search.
             */
            if(tt->depth >= r && 
            NT != ROOT && NT != IID) {

                /**
                 * Get this node's score.
                 */
                int32_t score = tt->value;

                /**
                 * Adjust for mate.
                 */
                if(score <= -MateValue) 
                    score += d;
                else if(score >= MateValue) 
                    score -= d;

                /**
                 * If the entry type is
                 * exact, return its 
                 * score.
                 */
                if(tt->type == exact)
                    return score;

                /**
                 * If the entry type is
                 * lower or upper, we
                 * can use it to tighten
                 * the bounds for this
                 * node.
                 */
                if(tt->type == lower)
                    a = std::max
                        (a, score);
                else if(tt->type == upper)
                    o = std::min
                        (o, score);

                /**
                 * Return the score if 
                 * a beta cutoff occurs.
                 */
                if(a >= o)
                    return score;
            }

            /**
             * Set the PV move for
             * use in move ordering.
             */
            ttmove = tt->move;
        }

        /**
         * Is this a PV node?
         */
        constexpr bool pvNode = 
            NT != NONPV;

        /**
         * Are we in check?
         */
        const bool inCheck = 
        attacksOn<A, King>(
        b, bitScanFwd
        (b->getPieces<A, King>()));

        /**
         * Evaluate the board.
         */
        const int32_t ev = 
            c->evals[d] = inCheck? 
            -mateEval(d): eval<A>(b);

        /**
         * Is the evaluation an
         * improvement for us?
         */
        const bool improving = 
            d > 2 && ev > c->evals[d - 2];

        /**
         * The Static Null Move
         * (Reverse Futility) Pruning 
         * margin.
         */
        const int32_t rfMargin = 
            50 + 100 * (r + improving);

        /**
         * Static Null Move Pruning.
         * Drofa style. If the 
         * current evaluation is
         * greater than beta by
         * a significant margin
         * and the node is at a
         * low depth, this node is 
         * probably too good. One of
         * the moves will probably 
         * cause a fail high. Just 
         * save the effort and prune.
         */
        if(!inCheck && 
            !pvNode &&
            r <= RFP_RD &&
            std::abs(o) < MinMate &&
            (ev - rfMargin) >= o) {
            return o;
        }

        /**
         * Null-Move pruning.
         * The idea is that, if
         * this node is way too
         * good (i.e. we can skip
         * a move and a reduced-depth
         * null-window search fails 
         * high), there isn't much 
         * reason to make a move and 
         * search deeper because we 
         * would likely still fail high.
         * I.e. the optimal line of play
         * has a very low probability
         * of reaching here because 
         * the opponent almost 
         * certainly has a way to 
         * avoid this node. Prune.
         */
        if(DO_NULL && 
            !inCheck && 
            !pvNode &&
            r >= NMP_RD && 
            d > c->NULL_PLY && 
            b->hasMajorMinor()) {
            State s;
            b->applyNullMove(s);
            const int32_t nms = 
            -alphaBeta<~A, NONPV, false>
            (
                b, d + 1,
                r - 1 - NULL_R, 
                -o, -o + 1, c
            );
            b->retractNullMove();
            if(nms >= o && 
               std::abs(nms) < MinMate) 
                return o;
        }

        /**
         * The razoring margin.
         */
        const int32_t rMargin = r * 300;

        /**
         * Try Razoring.
         * If at a low depth,
         * when the evaluation
         * is really bad, if 
         * a q search cannot 
         * raise alpha by at least
         * the margin, it isn't
         * likely that any of the
         * moves will raise alpha.
         * Save time and prune.
         */
        if(!inCheck && 
            !pvNode &&
            r <= RAZ_RD &&
            (ev + rMargin) < a) {
            const int32_t rs = 
            quiescence<A>
            (
                b, d, 0,
                a - 1, a, c
            );
            if((rs + rMargin) < a) 
                return a;
        }

        /**
         * The futility margin.
         */
        const int32_t fMargin = 
            100 + (r - 1) * 70;

        /**
         * See if this is a futile 
         * node. (static evaluation
         * is less than alpha by a
         * significant margin at a
         * low remaniing depth). 
         * If the node is futile, 
         * we can prune unintersting 
         * children as they probably 
         * aren't going to give us
         * anything useful (raise 
         * alpha). Inspired by CPW-
         * engine and Blunder.
         */
        const bool futile =
            r <= FUT_RD &&
            !pvNode &&
            std::abs(a) < MinMate &&
            std::abs(o) < MinMate &&
            (ev + fMargin) < a;

        /**
         * Internal iterative 
         * deepening. If this is a 
         * pv node at some distance
         * from the horizon and we 
         * don't have a PV move, 
         * run a shallow search to 
         * get one.
         */
        if(r >= IID_RD && pvNode && 
            ttmove == NullMove) {
            c->iidMoves[d] = NullMove;
            alphaBeta<A, IID, true>
            (
                b, d, r - IID_R,
                a, o, c
            );
            ttmove = c->iidMoves[d];
        } 

        /**
         * Set the pv move to be
         * used in sorting.
         */
        c->pvMove = ttmove;
        
        /**
         * Find the next node type 
         * for PVS.
         */
        constexpr NodeType _N = 
            pvNode? PV: NONPV;

        /**
         * Generate the moves.
         * PV, MVV-LVA, Killers, 
         * History.
         */
        MoveList<AB> ml(b, c, d);

        /**
         * If the move list is
         * empty, this position
         * is either a checkmate
         * or a stalemate.
         */
        if(ml.length() <= 0) {

            /**
             * If checkmate, 
             * evaluate
             * and return.
             */
            if(inCheck)
                return 
                -mateEval(d);

            /**
             * If stalemate,
             * return 0.
             */
            else return 0;
        } 

        /**
         * Initialize iterator
         * pointers.
         */
        Move *  k = ml.begin(),
        * const base = k,
        * const e = ml.end();

        /**
         * Set high score to int min
         * so that it is immediately
         * replaced.
         */
        int32_t highScore = INT32_MIN;
        Move hm = NullMove;

        /**
         * Loop through every 
         * legal move. We have
         * at least one, as
         * we aren't in mate.
         */
        do {

            /**
             * A state for move
             * making.
             */
            State s;

            /**
             * The score of the
             * current move. 
             */
            int32_t score;  

            /**
             * R is the amount we will
             * reduce the depth for LMR.
             */
            int R = 0;  

            /**
             * Do the move.
             */
            b->applyMove(*k, s);

            /**
             * Does this move
             * put the enemy
             * in check?
             */
            const bool giveCheck = 
            attacksOn<~A, King>(
            b, bitScanFwd
            (b->getPieces<~A, King>()));  

            /**
             * Is this move an
             * attack move?
             */
            const bool isAttack = 
                b->hasAttack();

            /**
             * Is this move
             * concerning enough
             * to keep our
             * attention?
             */
            const bool concern = 
                isAttack ||
                inCheck || 
                k->isPromotion() ||
                giveCheck ||
                c->isKiller(d,*k);

            /*
             * PV Search
             */
            if(k <= base) {

                /**
                 * Do a normal search
                 * beneath the first move.
                 */
                score = 
                -alphaBeta<~A, _N, true>
                (
                    b, d + 1, r - 1,
                    -o, -a, c
                );

                /**
                 * Bypass null window
                 * search and re-search.
                 */
                goto scoring;
            }   

            /**
             * Late Move Pruning.
             * Prunes moves that 
             * the move ordering
             * algorithm considers to
             * be undesirable.
             */
            if(r <= LMP_RD && 
                !pvNode &&
                !concern &&
                (k - base) > lmpMargins[r]) {
                b->retractMove(*k); 
                continue;
            }

            /**
             * Futility Pruning.
             */
            if(!concern && futile) {
                b->retractMove(*k);
                continue;
            }

            /**
             * Late Move Reductions.
             */
            if(!concern && r >= LMR_RD) {

                /**
                 * If this is a
                 * pv node, reduce 
                 * carefully. If not,
                 * reduce a few plies
                 * depending on the 
                 * remaining depth and
                 * the number of moves
                 * we have seen so far.
                 */
                R = pvNode? 
                    1 + (k - base) / 12: 

                    /**
                     * From Blunder.
                     */
                    std::max(2, r / 4) + 
                    (k - base) / 12;

                /**
                 * Try out the 
                 * reduced-depth
                 * search. If it
                 * doesn't raise
                 * alpha, there is
                 * no need to 
                 * re-search.
                 */
                score = 
                -alphaBeta<~A, NONPV, true>
                (
                    b, d + 1,
                    r - 1 - R, 
                    -a - 1, -a, c
                );

                /**
                 * If we haven't
                 * managed to 
                 * raise alpha,
                 * don't re-search.
                 */                        
                if(score <= a) {
                    goto scoring;
                }

                /**
                 * If the score is
                 * greater than alpha,
                 * try again at full
                 * depth.
                 */
            }

            /**
             * Search at full depth
             * with null window.
             */
            score = 
            -alphaBeta<~A, NONPV, true>
            (
                b, d + 1,
                r - 1, 
                -a - 1, -a, c
            );
            
            /** 
             * If our search
             * lands within the
             * full window,
             * re-search with 
             * the full window.
             */
            if((score > a && (R > 0 || 
            NT == ROOT || score < o))) {
                score = 
                -alphaBeta<~A, _N, true>
                (
                    b, d + 1, r - 1,
                    -o, -a, c
                );
            } 

            /**
             * See if this move's 
             * score is significant.
             */
            scoring:

            /**
             * Undo the move.
             */
            b->retractMove(*k);  

            /**
             * If we fail to raise
             * the high score, 
             * continue.
             */
            if(score <= highScore) 
                continue;

            /**
             * update the high score.
             * set the move for the
             * transposition table.
             */
            highScore = score;
            if constexpr (NT == IID)
                c->iidMoves[d] = *k;
            if constexpr (NT == ROOT)
                c->bestMove = *k;
            hm = *k;

           /*
            * If we fail to raise
            * alpha, continue.
            */
            if(score <= a) 
                continue;

            /**
             * Normal alpha-beta 
             * pruning.
             */
            if(score >= o) {
                
                /**
                 * If the move
                 * was an attack,
                 * don't update
                 * the history.
                 */
                if(isAttack)
                    break;
                
                /**
                 * Update the
                 * history.
                 * Update the 
                 * killers.
                 */
                c->updateHistory<A>
                (
                    k->origin(),
                    k->destination(), 
                    r
                );
                c->addKiller(d, *k);
                break;
            }

            /**
             * If the move isn't
             * an attack, raise
             * the history by
             * a little bit.
             * We want to prefer
             * moves that raise
             * alpha (just not
             * quite as much as
             * we want to prefer
             * moves that cause
             * a beta-cutoff).
             */
            if(!isAttack) {
                c->raiseHistory<A>
                (
                    k->origin(),
                    k->destination(), 
                    r
                );
            }

            /**
             * Set alpha.
             */
            a = score;
        } while(++k < e);

        /**
         * Cache this node's
         * info in the
         * transposition
         * table.
         */
        store(
            key, highScore, 
            highScore <= oa ? upper: 
            highScore >= o  ? lower: 
            exact, r, hm,
            elapsed(c->epoch)
        );

        /**
         * Return the high 
         * score.
         */
        return highScore;
    }

      //////////////////////////////////////////////////////////
     // EXPLICIT INSTANTIATIONS
    ////////////////////////////////////////////////////////////

    template int32_t alphaBeta<White, PV>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t alphaBeta<Black, PV>
    (Board*, int, int, int32_t, int32_t, control*);

    template int32_t alphaBeta<White, PV, false>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t alphaBeta<Black, PV, false>
    (Board*, int, int, int32_t, int32_t, control*);

    template int32_t alphaBeta<White, ROOT>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t alphaBeta<Black, ROOT>
    (Board*, int, int, int32_t, int32_t, control*);

    template int32_t alphaBeta<White, NONPV, false>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t alphaBeta<Black, NONPV, false>
    (Board*, int, int, int32_t, int32_t, control*);

    template int32_t alphaBeta<White, IID, true>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t alphaBeta<Black, IID, true>
    (Board*, int, int, int32_t, int32_t, control*);

    template int32_t alphaBeta<White, NONPV, true>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t alphaBeta<Black, NONPV, true>
    (Board*, int, int, int32_t, int32_t, control*);

     ///////////////////////////////////////////////////////////
    /** 
    *** QUIESCENCE SEARCH - BACKTRACKING IMPLEMENTATION
    ***
    *** <summary>
    *** <p>
    *** A classical backtracking Quiescence Search.
    *** </p>
    ***
    *** <p>
    *** Depth-limited Alpha-Beta has a horizon &mdash; a ply
    *** that the search cannot see past. A static evaluation
    *** function is used on the horizon in place of a deeper
    *** search. At quiet positions, this static evaluation is
    *** safe. At loud positions, however, static evaluation
    *** can miss critical exchanges that occur just beyond the
    *** horizon. This is known as the Horizon Effect, and, in
    *** some cases, it can cause the search to blunder.
    *** </p>
    ***
    *** <p>
    *** Quiescence Search aims to mitigate the Horizon Effect.
    *** It does this by evaluating loud positions with a 
    *** selective search. This search stands pat with a static 
    *** evaluation and tries out attack moves only, attempting 
    *** to improve upon the evaluation.
    *** </p>
    ***
    *** <p>
    *** This implementation uses the Fail-Hard approach. It
    *** seems to perform better than Fail-Soft for Homura.
    *** </p>
    *** </summary>
    ***
    *** @param b board
    *** @param d the depth (ply)
    *** @param r the remaining depth
    *** @param a alpha
    *** @param o beta
    *** @param c the search controls
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    template<Alliance A>
    int32_t quiescence
        (
        Board* const b,     /** Board           */
        const int d,        /** Depth (ply)     */
        const int r,        /** Remaining Depth */
        int32_t a,          /** Alpha           */
        int32_t o,          /** Beta            */
        control* const c    /** Search Controls */
        ) 
    { 
        if(abort(c->time, c->epoch)) 
            return 0;

        /* Count the nodes. */
        ++c->NODES;

        /**
         * If we are drawn,
         * return 0.
         */
        if(!isMatePossible(b) || 
            repeating(b, d))
            return 0;

        /**
         * Are we in check?
         * If so, extend.
         */
        if(attacksOn<A, King>(
            b, bitScanFwd
        (b->getPieces<A, King>()))) {
            c->pvMove = NullMove;

            /**
             * We need to try to get 
             * out of check.
             * Try out both attacks
             * and quiets.
             */
            MoveList<AB> ml(b, c, d);

            /**
             * If the move list is
             * empty, return mate
             * score.
             */
            if(ml.length() <= 0) 
                return -mateEval(d);
            
            /**
             * Initialize iterator
             * pointers.
             */
            Move *  k = ml.begin(),
            * const e = ml.end();

            /**
             * Loop through every 
             * legal move.
             */
            for(State s;;) {

                /**
                 * Do the move.
                 */
                b->applyMove(*k, s);

                /**
                 * Get this move's
                 * score.
                 */
                const int32_t score = 
                -quiescence<~A>
                (
                    b, d + 1, r - 1,
                    -o, -a, c
                );

                /**
                 * Undo the move.
                 */
                b->retractMove(*k);  

                /**
                 * Return beta if we
                 * fail high. Reset
                 * alpha when it is
                 * raised.
                 */
                if(score >= o) return o;
                if(score > a) a = score;

                /**
                 * Loop condition.
                 */
                if(++k >= e) break;
            }

            /**
             * Return alpha if no
             * beta cutoff.
             */
            return a;
        }

        /**
         * If we've gone too
         * deep, just evaluate and
         * return the score.
         */
        if(r <= -c->Q_PLY) 
            return 
                eval<A>(b);

        /**
         * If we aren't in check,
         * stand pat.
         */
        int32_t sp = eval<A>(b);
        if(sp >= o) return o;
        if(a < sp)  a = sp;

        /**
         * MVV-LVA
         */
        c->pvMove = NullMove;
        MoveList<Q> ml(b, c, d);
        
        /**
         * Initialize iterator
         * pointers.
         */
        Move *  k = ml.begin(),
        * const e = ml.end();

        /**
         * Loop through every 
         * legal move.
         */
        for(State s; k < e; ++k) {

            /**
             * Do the move.
             */
            b->applyMove(*k, s);

            /**
             * Get this move's
             * score.
             */
            const int32_t score = 
            -quiescence<~A>
            (
                b, d + 1, r - 1,
                -o, -a, c
            );

            /**
             * Undo the move.
             */
            b->retractMove(*k);  

            /**
             * Return beta if we
             * fail high. Reset
             * alpha when it is
             * raised.
             */
            if(score >= o) return o;
            if(score > a) a = score;
        }
        
        /**
         * Return alpha if no
         * beta cutoff.
         */
        return a;
    }

      //////////////////////////////////////////////////////////
     // EXPLICIT INSTANTIATIONS
    ////////////////////////////////////////////////////////////

    template int32_t quiescence<White>
    (Board*, int, int, int32_t, int32_t, control*);
    template int32_t quiescence<Black>
    (Board*, int, int, int32_t, int32_t, control*);

      //////////////////////////////////////////////////////////
     // ITERATIVE DEEPENING - FOR SCIENCE
    ////////////////////////////////////////////////////////////

    // Move search
    //     (
    //     Board *const b,
    //     char* info,
    //     control& q,
    //     int time
    //     )
    // {
    //     q.epoch = system_clock::now();
    //     q.time = time;
    //     int depth = 1;
    //     Move bestYet = NullMove;
    //     int64_t beta = INT64_MAX, alpha = -INT64_MAX;
    //     int nodes = 0;
    //     do {
    //         q.MAX_DEPTH = depth;
    //         q.NULL_PLY = depth / 4;
    //         q.Q_PLY    = 65;
    //         q.NODES    = 0;
    //         int64_t score = INT32_MIN;
    //         Alliance a = b->currentPlayer();
    //         if(a == White)
    //             score = -alphaBeta
    //             <White, ROOT>
    //             (
    //                 b, 0, depth,
    //                 alpha, beta, &q
    //             );
    //         else 
    //             score = -alphaBeta
    //             <Black, ROOT>
    //             (
    //                 b, 0, depth,
    //                 alpha, beta, &q
    //             );
    //         int64_t ms = elapsed(q.epoch);
    //         if(ms >= time) break;
    //         bestYet = q.bestMove;
    //         nodes += q.NODES;
    //         // if(score <= alpha || score >= beta) {
    //         //     alpha = -INT32_MAX;
    //         //     beta = INT32_MAX;
    //         //     continue;
    //         // }
    //         std::cout << "info depth " 
    //                   << depth
    //                   << " score cp " 
    //                   << -score
    //                   << " nodes "
    //                   << q.NODES
    //                   <<  " nps " 
    //                   << (ms >= 1000? nodes / (ms / 1000): nodes)
    //                   << " time "
    //                   << ms << '\n';
    //         // alpha = score - 35;
    //         // beta = score + 35;
    //         ++depth;
    //     } while(true);
    //     return bestYet;
    // }
}
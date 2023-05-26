#include "Rollout.h"

namespace Homura {
    namespace {

         ///////////////////////////////////////////////////////////
        /** 
        *** ALPHA BETA - ROLLOUT IMPLEMENTATION             
        ***
        *** <summary>
        *** <p>
        *** An implementation of Dr. Bojun Huang's Alpha-Beta 
        *** rollout algorithm. This algorithm walks through 
        *** candidate PV lines in a sequence of depth-limited 
        *** rollouts. It visits all other parts of the tree with 
        *** backtracking.
        *** </p>
        ***
        *** <p>
        ***  <b><i>Techniques Implemented:</i></b>
        ***  <ul>
        ***   <li>Principal Variation Search</li>
        ***   <li>Internal Iterative Deepening</li>
        ***   <li>Transposition Table With Multiple Visits</li>
        ***   <li>Quiescence Search</li>
        ***   <li>Leftmost-Greedy Tree Policy</li>
        ***  </ul>
        *** </p>
        *** </summary>
        *** 
        *** @param b the board
        *** @param n the current node
        *** @param d the depth (ply)
        *** @param r the remaining depth
        *** @param gc the garbage collector
        *** @param c the search controls
        *** @author Ellie Moore
        *** @version 05.11.2023
         *//////////////////////////////////////////////////////////

        template<Alliance A>
        void alphaBetaRollout
            (
            Board* const b,     /** Board             */
            Node* const n,      /** Current Node      */
            const int d,        /** Depth (ply)       */
            const int r,        /** Remaining Depth   */
            MemManager &gc,     /** Garbage Collector */
            control* const c    /** Search Controls   */
            ) 
        {         
            /**
             * Are we out of time?
             * If so, quit.
             */
            int64_t el = 
                elapsed(c->epoch);
            if(el >= c->time)
                return;

            /**
             * Is this a terminal node? 
             * If it is, we can just
             * evaluate it.
             */
            uint8_t term = n->terminal();

            /**
             * If the current Child is
             * a win, return the mate
             * score.
             */
            if(term == WIN) {
                n->setScore(-mateEval(d));
                return;
            } 

            /**
             * If the current Child is
             * a draw, return the draw
             * score.
             */
            if(term == DRAW) {
                n->setScore(contempt(b));
                return;
            }

           /*
            * If we reach a
            * non-terminal
            * node at max depth.
            */
            if(r <= 0) {

                /**
                 * If we are at r == 0,
                 * evaluate by 
                 * backtracking Q 
                 * search.
                 */
                n->qSearch<A>(b, c);
                return;
            }

            /**
             * Init alpha, beta. Save
             * alpha.
             */
            int32_t 
                alpha = n->getAlpha(), 
                beta  = n->getBeta(),
                oa = alpha;

            /**
             * Clear the pv move.
             */
            c->pvMove = NullMove;

            /**
             * try retrieving
             * this node from the 
             * transposition
             * table.
             */
            uint64_t key = 
            b->getState()->key;
            Entry* tt = retrieve(key, el);

            /**
             * If the entry exists.
             */
            if(tt != nullptr && 
                tt->move != NullMove) {   

                /**
                 * If the entry is valid
                 * and we are not at the
                 * root.
                 */
                if(tt->depth >= r && 
                    n->getParent()) {
                    
                    /**
                     * Get this Node's score.
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
                     * exact, set the score
                     * and start backprop.
                     */
                    if(tt->type == exact) {
                        n->setScore(score);
                        return;
                    }

                    /**
                     * If the entry type is
                     * lower or upper, we
                     * can use it to tighten
                     * the bounds for this
                     * node.
                     */
                    if(tt->type == lower)
                        alpha = std::max
                            (alpha, score);
                    else if(tt->type == upper)
                        beta = std::min
                            (beta, score);

                    /**
                     * set the score and 
                     * start backprop if 
                     * a beta cutoff occurs.
                     */
                    if(alpha >= beta) {
                        n->setScore(score);
                        return;
                    }
                }

                /**
                 * Set the PV move for
                 * use in move ordering.
                 */
                c->pvMove = tt->move; 
            } 

            /**
             * Are we in check?
             */
            const bool inCheck = 
            attacksOn<A, King>(
            b, bitScanFwd
            (b->getPieces<A, King>()));

            /**
             * If we have no children.
             */
            if(n->hasNoChildren()) {

                /**
                 * INTERNAL ITERATIVE DEEPENING
                 */
                if(c->pvMove == NullMove && 
                   r >= IID_RD) c->pvMove =
                    n->iidSearch<A>(b, d, r, c);

                /**
                 * Try making the children.
                 * If there isn't enough
                 * room to make the children,
                 * V-, V+, and the score will
                 * be set via backtracking 
                 * search. Start backprop.
                 */
                if(!n->expand<A>(b, d, r, gc, c))
                    return;
            }  

            int idx = 0;

            /**
             * Select a child with
             * leftmost-max tree policy.
             */
            Node* k = n->select(idx, r);

            /**
             * If the child is null...
             * we should assert false,
             * as this shouldn't ever
             * happen... But if it does
             * happen in the release build, 
             * just return.
             */
            if(!k) { assert(false); return; }

            /**
             * Get the move.
             */
            Move move = k->getMove();
            State s;

            /**
             * do the move.
             */
            b->applyMove(move, s);

            /**
             * SEARCH DEEPER
             */
            if(k->reSearch() ||
                idx == 0 ||

                /**
                 * NON-PV SEARCH
                 */
                k->nonPVSearch<A>
                (b, inCheck, d, r, idx, c)) {

                /**
                 * PV SEARCH
                 */
                alphaBetaRollout<~A>
                (
                    b, k, d + 1, r - 1,
                    gc, c
                );
            }

            /**
             * undo the move.
             */
            b->retractMove(move);
            
            /**
             * Backpropagate.
             */
            n->backprop();

            /**
             * Cache this node in the
             * transposition table once
             * it has been evaluated
             * completely.
             */
            if(n->converged()) {

                Move pvMove = n->getPVMove();
                int32_t highScore = n->getScore();

                /**
                 * Store the node.
                 */
                store(
                    key, highScore, 
                    highScore <= oa ? upper: 
                    highScore >= beta ? lower: 
                    exact, r, pvMove,
                    elapsed(c->epoch)
                );
            }
        }

         ///////////////////////////////////////////////////////////
        /** 
        *** ITERATIVE DEEPENING - ROLLOUT IMPLEMENTATION             
        ***
        *** <summary>
        *** <p>
        *** An implementation of Dr. Huang's Iterative Deepening 
        *** loop.
        *** </p>
        ***
        *** <p>
        *** As the candidate PV lines constitute a small portion of
        *** the full search tree and vary from depth to depth, this
        *** iterative deepening loop builds a new tree for each
        *** depth iteration, passing the previous iteration's tree
        *** to a garbage collector.
        *** </p> 
        *** </summary>
        ***
        *** @param _b the board
        *** @param n the current node
        *** @param gc the garbage collector
        *** @param time the allotted time
        *** @param bestMove the best move to set (for future 
        *** parallelization, keep this as an "out" param)
        *** @param c the search controls
        *** @author Ellie Moore
        *** @version 05.11.2023
         *//////////////////////////////////////////////////////////

        template<Alliance A>
        void worker
            (
            Board *const _b,        /** Board             */
            Node* n,                /** Current Node      */
            MemManager& gc,         /** Garbage Collector */
            const int time,         /** allotted time     */
            Move& bestMove,         /** Best Move         */
            control& c              /** Search Controls   */
            )
        {
            /**
             * Build the new board.
             */
            Board b = Board::Builder
            <Default>(*_b).build();

            /**
             * Reset the search 
             * controls.
             */
            c.epoch = system_clock::now();
            c.time = time;
            c.MAX_DEPTH = 1;
            c.NODES = 0;
            c.Q_PLY = MaxDepth;
            c.ageHistory();
            c.NULL_PLY = 0;

            /**
             * Main iterative deepening 
             * loop.
             */
            while(c.MAX_DEPTH < MaxDepth && 
                !abort(time, c.epoch)) {

                /**
                 * Do an alpha-beta rollout
                 * from the root.
                 */
                alphaBetaRollout<A>
                    (
                    &b, n, 0, c.MAX_DEPTH,
                    gc, &c
                    ); 

                /**
                 * If V- and V+ at the root
                 * haven't crossed, update
                 * alpha and beta and do
                 * the next rollout at the
                 * same depth.
                 */
                if(!n->converged()) {
                    n->updateAB();
                    continue;
                }

                /**
                 * Set pv move.
                 */
                bestMove = n->getPVMove();

                /**
                 * Print UCI info.
                 * Increment the depth.
                 */
                std::cout 
                << "info depth " 
                << c.MAX_DEPTH++ 
                << " score cp "
                << n->getScore()
                << " nodes " 
                << gc.getTotal() << '\n';

                /**
                 * Set the ply at which
                 * we will allow null-move
                 * pruning. From Leorik.
                 */
                c.NULL_PLY = c.MAX_DEPTH >> 2U;
                
                // std::cout << "info pv ";
                // MemManager::printPV(n);
                // std::cout << '\n';
                
                /**
                 * Collect the root and
                 * move to the next.
                 */
                gc.collect(n++);

                /**
                 * Reset the allocated
                 * node count.
                 */
                gc.reset();
            }
        }
    }

     ///////////////////////////////////////////////////////////
    /** 
    *** NODE - NON PV SEARCH           
    ***
    *** <summary>
    *** <p>
    *** Since the classical move-ordering algorithm is good, the 
    *** first move is usually the best move. The first move is 
    *** pretty likely to raise alpha and become the last move to 
    *** do so. Therefore, PVS searches every following move with 
    *** a null window around alpha to confirm that the first 
    *** move is the best. If any of these moves raise alpha, it 
    *** must be searched again with the full window, and our
    *** belief about the best move must be adjusted.
    *** </p>
    ***
    *** <p>
    *** During a rollout, when a node is initially selected, 
    *** if the node isn't the first node in the list, Homura
    *** searches it with backtracking and a null window to 
    *** confirm that a full window search won't raise alpha. 
    *** However, if the search fails to confirm our belief,
    *** Homura re-searches the node with rollouts and the
    *** full window.
    *** </p>
    ***
    *** <p>
    *** Homura's non-PV search also incorporates cautious late
    *** move reductions.
    *** </p>
    *** </summary>
    ***
    *** @param b the board
    *** @param inCheck whether the alliance, A, is in check
    *** @param d the depth (ply)
    *** @param r the remaining depth
    *** @param i this node's index
    *** @param c the search controls
    *** @return whether we need to re-search the chosen child
    *** of this node
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    template<Alliance A>
    inline bool 
    Node::nonPVSearch
        (
        Board* const b,         /** Board             */
        const bool inCheck,     /** Are We In Check?  */
        const int d,            /** Depth (ply)       */
        const int r,            /** Remaining Depth   */
        const int i,            /** This Node's Index */
        control* const c        /** Search Controls   */
        ) 
    {
        int R = 0;

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
         * Is this move
         * concerning enough
         * to keep our
         * attention?
         */
        const bool concern = 
            b->hasAttack() ||
            inCheck || 
            move.isPromotion() ||
            giveCheck ||
            c->isKiller(d, move);

        /**
         * LATE MOVE REDUCTIONS
         */
        if(r >= LMR_RD && !concern) {

            /**
             * We're in a PV node, so
             * reduce conservatively
             */
            R = 1 + i / 12;

            /**
             * REDUCED NULL-WINDOW SEARCH
             */
            int32_t sc = -alphaBeta
            <~A, NONPV, true>
            (
                b, d + 1, r - 1 - R,
                -parent->alpha - 1, 
                -parent->alpha, c
            );

            /**
             * If we haven't raised alpha,
             * There is no reason to re-search.
             */
            if(sc <= parent->alpha) {
                vminus = vplus = score = -sc;
                return false;
            }
        }

        /**
         * NORMAL NULL-WINDOW SEARCH
         */
        int32_t sc = -alphaBeta
        <~A, NONPV, true>
        (
            b, d + 1, r - 1,
            -parent->alpha - 1, 
            -parent->alpha, c
        );

        /**
         * If we land within the full
         * window, then we need to 
         * re-search. We will do so via
         * rollout.
         */
        if(sc > parent->alpha && 
          (R > 0 || d == 0 || 
           sc < parent->beta)) {
                
            /**
             * Set re-search flag.
             */
            flags |= ReMask;
            return true;
        }

        /**
         * If we haven't raised alpha,
         * There is no reason to re-search.
         */
        vminus = vplus = score = -sc;
        return false;
    }

    template bool Node::nonPVSearch<White>
        (
        Board* const b,
        const bool inCheck,     
        const int d, 
        const int r, 
        const int i,
        control* const c
        );

    template bool Node::nonPVSearch<Black>
        (
        Board* const b,
        const bool inCheck, 
        const int d, 
        const int r, 
        const int i,
        control* const c
        );

     ///////////////////////////////////////////////////////////
    /** 
    *** NODE - EXPAND            
    ***
    *** <summary>
    *** <p>
    *** During a rollout, expansion is the step that adds new 
    *** nodes to the tree. If we don't have enough space to add
    *** new nodes, we can simply evaluate this node with a 
    *** backtracking Alpha-Beta search to the remaining depth
    *** just like in the classical search. However, this is
    *** very unlikely to happen unless the user gives the search
    *** an excessive amount of time.
    *** </p>
    *** </summary>
    ***
    *** @param b the board
    *** @param d the depth (ply)
    *** @param r the remaining depth
    *** @param gc the garbage collector
    *** @param c the search controls
    *** @return whether this method added new nodes to the tree
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    template<Alliance A>
    bool Node::expand
        (
        Board *const b,     /** Board             */
        const int d,        /** Depth (ply)       */
        const int r,        /** Remaining Depth   */
        MemManager &gc,     /** Garbage Collector */
        control* const c    /** Search Controls   */
        ) 
    {
        /**
         * If we have too many
         * nodes in the tree,
         * just evaluate this
         * node by backtracking
         * search.
         */
        if(gc.maxNodesExceeded()) {
            vminus = vplus = score = 
            alphaBeta<A, PV, true>
            (
                b, d, r,
                alpha, 
                beta, c
            );
            return false;
        }

        /**
         * Generate the moves.
         * PV, MVV-LVA, Killers, 
         * History for a 
         * significant speedup.
         */
        MoveList<AB> ml(b, c, d);
        Move *k = ml.begin(),
             *e = ml.end();

        /**
         * Iterate through the moves.
         * Expand.
         */
        for(State s; k < e; ++k) {

            /**
             * do the move.
             */
            b->applyMove(*k, s);

            /**
             * Are we in check?
             */
            const bool inCheck = 
            attacksOn<~A, King>(
            b, bitScanFwd
            (b->getPieces<~A, King>()));

            /**
             * Generate reply moves.
             * Our move generator is
             * fast enough that this
             * probably doesn't impact
             * search speed... But 
             * admittedly, it is very
             * inefficient.
             * 
             * TODO:
             * In the future, consider 
             * storing moves or finding
             * evasions.
             */
            MoveList<MCTS> reply(b);

            /*
             * Check if the current node
             * is a terminal node. If it
             * is, set the term flags.
             * Set the score to negative 
             * infinity so that we ignore
             * it during backprop.
             */
            children
                .push_back(gc.alloc(
                    this, *k, 
                    reply.length() <= 0?
                    (inCheck? WIN: DRAW):
                    (!isMatePossible(b) 
                    || repeating(b, d)? 
                    DRAW: NOT), 
                    INT32_MIN
                ));
                
            /**
             * undo the move
             */
            b->retractMove(*k);
        }
        return true;
    }

    template bool Node::expand<Black>
        (
        Board *const b,
        const int d,
        const int r,
        MemManager &gc,
        control* const c
        );

    template bool Node::expand<White>
        (
        Board *const b,
        const int d,
        const int r,
        MemManager &gc,
        control* const c
        );
    
     ///////////////////////////////////////////////////////////
    /** 
    *** NODE - SELECT            
    ***
    *** <summary>
    *** <p>
    *** During a rollout, selection is the step that occurs at
    *** each internal node visited. In this step, the search
    *** chooses a child of the current node to visit.
    *** </p>
    ***
    *** <p>
    *** The tree policy for this selection routine has two parts.
    ***  <ol>
    ***   <li>
    ***    <b>Leftmost</b>
    ***    <p> 
    ***    This is the tree policy used in the classical 
    ***    backtracking version of the search. It chooses the
    ***    leftmost child, determined to be the next-best by the
    ***    move-ordering algorithm.
    ***    </p>
    ***    <p>
    ***    Homura chooses the leftmost child whenever its
    ***    index in the move list is less than the remaining
    ***    depth times two. The idea is to abandon the classical
    ***    move ordering technique if we are performing a re-
    ***    search late in the list. (Just an ameteur idea, but 
    ***    it seems to help a little bit. I'd guess 0-10 elo)
    ***    </p>
    ***   </li>
    ***   <li>
    ***    <b>Greedy</b>
    ***    <p> 
    ***    This is the "exploitation" side of any tree policy 
    ***    used in the classical Monte Carlo Tree Search. It
    ***    chooses the child with the highest expected reward,
    ***    or in the context of Alpha-Beta, the child with the
    ***    highest minimax value so far.
    ***    </p>
    ***    <p>
    ***    When Homura abandons the leftmost policy, it resorts
    ***    to the Greedy policy after visiting each node once.
    ***    </p>
    ***   </li>
    ***  </ol>
    *** </p>
    *** </summary>
    ***
    *** @param i the index to set
    *** @param r the remaining depth
    *** @return the chosen child node
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    inline Node* Node::select
        (
        int& i,             /** Index, To Set   */
        const uint32_t r    /** Remaining Depth */
        )
    {
        /**
         * Initialize the choice and
         * max score.
         */
        Node* choice = nullptr;
        int32_t maxScore = INT32_MIN;

        /**
         * Set the greedy selection 
         * margin.
         */
        const uint32_t margin = r << 1U;

        /**
         * Reset i. i will be the
         * index of the chosen node
         * in the child list.
         */
        i = 0;

        /**
         * Loop through the children
         * of this node.
         */
        foreach_node(x, children, ++i) {

            /**
             * Calculate current bounds.
             */
            x->alpha = std::max(-beta, x->vminus);
            x->beta  = std::min(-alpha, x->vplus);

            /**
             * If the bounds of this
             * child have crossed,
             * skip it.
             */
            if(x->alpha >= x->beta)
                continue;

            /**
             * Use Leftmost Policy in
             * two cases:
             * 
             * 1) If this node is the
             * root node.
             * 
             * 2) If the index of this
             * child in the child list
             * is less than the
             * remaining depth * 2
             * 
             * Otherwise, visit each
             * child once and use the
             * Greedy Policy.
             */
            if(!parent || i < margin || 
                x->score == INT32_MIN)
                return x;

            /**
             * Use Greedy Policy.
             * Choose the child with
             * the highest minimax
             * value so far.
             */
            const int32_t l = -x->score;
            if(l > maxScore) {
                maxScore = l; choice = x;
            }
        }

        /**
         * Return the greedy choice.
         */
        return choice;
    }

     ///////////////////////////////////////////////////////////
    /** 
    *** NODE - BACKPROPAGATE           
    ***
    *** <summary>
    *** <p>
    *** At the end of a rollout, a rollout algorithm must
    *** backpropagate the results of the simulation step to
    *** inform subsequent rollouts. 
    *** </p>
    ***
    *** <p>
    *** Homura backpropagates its bounds in the same way as 
    *** Dr. Huang's Algorithm 4 and backpropagates the score in 
    *** the minimax fashion, ignoring unvisited children.
    *** </p>
    *** </summary>
    ***
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    inline void Node::backprop() 
    {

        /**
         * Initialize variables to
         * hold the max V-, V+
         * and score of this 
         * node's children.
         */
        int32_t maxVMinus = -INT32_MAX, 
                maxVPlus  = -INT32_MAX, 
                maxScore  = -INT32_MAX;
        Node* currentPVNode = nullptr;

        /**
         * Loop through the children
         * of this node.
         */
        foreach_node(x, children) {

            /**
             * Find the max V- and V+.
             */
            maxVMinus = std::max(maxVMinus, -x->vplus);
            maxVPlus  = std::max(maxVPlus, -x->vminus);

            /**
             * Find the current minimax
             * score of this node,
             * ignoring the scores of
             * unvisited children.
             */
            const int32_t l = x->score;
            if(l != INT32_MIN && -l > maxScore) {
                maxScore = -l; currentPVNode = x;
            }
        }

        /**
         * Set V-, V+, and score.
         */
        vminus = maxVMinus;
        vplus = maxVPlus;
        score = maxScore;
        pvNode = currentPVNode;
    }

     ///////////////////////////////////////////////////////////
    /** 
    *** NODE - SIMULATE - QUIESCENCE SEARCH          
    ***
    *** <summary>
    *** <p>
    *** During a rollout, after the selection and expansion 
    *** steps are finished, a rollout algorithm enters the 
    *** simulation step.
    *** </p>
    ***
    *** <p>
    *** When Homura reaches its maximum depth during a rollout, 
    *** it performs a simulation by evaluating with backtracking 
    *** quiescence search.
    *** </p>
    *** </summary>
    ***
    *** @param b the board pointer
    *** @param c the search controls
    *** @return the evaluation
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    template<Alliance A>
    inline int32_t Node::qSearch
        (
        Board* const b,     /** Board             */
        control* const c    /** Search Controls   */
        ) 
    {
        /**
         * Evaluate by Q search,
         * set V-, V+, and the score
         * for backpropagation.
         */
        return 
            vminus = vplus = score = 
            quiescence<A>
            (b, 0, 0, alpha, beta, c);
    }

    template int32_t Node::qSearch<Black>
        (
        Board* const b, 
        control* const c
        );
    
    template int32_t Node::qSearch<White>
        (
        Board* const b, 
        control* const c
        );

     ///////////////////////////////////////////////////////////
    /** 
    *** NODE - INTERNAL ITERATIVE DEEPENING SEARCH          
    ***
    *** <summary>
    *** <p>
    *** If there isn't a pv move in the transposition table
    *** prior to an expansion, and there are a few plies 
    *** remaining, this search will find a good-enough pv move 
    *** via backtracking.
    *** </p>
    *** </summary>
    ***
    *** @param b the board pointer
    *** @param d the depth (ply)
    *** @param r the remaining depth
    *** @param c the search controls
    *** @return the pv move found
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    template<Alliance A>
    Move Node::iidSearch
        (
        Board* const b,     /** Board             */
        const int d,        /** Depth (ply)       */
        const int r,        /** Remaining Depth   */
        control* const c    /** Search Controls   */
        ) 
    {
        /**
         * Clear the iid move for 
         * this ply.
         */
        c->iidMoves[d] = NullMove;

        /**
         * Run a reduced-depth search.
         */
        alphaBeta<A, IID, true>
        (
            b, d, r - 3,
            alpha, beta, c
        );

        /**
         * Return the move found.
         */
        return c->iidMoves[d];
    }

    template Move Node::iidSearch<Black>
        (
        Board* const b, 
        const int d, 
        const int r, 
        control* const c
        );
    
    template Move Node::iidSearch<White>
        (
        Board* const b, 
        const int d, 
        const int r, 
        control* const c
        );

     ///////////////////////////////////////////////////////////
    /** 
    *** SEARCH            
    ***
    *** <summary>
    *** <p>
    *** This function implements a full hybrid Chess search.
    *** </p>
    *** </summary>
    ***
    *** @param b the board pointer
    *** @param info the caller info string to be filled
    *** @param root a pointer to the array of roots
    *** @param gc the garbage collector
    *** @param c the search controls
    *** @param time the time allotted
    *** @return the best move
    *** @author Ellie Moore
    *** @version 05.11.2023
     *//////////////////////////////////////////////////////////

    Move search
        (
        Board *const b,     /** Board              */
        char* const info,   /** Caller Info String */
        Node* &root,        /** Root Array Pointer */
        MemManager& gc,     /** Garbage Collector  */
        control& c,         /** Search Controls    */
        const int time      /** Time Allotted      */
        )
    {
        /**
         * Reset the allocated
         * node count.
         */
        gc.reset();

        // return search(b, info, q, time);
        
        /**
         * Call the worker routine
         * with the correct alliance.
         */
        Move best;
        if(b->currentPlayer() == White) 
            worker<White>(b, root, gc, time, best, c);
        else 
            worker<Black>(b, root, gc, time, best, c);

        /**
         * Fill the "info."
         */
        sprintf
        (
            info, 
            "depth %d nodes %d", 
            c.MAX_DEPTH - 1, 
            gc.getTotal()
        );

        /**
         * Collect the extra root
         * (If needed)
         */
        gc.collect(root + (c.MAX_DEPTH - 1));
        return best;
    }

      /////////////////////////////////////////////////////////
     // MEM MANAGER METHODS
    ///////////////////////////////////////////////////////////

    inline int MemManager::height
        (
        Node* const n
        ) 
    {
        if (n->children.empty())
            return 0;
        int i = 0;
        foreach_node(x, n->children)
            i = std::max(i, height(x));
        return i + 1;
    }

    void MemManager::printPV
        (
        Node* n
        ) 
    {
        while((n = n->pvNode))
            std::cout << n->move << ' ';
    }

    int MemManager::treeWalk
        (
        Node* const n,
        const int depth
        )
    {
        if (n->children.empty()) return 1;
        int c = 0;
        foreach_node(x, n->children) {
            c += treeWalk(x, depth + 1);
        }
        return c + 1;
    }

    int MemManager::treePrint
        (
        Node* const n,
        const int depth
        )
    {
        if (n->children.empty()) return 1;
        int c = 0;
        foreach_node(x, n->children) {
            for (int i = 0; i < depth; ++i)
                std::cout << '\t';
            std::cout 
            << x->move << ": "
            << x->score 
            << " {" << x->alpha << ", " << x->beta << "}, "
            << "{" << x->vminus << ", " << x->vplus << "}"
            << '\n';
            c += treeWalk(x, depth + 1);
        }
        return c + 1;
    }

    void MemManager::destroy
        (
        Node* const n
        )
    {
        Node* x = n->children.begin();
        while(x) {
            destroy(x);
            Node* const y = x; 
            x = x->next;
            delete y;
        }
    }

    void MemManager::purgeCollection()
    {
        lock_guard<mutex> ts(local);
        while(!roots.empty()) {
            Node* x = roots.front(); 
            destroy(x);
            x->disown();
            roots.pop();
        }
        while(!rootFrames.empty()) {
            delete[] rootFrames.front();
            rootFrames.pop();
        }
    }

    inline Node*
    MemManager::alloc
        (
        Node* const p,
        const Move m,
        const TermType t,
        const int32_t s
        )
    {
        ++count;
        return new Node(p, m, t, s);
    }

      /////////////////////////////////////////////////////////
     // NODE LIST METHOD - PUSH BACK - FITS BETTER HERE
    ///////////////////////////////////////////////////////////

    inline void 
    NodeList::push_back
        (
        Node* const n
        ) 
    {
        if(head) 
        { head->next = n; head = n; }
        else head = tail = n;
    }
}
//
// Created by evcmo on 8/12/2021.
//

#pragma once
#ifndef HOMURA_ROLLOUT_H
#define HOMURA_ROLLOUT_H

#include "Backtrack.h"
#include <thread>
#include <queue>

namespace Homura {

    /**
     * The terminal node types, enumerated.
     */
    enum TermType : uint8_t 
    { NOT = 0x00U,  DRAW = 0x02U,  WIN = 0x04U };

    /**
     * The terminal mask to extract a TermType
     * from flag bits.
     */
    constexpr uint8_t  TermMask =    0x06U;    

    /**
     * The maximum number of nodes.
     */
    constexpr uint32_t MaxNodes = 10000000;

    // Using...
    using std::mutex;
    using std::lock_guard;
    using std::ref;
    using std::thread;
    using std::queue;
    using std::this_thread::sleep_for;

#define GET_MACRO(_1, _2, _3, NAME, ...) NAME

    /**
     * Oveloaded for-each macro to easily 
     * iterate through a node list. Needs
     * gcc or clang to work.
     */
#define foreach_node(...)                     \
        GET_MACRO(__VA_ARGS__,                \
        foreach_node3, foreach_node2)         \
        (__VA_ARGS__)                         \

    /**
     * For-each macro to easily iterate
     * through a node list with an extra
     * increment statement.
     */
#define foreach_node3(ID, list, INC)          \
        for                                   \
        (                                     \
            Node* ID = list.tail;             \
            ID != nullptr; ID = ID->next, INC \
        )                                     \

    /**
     * For-each macro to easily iterate
     * through a node list.
     */
#define foreach_node2(ID, list)               \
        for                                   \
        (                                     \
            Node* ID = list.tail;             \
            ID != nullptr; ID = ID->next      \
        )                                     \

    class MemManager;
    class Node;

    /**
     * @class NodeList
     * 
     * <summary>
     * A linked list of Nodes. Each 
     * Node contains a NodeList of 
     * children.
     * </summary>
     * 
     * @author Ellie Moore
     * @version 01.01.2023
     */
    class NodeList final {
    private:
        friend class MemManager;
        friend class Node;

        /**
         * The head pointer of this
         * node list, where new nodes
         * are appended.
         */
        Node* head;

        /**
         * The tail pointer of this
         * node list, where iteration
         * begins.
         */
        Node* tail;
    public:

        /**
         * A public contructor for a
         * node list.
         */
        constexpr NodeList() : 
        head(nullptr), tail(head)
        {  }

        /**
         * A method to append a node
         * to this node list.
         */
        void push_back(Node*);

        /**
         * A method to clear this
         * node list.
         */
        constexpr void clear() 
        { head = tail = nullptr; }

        /**
         * A method to get the tail
         * pointer of this node
         * list, for iteration.
         */
        constexpr Node* begin() 
        { return tail; }

        /**
         * A method to determine
         * whether this node list 
         * is empty.
         */
        [[nodiscard]]
        constexpr bool empty() const
        { return tail == nullptr; }
    };

    /**
     * @class Node
     * 
     * <summary>
     * The Node is the basic modular
     * unit of the search tree built
     * in memory.
     * <summary>
     * 
     * @author Ellie Moore
     * @version 01.01.2023
     */
    class Node final {
    private:
        friend class    NodeList;
        friend class    MemManager;

        /**
         * A list of this Node's children.
         */
        NodeList        children;

        /** 
         * The parent of this node.
         */
        Node*           parent;

        /**
         * The next right sibling.
         */
        Node*           next;

        /**
         * The pv child of this node.
         */
        Node*           pvNode;

        /**
         * Alpha.
         */
        int32_t         alpha;

        /**
         * Beta.
         */
        int32_t         beta;

        /**
         * V-
         */
        int32_t         vminus;

        /**
         * V+
         */
        int32_t         vplus;

        /**
         * The minimax score of this node.
         */
        int32_t         score;

        /**
         * The action taken to reach this
         * node.
         */
        Move            move;

        /**
         * The flag bits for terminal
         * status and re-searching.
         */
        uint8_t         flags;
        
    public:

        /**
         * A public constructor for a Node.
         * 
         * @param p the parent
         * @param m the move
         * @param t whether this node is terminal
         * @param s the score of this node
         */
        constexpr Node(Node* const p,
        const Move m, TermType t, int32_t s) :
        parent(p), next(nullptr), pvNode(nullptr),
        alpha(-INT32_MAX), beta(INT32_MAX), 
        vminus(-INT32_MAX), vplus(INT32_MAX), 
        score(s), move(m), 
        flags(t)
        { }

        /**
         * A default constructor for a Node.
         */
        constexpr Node() :
        parent(nullptr), next(nullptr), pvNode(nullptr),
        alpha(-INT32_MAX), beta(INT32_MAX), 
        vminus(-INT32_MAX), vplus(INT32_MAX), 
        score(INT32_MIN), move(NullMove),
        flags(NOT)
        { }

        /**
         * A method to set the score of this 
         * node, along with bounds V- and V+,
         * for backpropagation.
         * 
         * @param s the score
         */
        constexpr void setScore
        (const int32_t s)
        { score = s; vminus = s; vplus = s; }

        /**
         * A method to expose the current
         * score of this node.
         * 
         * @return the int64_t score of this node
         */
        [[nodiscard]]
        constexpr int32_t getScore() 
        { return score; }

        /*
         * A method to perform a quiescence
         * search with this node as the root.
         */
        template<Alliance A>
        int32_t qSearch
            (
            Board*, 
            control*
            );

        /*
         * A method to perform a non-PV
         * search from this node via
         * backtracking.
         */
        template<Alliance A>
        bool nonPVSearch
            (
            Board*,
            bool, 
            int, 
            int, 
            int,
            control*
            );

        /*
         * A method to perform an internal
         * iterative deepening search via 
         * backtracking from this node, 
         * and return the best move.
         */
        template<Alliance A>
        Move iidSearch
            (
            Board*,
            int, 
            int, 
            control*
            );

        /*
         * A method to expand this node into 
         * its children if memory is available.
         */
        template<Alliance A>
        bool expand
            (
            Board*,
            int,
            int,
            MemManager&,
            control*
            );

        /**
         * A method to indicate whether the main
         * search is re-searching this node via
         * rollout.
         * 
         * @return whether the main search is 
         * re-searching this node via rollout 
         */
        [[nodiscard]]
        constexpr bool reSearch() 
        { return flags & 0x01U; }
        
        /**
         * A method to indicate whether the V-
         * and V+ bounds have met at this node
         * and the search has converged on the
         * principal variation beneath.
         * 
         * @return whether the V- and V+ bounds
         * have crossed at this node.
         */
        [[nodiscard]]
        constexpr bool converged()
        { return vminus >= vplus; }
 
        /**
         * A method to expose this Node's move.
         * 
         * @return this node's move
         */
        [[nodiscard]]
        constexpr Move getMove()
        { return move; }
        
        /**
         * A method to expose this Node's pv
         * move, if one exists.
         * 
         * @return this node's pv move 
         */
        [[nodiscard]]
        constexpr Move getPVMove()
        { return pvNode? pvNode->move: NullMove; }
        
        /**
         * A method to expose alpha.
         * 
         * @return alpha
         */
        [[nodiscard]]
        constexpr int32_t getAlpha()
        { return alpha; }

        /**
         * A method to expose beta.
         * 
         * @return beta 
         */
        [[nodiscard]]
        constexpr int32_t getBeta()
        { return beta; }

        /**
         * A method to update the alpha and
         * beta bounds at the root.
         */
        constexpr void updateAB()
        { 
            alpha = std::max(alpha, vminus); 
            beta  = std::min(beta , vplus ); 
        }
        
        /**
         * A method to expose this node's
         * parent pointer.
         * 
         * @return this Node's parent pointer.
         */
        [[nodiscard]]
        constexpr Node* getParent()
        { return parent; }
        
        /**
         * A method to indicate whether this
         * Node has children.
         * 
         * @return whether this node has children.
         */
        [[nodiscard]]
        constexpr bool hasNoChildren()
        { return children.empty(); }
        
        /**
         * A method to indicate whether this
         * Node is terminal.
         * 
         * @return a TermType indicating whether
         * this node is terminal, and if so, the
         * type of terminality.
         */
        [[nodiscard]]
        constexpr uint8_t terminal() 
        { return flags & TermMask; }
        
        /**
         * A method to disown this node's 
         * children. This method is unsafe.
         * It doesn't care whether the children 
         * have been de-allocated.
         */
        constexpr void disown() 
        { children.clear(); }

        /**
         * A method that defines the tree policy
         * used to select a child at this node.
         * 
         * @return the selected child.
         */
        Node* select(int&, uint32_t);

        /**
         * A method to backpropagate bounds +
         * score from the children into this 
         * Node.
         */
        void backprop();
    };

    /**
     * @class MemManager
     * 
     * <summary>
     * A heap "memory manager" for the search 
     * that collects heap garbage in the 
     * background.
     * </summary>
     * 
     * @author Ellie Moore
     * @version 01.01.2023
     */
    class MemManager final {
    private:

        /**
         * A local mutex to prevent race 
         * conditions around garbage queues.
         */
        mutex        local;

        /**
         * A bool to indicate whether the 
         * garbage collection thread should
         * stop.
         */
        bool         stop;

        /**
         * A currently-allocated node count.
         */
        uint32_t     count;

        /**
         * A queue of root Nodes to collect.
         */
        queue<Node*> roots;

        /**
         * A queue of Node array base 
         * pointers to collect.
         */
        queue<Node*> rootFrames;

        /**
         * A single garbage collection thread.
         */
        thread       gc;

        /**
         * A method to delete the subtree below a node.
         */
        void destroy(Node*);

        /**
         * A method to purge the garbage collector 
         * queues.
         */
        void purgeCollection();
    public:

        /**
         * A method to queue a root for deletion.
         * 
         * @param n the root node
         */
        inline void collect(Node * n) 
        { lock_guard<mutex> ts(local); roots.push(n); }

        /**
         * A method to queue a root array for deletion.
         * 
         * @param n the base pointer to the root array
         */
        inline void collectRoots(Node * n) 
        { lock_guard<mutex> ts(local); rootFrames.push(n); }

        /**
         * A default constructor for the MemManager.
         */
        inline explicit
        MemManager() : 
        stop(false), count(0),
        gc(thread([this]() {
            while(!stop) {
                sleep_for(milliseconds(100));
                purgeCollection();
            }
        })) { }

        /**
         * A method to expose the total current
         * allocated Node count.
         * 
         * @return the currently allocated node count
         */
        [[nodiscard]]
        constexpr uint32_t getTotal() const
        { return count; }

        /**
         * A method to indicate whether the current
         * allocated Node count exceeds the maximum.
         * 
         * @return whether the currently-allocated 
         * Node count exceeds the maximum.
         */
        [[nodiscard]]
        constexpr bool maxNodesExceeded() const
        { return count > MaxNodes; }

        /**
         * A method to allocate a Node, registering it
         * with the MemManager.
         * 
         * @return a pointer to the allocated Node. 
         */
        Node* alloc(Node*, Move, TermType, int32_t);

        /**
         * Static tree utility methods.
         */
        static int height(Node*);
        static int treeWalk(Node*,int);
        static int treePrint(Node*,int);
        static void printPV(Node*);

        /**
         * A method to reset the currently-allocated
         * node count of this MemManager.
         */
        constexpr void reset() { count = 0; }

        /**
         * A public destructor for a MemManager.
         */
        inline ~MemManager() {
            stop = true;
            if(gc.joinable()) gc.join();
            purgeCollection();
        }
    };

    /**
     * Homura's main search function.
     * 
     * <p>
     * This function implements a rollout-based
     * principal variation search where all PV
     * nodes are searched by rollout, and the 
     * remaining nodes are searched via backtracking
     * with a null window.
     * </p>
     * 
     * @return the best move
     */
    Move search(Board*, char*, Node*&, MemManager&, control&, int);
}

#endif
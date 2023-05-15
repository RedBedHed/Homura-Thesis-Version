//
// Created by evcmo on 3/19/2022.
//

#pragma once
#ifndef HELP_ME_ANALYZER_H
#define HELP_ME_ANALYZER_H

#include <iostream>
#include <ostream>
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <fstream>
#include <iomanip>

using
std::stringstream,
std::vector,
std::cin,
std::cout,
std::shared_ptr,
std::make_shared,
std::string,
std::unordered_set,
std::unordered_map,
std::stack,
std::queue,
std::ifstream,
std::setw;

namespace lexer {

    enum ErrorType
    { SYNTAX, SEMANTIC };

    /**
     * A node is the basic unit
     * of a NFA which holds information
     * about outgoing transitions along
     * with pointers to the next nodes,
     * if any.
     */
    struct Node final {
        char l_lab = '\0';
        bool split = false;
        shared_ptr<Node> l = nullptr;
        shared_ptr<Node> r = nullptr;
    };

    /**
     * A NFA is a data structure
     * representing a simple (and
     * suboptimal) non-deterministic
     * finite automata.
     */
    struct NFA final {
        shared_ptr<Node>
            start = nullptr;
        shared_ptr<Node>
            accept = nullptr;
    };

    /**
     * A token packages a lexeme
     * with the name of the language
     * that it belongs to and the line
     * number at which it occurred in
     * the source file.
     */
    struct Token final {
        size_t line_no = 0;
        size_t token = -1;
        string lexeme;
    };

    /**
     * To improve readability.
     */
    class NodeSet final: public
    unordered_set<shared_ptr<Node>> {
    public:
        bool
        contains(const shared_ptr<Node>& n)
        { return find(n) != end(); }
    };

    /**
     * Abbreviations.
     */
    typedef
    stack<shared_ptr<Node>> NodeStack;
    typedef
    vector<shared_ptr<NodeSet>> NodeSetVec;
    typedef
    vector<shared_ptr<Node>> NodeVec;

    /**
     * Class Analyzer
     *
     * @author Ellie Moore
     * @version 05.05.2022
     */
    class Analyzer final {
    private:

        /**
         * A vector to hold the output
         * tokens.
         */
        vector<Token> output;

        /**
         * A vector to hold the NFAs
         */
        vector<NFA> nfas;

        /**
         * A Vector to hold the tokens
         * parsed from the lexical
         * specification.
         */
        vector<string> tokens;

        /**
         * The lexical specification, as
         * a C string.
         */
        const char* input;

        /**
         * The source file, as a C string.
         */
        const char* source;

        /**
         * A vector to hold the Nodes that
         * hold nasty cyclic references.
         * These nodes will need to be
         * dealt with in the Analyzer
         * deconstructor so that RAII
         * can do its job.
         */
        NodeVec loopers;

        /**
         * This is a vector that holds
         * each Node Set representing
         * the epsilon closure of the
         * start state of an NFA (NFA),
         * associated loosely by index.
         */
        NodeSetVec nsv;

        /**
         * This is the current line number
         * within the source string. (E.G.
         * The number of newlines seen so far).
         */
        size_t line_no;

        size_t currentTok;
    public:

        /**
         * @public
         *
         * A public constructor.
         */
        Analyzer();
    private:

        /**
         * <i><b>Parsing function</b></i>
         *
         * <p>
         * This function will parse the
         * lexical specification and transform
         * it into a list of Non-deterministic 
         * Finite Automata.
         * </p>
         *
         * <p>
         * The parser defined in this function
         * is predictive. Its runtime is linear.
         * </p>
         */
        void PARSE();

        /**
         * This is a function to parse a
         * single escape character. Switch
         * is preferred to a map, as a binary
         * search over a handful of elements
         * is likely to be faster.
         *
         * @return the ascii version of the
         * given escape character.
         */
        char ESC(char);

        /**
         * The function A ensures that the
         * first character of an "extended"
         * regular expression is valid before
         * parsing it.
         *
         * @return The NFA representation of
         * the parsed regular expression.
         */
        NFA A();

        /**
         * The function O parses a Token.
         *
         * @return a string representing
         * a single token.
         */
        string O();

        /**
         * The function Z parses a
         * union expression (shorthand,
         * in quotes), returning a NFA
         * for an NFA that recognizes
         * the expression.
         *
         * @return an NFA
         */
        NFA Z();

        /**
         * This function provides an
         * entry point for the parsing
         * of an "extended" regular
         * expression that preserves the
         * correct order of operations.
         * * > . > |
         * E and F work together to
         * parse a union expression.
         *
         * @return an NFA
         */
        NFA E();

        /**
         * A function to parse a
         * concatenation expression
         * in concert with Y.
         *
         * @return an NFA
         */
        NFA D();

        /**
         * A function to parse either
         * a single character, a
         * union shorthand expression,
         * a concatenation shorthand
         * expression, or a nested
         * expression of higher
         * precedence.
         *
         * @return an NFA
         */
        NFA P();

        /**
         * A function to consume the
         * next character in the lexical
         * specification string and verify
         * that it matches the given
         * character. If the characters
         * don't match, this function will
         * throw a syntax error immediately.
         *
         * @param c the character to expect
         */
        void expect(char c);

        /**
         * A function to pop the next char
         * in the lexical specification.
         *
         * @return the next char
         * in the lexical specification.
         */
        char pop();

        /**
         * A function to peek at the next
         * char in the lexical specification,
         * without consuming it.
         *
         * @return the next char
         * in the lexical specification.
         */
        char peek();

        /**
         * A function to pass the next char
         * the lexical specification.
         */
        void pass();

        /**
         * A function to eat whitespace from
         * the lexical specification string.
         */
        void eatSpace();

        /**
         * <b><i>Setup Function</i></b>
         * This function calculates the epsilon
         * closure from the start state and stores
         * each set in a list entry.
         */
        void SETUP();

        /**
         * <b><i>Analyze Function</i></b>
         *
         * <p>
         * This function analyzes the input
         * file and breaks it into it's tokens
         * according to the lexical specification.
         * </p>
         *
         * <p>
         * This function simulates the list of
         * NFAs, calculating the epsilon closure
         * of each state and traversing the states
         * in a set-wise manner.
         * </p>
         */
        void ANALYZE(const string&);

        /**
         * A function to consume all whitespace
         * characters between lexemes in the source
         * file.
         */
        void eatWhiteSpace();

        /**
         * A function to find the next longest
         * match in the source file, adding a
         * token to the output list.
         *
         * @return whether a match was found.
         */
        bool match();

        /**
         * A function to print a syntax error
         * message and promptly exit the program.
         */
        template<ErrorType>
        void error(const char*, int = 0, char = ' ');

        /**
         * A function to create a trivial
         * NFA that recognizes the given
         * character.
         *
         * @return a NFA
         */
        static NFA trivialNFA(char);
        template<char>
        static NFA trivialNFA();

        /**
         * A function to create a NFA
         * that recognizes zero or one
         * of the given NFA.
         *
         * @return a NFA
         */
        static NFA question(const NFA&);

        /**
         * A function to create a NFA
         * that recognizes the kleene
         * closure of the given NFA.
         *
         * @return a NFA
         */
        NFA kleene(const NFA&);

        /**
         * A function to create a NFA
         * that recognizes the kleene
         * plus closure of the given
         * NFA.
         *
         * @return a NFA
         */
        NFA plus(const NFA&);

        /**
         * A function to create a NFA
         * that recognizes the union
         * of the given two NFAs.
         *
         * @return an NFA
         */
        static NFA uni(const NFA&, const NFA&);

        /**
         * A function to create a NFA that
         * recognizes the concatenation of
         * the two given NFAs
         *
         * @return an NFA
         */
        static NFA cat(const NFA&, const NFA&);

        /**
         * A function to find the epsilon
         * closure of the given NFA Node,
         * populating the given set.
         */
        static void closeEpsilon(
        NodeSet&, const shared_ptr<Node>&);

        /**
         * A function to find all transitions
         * on the given character from the
         * Nodes in the appropriate NodeSet.
         * If any such transitions exist, we
         * make them, close epsilon, and
         * populate the appropriate set.
         */
        static void
        matchChar(NodeSet&, NodeSet&, char);

        /** For the next steps. */
        void genENUM();

    public:

        Token nextTok();
        Token peekTok();
        void passTok();

        void loadSpec(const char* spec);
        void nextInput(const string& source);

        /**
         * @public
         *
         * A public destructor.
         */
        ~Analyzer();
    };
}


#endif //HELP_ME_ANALYZER_H

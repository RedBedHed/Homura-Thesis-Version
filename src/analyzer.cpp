//
// Created by evcmo on 3/19/2022.
//

#include "analyzer.h"

namespace lexer {

    Analyzer::
    Analyzer() :
    input(nullptr), source(nullptr), 
    line_no(0), currentTok(-1)
        {  }

    void Analyzer::
    loadSpec(const char* spec) {
        // Open an input
        // file stream for
        // the spec file.
        ifstream is(spec);

        // Get the file
        // as a c-string.
        stringstream u;
        u << is.rdbuf();
        string x = u.str();
        input = x.c_str();

        // Initialize
        // line number.
        line_no = 0;

        // Close the
        // stream.
        is.close();

        // Do the work.
        // First parse
        // and compile
        // the NFAs.
        PARSE();

        // // Next,
        // // loadSpec the
        // // source file
        // // using the
        // // list of NFAs.
        // ANALYZE(s);
    }

    inline void
    Analyzer::PARSE() {

        // go through the lexical
        // spec and parse each
        // 'REGEX # TOKEN' expression.
        // If the specification is
        // valid, these expressions
        // will be separated by
        // commas
        do {

            // Parse the REGEX
            // portion of the
            // expression and
            // add its NFA to
            // the nfas list.
            nfas.push_back(A());

            // Expect the beginning
            // quote of the TOKEN
            // part of the
            // expression.
            expect('\"');

            // Parse the TOKEN
            // portion of the
            // expression and
            // add it to the
            // tokens list.
            tokens.push_back(O());

            // pop the next
            // character from
            // the spec. This
            // char should
            // either be a comma
            // or the EOF
            char c = pop();

            // If we are at
            // the EOF, parsing
            // was successful,
            // and we are done.
            if(c == '\0') break;

            // If the popped
            // character is
            // NOT a comma,
            // throw a syntax
            // error. Otherwise,
            // move on to parse
            // the next expression.
            if(c != ',')
                error<SYNTAX>(
                    "No separator."
                );
        } while(true);
        tokens
        .emplace_back("ERROR");
    }

    NFA Analyzer::E() {

        // Parse D first.
        NFA g = D();

        do {

            // Peek at the
            // next char.
            char c = peek();

            // If the char
            // is not the
            // union
            // operator,
            // return the
            // passed NFA.
            if(c != '|')
                return g;

            // Else pass the
            // peeked char.
            pass();

            // g now
            // recognizes
            // the union
            // of g and the
            // result of a
            // call to D.
            g = uni(g, D());
        } while(true);
    }

    inline NFA Analyzer::D() {

        // Parse P first.
        NFA g = P();

        do {

            // Peek at the
            // next char.
            char c = peek();

            // If we see
            // any of the
            // following
            // chars, then
            // we have nothing
            // to concatenate.
            // return
            // the passed NFA.
            if(c == '|' ||
               c == ')' ||
               c == '#')
                return g;

            // g now
            // recognizes
            // g concatenated
            // with the
            // result of
            // a call to P.
            g = cat(g, P());
        } while(true);
    }

    inline char Analyzer::
    ESC(const char c) {

        // Translate the
        // given character
        // into its ascii
        // escape equivalent
        // and return.
        switch(c) {
            case 't':
                return '\t';
            case 'b':
                return '\b';
            case 'n':
                return '\n';
            case 'r':
                return '\r';
            case 'f':
                return '\f';
            case 's':
                return ' ';
            case '+':
            case '.':
            case '|':
            case ')':
            case '(':
            case '*':
            case '\'':
            case '\"':
            case '\\':
            case ']':
            case '[':
            case '#':
            case '?':
                return c;
            default:
                error<SYNTAX>(
                    "Invalid "
                    "escape."
                );
        }

        // We shouldn't
        // reach here...
        // But this
        // statement
        // is needed
        // to keep g++
        // and clang++
        // happy.
        return '\0';
    }

    inline NFA Analyzer::A() {

        // Fire up a NFA.
        NFA g;

        // Peek at the
        // next char.
        char c = peek();

        // If the next
        // char is any
        // of the
        // enumerated
        // symbols,
        // throw a
        // syntax error.
        // Otherwise,
        // parse a
        // REGEX.
        switch(c) {
            case '|':
            case '*':
            case '?':
            case '+':
            case ')':
            case ']':
            case '#':
                error<SYNTAX>(
                    "Invalid "
                    "leading "
                    "meta-"
                    "character "
                    "for regex "
                    "pattern."
                );
                break;
            default:
                g = E();
        }

        // The next char
        // should be a
        // hashtag.
        expect('#');

        // return the
        // NFA by value.
        // don't worry
        // about the
        // trivial copying.
        return g;
    }

    inline string Analyzer::O() {

        // Fire up a
        // string to hold
        // the token.
        // give it room
        // for 100 to be
        // fast.
        string s;
        s.reserve(100);

        // Pop the next
        // char.
        char c = peek();
        if (c == '\"')
            error<SYNTAX>(
               "Invalid "
               "token "
               "name"
            );
        do {

            // Pop the next
            // char.
            c = pop();

            // If the current
            // char is a quote,
            // then we are
            // done.
            if (c == '\"')
                break;

            // If it is an
            // escape char,
            // you know the
            // drill.
            if (c == '\\')
                error<SYNTAX>(
                   "Invalid "
                   "token "
                   "name"
                );

            // If we have
            // reached the
            // EOF, throw a
            // syntax error.
            if(c == '\0')
                error<SYNTAX>(
                    "Reached "
                    "EOF."
                );

            // push the
            // current
            // char into
            // the token
            // string and
            // repeat.
            s.push_back(c);
        } while(true);
        return s;
    }

    inline NFA Analyzer::Z() {

        // Pop the next
        // char.
        char c = pop();
        if(c == ']')
            error<SYNTAX>(
            "Empty "
            "bracket "
            "expression."
            );

        // Fire up a
        // NFA.
        NFA x;

        // If the
        // current
        // char is a
        // backslash,
        // turn the next
        // char into an
        // escape char.
        if (c == '\\')
            c = ESC(pop());

        // If we reach the
        // EOF, throw a
        // syntax error.
        if (c == '\0')
            error<SYNTAX>(
                "Reached EOF."
            );

        // z holds the
        // previous
        // char.
        char z = c;

        // Make a
        // NFA that
        // recognizes
        // the current
        // char.
        x = trivialNFA(c);
        do {

            // Pop the next
            // char.
            c = pop();

            // If the next
            // character is
            // not -, jump
            // to the pass
            // label and
            // continue
            // processing.
            if(c != '-')
                goto pass;

            // Pop the next
            // char.
            c = pop();

            // If the current
            // char is a right
            // bracket, then
            // we are done.
            if (c == ']')
                return uni(x,
                    trivialNFA<'-'>()
                );

            // If the
            // current
            // char is a
            // backslash,
            // turn the next
            // char into an
            // escape char.
            if (c == '\\')
                c = ESC(pop());

            // If we reach the
            // EOF, throw a
            // syntax error.
            if (c == '\0')
                error<SYNTAX>(
                    "Reached EOF."
                );

            // If the character
            // on the right is
            // less than the
            // character on
            // the left, then
            // the range
            // expression is
            // out of order.
            // We throw a
            // syntax error.
            if (z > c)
                error<SYNTAX>(
                    "Out-of"
                    "-order "
                    "range "
                    "expression."
                );

            // Union the range.
            do {
                x = uni(x,
                    trivialNFA(++z)
                );
            } while (z < c);

            // Pop the next
            // char.
            c = pop();

            // The pass label
            // is where we
            // continue
            // processing.
            pass:

            // If the current
            // char is a right
            // bracket, then
            // we are done.
            if (c == ']')
                break;

            // If the current
            // char is a back-
            // slash, then you
            // know what needs
            // to happen.
            if (c == '\\')
                c = ESC(pop());

            // If we reach the
            // EOF, throw a
            // syntax error.
            if(c == '\0')
                error<SYNTAX>(
                    "Reached EOF."
                );

            // Make a
            // NFA that
            // recognizes
            // the current
            // char and
            // union it
            // with the
            // previous NFA.
            // store the
            // resulting NFA
            // in x. Update
            // z.
            z = c;
            x = uni(x,
                trivialNFA(c)
            );
        } while(true);
        return x;
    }

    inline NFA Analyzer::P() {

        // Fire up a NFA.
        NFA x;

        // Pop the next
        // char.
        char c = pop();
        switch(c) {

            // If the
            // current
            // char is a
            // left bracket,
            // call
            // Z to parse
            // the following
            // union
            // expression.
            case '[' :
                x = Z();
                break;

            // If the
            // current
            // char is a
            // left paren,
            // call E to
            // parse the
            // sub-regular
            // expression
            // with higher
            // precedence.
            case '(' :
                x = E();
                expect(')');
                break;

            // If the
            // current
            // char is a
            // dot, make
            // a NFA that
            // recognizes
            // ASCII - {'\0'}.
            case '.':
            {
                int i = 2;
                x = trivialNFA<1>();
                do {
                    x = uni(x,
                    trivialNFA(
                       (char) i
                    ));
                } while(++i < 256);
            }
                break;

            // If the
            // current
            // char is
            // any of the
            // following,
            // throw a
            // syntax
            // error.
            case '#':
            case ']':
            case '*':
            case '+':
            case '?':
            case '|' :
            case ')' :
                error<SYNTAX>(
                    "Out-of-"
                    "place "
                    "meta-"
                    "character "
                    "in regex "
                    "pattern."
                );
                break;

            // Obviously,
            // we shouldn't
            // be at the EOF.
            case '\0':
                error<SYNTAX>(
                    "Reached "
                    "EOF."
                );

            // If the
            // current
            // char is not
            // an operator
            // make a NFA
            // that
            // recognizes it.
            default:
                if (c == '\\')
                    c = ESC(pop());
                x = trivialNFA(c);
        }

        // Peek at the next
        // char.
        switch(peek()) {

            // If plus,
            // return a  NFA
            // recognizing
            // the kleene
            // plus of the
            // current NFA.
            case '+':
                pass();
                return plus(x);

            // If question mark,
            // return a NFA
            // recognizing the
            // union of the
            // current NFA
            // with the epsilon
            // NFA.
            case '?':
                pass();
                return
                    question(x);

            // If star,
            // return a NFA
            // recognizing
            // the kleene
            // star of the
            // current NFA.
            case '*':
                pass();
                return
                    kleene(x);

            // If no
            // quantifier,
            // return the
            // current NFA.
            default:
                return x;
        }
    }

    inline void
    Analyzer::expect(char c) {

        // Consume the next char.
        // If it matches the
        // argument char, do no
        // more.
        if(pop() == c) return;

        // Else, throw a syntax
        // error.
        error<SYNTAX>("Expected: ", c);
    }

    inline void Analyzer::SETUP() {

        // Iterate through the NFAs.
        for(size_t i = 0;
            i < nfas.size(); ++i) {

            // Fire up a new Node Set
            // on the heap.
            shared_ptr<NodeSet>
            nx = make_shared<NodeSet>();

            // Find the epsilon closure
            // of the start state. Store
            // this set in nx.
            closeEpsilon(*nx, nfas[i].start);

            // If the accepting state is
            // reachable from the start
            // state on epsilon, we have
            // a semantic error.
            if(nx->contains(nfas[i].accept))
                error<SEMANTIC>(
                    "Token cannot be empty",
                    (int) (i + 1)
                );

            // Otherwise, add nx to the
            // global list nsv.
            nsv.push_back(nx);
        }

        // Get leading whitespace
        // out of the way.
        eatWhiteSpace();
    }

    inline void Analyzer::
    ANALYZE(const string& s) {

        // Read the source
        // file into a
        // string.
        // ifstream is(s);
        // stringstream u;
        // u << is.rdbuf();
        // string x = u.str();
        // source = x.c_str();
        // is.close();
        source = s.c_str();
        const char*
            lim = source
               + s.size();

        SETUP();

        // Go through the
        // source from
        // start to finish,
        // continuously
        // finding the
        // next longest
        // match. If no
        // match exists,
        // an error exists,
        // or we have reached
        // the end of the
        // source, then stop.
        while(
            source < lim
            && match()
        );

        // // Print the lexical
        // // analysis (for now).
        // for(Token& t: output)
        //     cout
        //     << setw(14)
        //     << tokens[t.token]
        //     << ", line "
        //     << t.line_no
        //     << " , \""
        //     << t.lexeme
        //     << "\"\n";

        output.push_back({
            line_no + 1,
            tokens.size() - 1,
            "EOF"
        });

        // genENUM();
    }

    inline void
    Analyzer::eatWhiteSpace() {

        // consume whitespace and
        // increment the line number
        // at each new line.
        bool t = false;
        while(
            (t = (*source == '\n')) ||
            *source  == ' '  ||
            *source  == '\t' ||
            *source  == '\r') {
            if(t) {
                ++line_no; t = false;
            }
            ++source;
        }
    }

    inline void Analyzer::matchChar
    (NodeSet& v, NodeSet& q, char c) {

        // Iterate through every Node
        // in v.
        for(const auto& nx: v) {

            // If we can take a
            // transition on the given
            // char, then take it and
            // find the epsilon closure
            // at the new location. Add
            // the nodes in this epsilon
            // closure to q.
            if(nx->l_lab == c)
                closeEpsilon(q,nx->l);
        }
    }

    inline void Analyzer::
    closeEpsilon(NodeSet& v,
    const shared_ptr<Node>& n) {
        if(n == nullptr) return;

        // Fire up a stack.
        NodeStack s;

        // make x point at the
        // Node that n points
        // at.
        shared_ptr<Node> x = n;

        // Push x.
        s.push(x);

        // depth first search from
        // x to find the epsilon
        // closure of n.
        // Make sure to consider
        // only nodes with at
        // least one outgoing
        // non-epsilon transition
        // or the final state of
        // the entire NFA.
        do {
            x = s.top(); s.pop();
            if (x->split) {
                s.push(x->l);
                s.push(x->r);
            } else if(x->l && !x->l_lab)
                s.push(x->l);
            else v.insert(x);
        } while(!s.empty());
    }

    bool Analyzer::match() {
        const size_t
        tokenCount = nfas.size();

        // Set aside a string to store
        // the match lexeme.
        string lexeme;

        // Max character count.
        size_t max = 0,

        // The index of the longest
        // match.
               matchIndex = 0;

        // The index Save (where we
        // return after each match.)
        const char*
        const indexSave = source,
             *indexJump = source;

        // A flag to ignore whitespace
        // within string literals.
        bool ignoreWS = false;

        // Fire up a Queue to hold
        // the characters in a
        // potential match.
        // If the potential
        // match has no path
        // to the final state
        // of the NFA, we will
        // discard these characters
        // and try the next NFA.
        queue<char> queue;

        // Check the source against
        // each NFA.
        for(size_t i = 0;
            i < tokenCount; ++i) {

            // Allocate space for
            // the candidate
            // lexeme.
            string str;

            // reserve room for the
            // candidate lexeme
            // to keep growth from
            // slowing us down.
            str.reserve(100);

            // Restore the saved index.
            source = indexSave;

            // Get a pointer to the
            // epsilon closure set
            // of the initial state
            // of the ith NFA.
            shared_ptr<NodeSet>
                k = nsv[i];

            // Get a pointer to the
            // final state of the
            // ith NFA.
            const shared_ptr<Node>
                a = nfas[i].accept;

            // We won't ignore
            // whitespace while
            // this flag is false.
            // We will instead
            // use it as a
            // delimiter.
            ignoreWS = false;

            // Loop while the current
            // node set is not empty.
            while (!k->empty()) {

                // Instantiate a new node
                // set on the heap.
                const
                shared_ptr<NodeSet>
                t = make_shared<NodeSet>();

                // get the next char from
                // the source.
                const char c = *source++;

                // Ignore whitespace when
                // trying to match a string
                // literal. This is a rushed
                // approach that needs
                // revision.
                if (c == '\"' &&
                    queue.front() != '\\')
                    ignoreWS = !ignoreWS;
                if ((!ignoreWS && c == ' ')
                    || c == '\0')
                    break;

                // Try to match the char.
                // Fill t if possible.
                matchChar(*k, *t, c);

                // Push the char into
                // our queue.
                queue.push(c);

                // If t contains the
                // final state, accept.
                // empty the queue into
                // the ith string.
                // This is a new match
                // lexeme. Note that there
                // may be another, longer
                // match, so we keep
                // looking.
                if (t->contains(a)) {
                    while (!queue.empty()) {
                        str.push_back(
                            queue.front()
                        );
                        queue.pop();
                    }
                }

                // Move to the new set.
                k = t;
            }

            // Clear the queue and reset
            // the source index.
            const size_t s = queue.size();
            if (s) { source -= s; queue = {}; }

            // Update the match index
            // if needed. This is the
            // index of the token for
            // the longest match seen
            // so far.
            const size_t l = str.length();
            if(l > max) {
                matchIndex = i; max = l;
                lexeme = str;

                // Save the next index to
                // be visited upon selecting
                // this match as the longest
                // match.
                indexJump = source;
            }
        }

        // If there were no matches,
        // generate an error token
        // and add it to the output
        // vector.
        if(max == 0) {
            output.push_back({
                line_no,
                tokens.size(),
                "ERROR"
            });
            return false;
        }

        // If there was a match,
        // add it to the output.
        output.push_back({
            line_no, matchIndex,
            lexeme
        });

        // Find the next index to be
        // visited.
        source = indexJump;

        // Skip over whitespace.
        eatWhiteSpace();

        // Will trigger re-entry
        // if we haven't reached
        // EOF.
        return true;
    }

    inline void Analyzer::eatSpace() {
        // Skip whitespace.
        while(*input == ' ' ||
              *input == '\t' ||
              *input == '\n'||
              *input == '\r')
            ++input;
    }

    inline char Analyzer::pop()
    { eatSpace(); return *input++; }

    inline char Analyzer::peek()
    { eatSpace(); return *input; }

    inline void Analyzer::pass()
    { eatSpace(); ++input; }

    constexpr const char* ToString[] =
    { "SYNTAX", "SEMANTIC" };

    template<ErrorType ET>
    inline void Analyzer::
    error(const char* err, int token, char c) {
        cout << ToString[ET]
             << " ERROR in token "
             << (ET == SYNTAX?
                 tokens.size() + 1:
                 token) << ".\n"
             << err << c << '\n';
        exit(0);
    }

    inline NFA Analyzer::
    trivialNFA(const char c) {
        NFA r;
        r.start = make_shared<Node>();
        r.accept = make_shared<Node>();
        r.start->l_lab = c;
        r.start->l = r.accept;
        return r;
    }

    template<char C>
    inline NFA Analyzer::
    trivialNFA() {
        NFA r;
        r.start = make_shared<Node>();
        r.accept = make_shared<Node>();
        r.start->l_lab = C;
        r.start->l = r.accept;
        return r;
    }

    inline NFA Analyzer::
    question(const NFA& x) {
        NFA r;
        r.start = make_shared<Node>();
        r.accept = make_shared<Node>();
        r.start->l = x.start;
        r.start->r = r.accept;
        r.start->split = true;
        x.accept->l = r.accept;
        return r;
    }

    inline NFA Analyzer::
    kleene(const NFA& x) {
        NFA r;
        r.start = make_shared<Node>();
        r.accept = make_shared<Node>();
        r.start->l = x.start;
        r.start->r = r.accept;
        r.start->split = true;
        x.accept->l = r.accept;
        x.accept->r = x.start;
        x.accept->split = true;
        loopers.push_back(x.accept);
        return r;
    }

    inline NFA Analyzer::
    plus(const NFA& x) {
        NFA r;
        r.start = make_shared<Node>();
        r.accept = make_shared<Node>();
        r.start->l = x.start;
        x.accept->l = r.accept;
        x.accept->r = x.start;
        x.accept->split = true;
        loopers.push_back(x.accept);
        return r;
    }

    inline NFA Analyzer::
    uni(const NFA& x, const NFA& y) {
        NFA r;
        r.start = make_shared<Node>();
        r.accept = make_shared<Node>();
        r.start->l = x.start;
        r.start->r = y.start;
        r.start->split = true;
        x.accept->l = r.accept;
        y.accept->l = r.accept;
        return r;
    }

    inline NFA Analyzer::
    cat(const NFA& x, const NFA& y) {
        NFA r;
        r.start = x.start;
        r.accept = y.accept;
        x.accept->l = y.start;
        return r;
    }

    void Analyzer::genENUM() {
        cout <<
        "enum Tokens : uint32_t {\n";
        for(size_t i= 0;;) {
            cout << '\t' << tokens[i];
            if(++i >= tokens.size())
                break;
            cout << ", \n";
        }
        cout << "\n}\n";
    }

    Token Analyzer::nextTok() {
        return output[++currentTok];
    }

    Token Analyzer::peekTok() {
        return output[currentTok + 1];
    }

    void Analyzer::passTok() {
        ++currentTok;
    }

    void Analyzer::nextInput(const string& s) {
        ANALYZE(s);
    }

    Analyzer::~Analyzer() {
        
        // Break cyclic references to
        // allow garbage to be collected.
        for(shared_ptr<Node>& l: loopers)
            l->r = nullptr;
    }
}

// int main(int argc, char** argv) {
//     if(argc < 2) return 0;
//     lexer::
//     Analyzer a;
//     a.loadSpec(argv[1], argv[2]);
// }
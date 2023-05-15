#include "Fen.h"
#include <ostream>
#include <iostream>
#include <time.h>
#include "Rollout.h"
#include "analyzer.h"
#include "Board.h"
#include<unordered_map>

using namespace Homura;
using namespace lexer;
using std::unordered_map;
using std::cout;

/*
 * This file contains the main method. It parses 
 * and handles a subset of UCI inputs. It is very
 * inefficient in places, and the code quality is very
 * poor. I need to just re-write this file completely.
 * However, I don't have the time to do that for this
 * "release."
 * 
 * I will definitely do so in my next engine.
 */

enum Tokens : uint32_t 
{
    UCI,
    DEBUG,
    ISREADY,
    SETOPTION,
    NAME,
    REGISTER,
    UCINEW,
    POSITION,
    STARTPOS,
    MOVES,
    GO,
    SEARCHMOVES,
    PONDER,
    WTIME,
    BTIME,
    WINC,
    BINC,
    MOVESTOGO,
    DEPTH,
    NODES,
    MATE,
    MOVETIME,
    INFINITE,
    STOP,
    PONDERHIT,
    QUIT,
    FEN,
    ALLIANCE,
    DASH,
    CRIGHTS,
    NUM,
    BOARD,
    LITERAL,
    _EOF,
    ERROR
};

void init_move_map
    (
    unordered_map<string, Move>& moveMap
    ) 
{
    for(int j = 0; j < 64; ++j) {
        for(int i = 0; i < 64; ++i) {
            string s = SquareToString[j]; 
            s.append(SquareToString[i]);
            // std::cout << s << '\n';
            moveMap[s] = Move::make(j, i);
        }
    }
}

void tryParseStartPos
    (
    Analyzer& a, 
    Board* const b,
    State*& ss,
    MemManager& gc,
    unordered_map<string, Move> moveMap
    ) 
{
    Token t;
    if((t = a.peekTok()).token != STARTPOS) {
        cout << "invalid position arg: " << t.lexeme << '\n';
        return;
    }
    a.nextTok();
    if((t = a.peekTok()).token != MOVES) {
        if(t.token != _EOF)
            cout << "invalid position arg: " << t.lexeme << '\n';
        return;
    }
    a.nextTok();
    while(a.peekTok().token == LITERAL)
        t = a.nextTok();
    if(t.token != LITERAL) {
        cout << "invalid position arg: " << t.lexeme << '\n';
        return;
    }
    Move mv;
    if(t.lexeme.size() > 4) {
        uint16_t i = 0;
        switch(t.lexeme[4]) {
            case 'q':
                i = (Queen - Rook) << 12U;
                break;
            case 'n':
                i = (Knight - Rook) << 12U;
                break;
            case 'r':
                i = 0;
                break;
            case 'b':
                i = (Bishop - Rook) << 12U;
                break;
        }
        mv = Move(moveMap[t.lexeme.substr(0, 4)].getManifest() | i | 0x8000U);
    } else mv = moveMap[t.lexeme];
    MoveList<MCTS> ml(b);
    Move* k = ml.begin();
    Move* e = ml.end();
    for(; k < e; ++k) {
        if(mv.origin() != 
            (*k).origin() ||
           mv.destination() != 
            (*k).destination() ||
            (t.lexeme.size() > 4 && 
            mv.promotionPiece() != 
                (*k).promotionPiece())) 
            continue;
        b->applyMove(*k, *ss++);
        break;
    }
    return;
}

void handleGo
    (
    Board& b,
    Analyzer& a, 
    Token& t, 
    char* info,
    State*& ss,
    MemManager& gc,
    control& q
    ) 
{
    int time = 5000;
    if(a.peekTok().token != _EOF) {
        t = a.nextTok();
        switch(t.token) {
        case MOVETIME:
            t = a.nextTok();
            time = atoi(t.lexeme.c_str());
            break;
        case INFINITE: // infinite gives Homura five seconds. 
            break;
        default:
            cout << "invalid go arg: " << t.lexeme << '\n';
            break;
        }
    }
    Homura::Node* n = new Homura::Node[65];
    Move m = search(&b, info, n, gc, q, time); 
    cout << "info " << info << '\n';         
    b.applyMove(m, *ss++);
    cout << "bestmove " 
         << SquareToString[m.origin()] 
         << SquareToString[m.destination()];
    if(m.isPromotion()) {
        switch(m.promotionPiece()) {
            case Queen:
                cout << 'q';
                break;
            case Bishop:
                cout << 'b';
                break;
            case Knight:
                cout << 'n';
                break;
            case Rook:
                cout << 'r';
                break;
        }
    }
    cout << '\n';
    gc.collectRoots(n);
}

int main() 
{
    Witchcraft::init();
    Zobrist::init();
    unordered_map<string, Move> moveMap;
    init_move_map(moveMap);
    State state, stack[512], *ss = stack;
    char info[500];
    Board b = Board::Builder<Default>(state).build();
    lexer::Analyzer a;
    a.loadSpec("ospec.txt");
    control q;
    MemManager gc;
    while(true) {  
        string s;
        getline(cin, s, '\n');
        a.nextInput(s);
        Token t = a.nextTok();
        switch(t.token) {
        case STOP: break;
        case QUIT: case ERROR: goto out; 
        case UCI:
            cout << "id name Homura\n";
            cout << "id author Ellie Moore\n";
            cout << "uciok\n";
            break;
        case ISREADY:
            cout << "readyok\n";
            break;
        case UCINEW:
            gc.reset();
            b = Board::Builder<Default>(state).build();
            Zobrist::reset();
            ss = stack;
            q.clearHistory();
            break;
        case POSITION:
            tryParseStartPos(a, &b, ss, gc, moveMap);
            break;
        case GO: 
            handleGo(b, a, t, info, ss, gc, q);
            break;
        case BOARD:
            cout << "here:\n" << b << '\n';
            break;
        case _EOF:
            cout << "no cmd\n";
            continue;
        default:
            cout << "unknown cmd: " << t.lexeme << '\n';
        }
        while(a.nextTok().token != _EOF);
    }
    out:
    cout << "done" << '\n';
    Zobrist::destroy();
    Witchcraft::destroy();
    return 0;
}
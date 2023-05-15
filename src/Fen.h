//
// Created by evcmo on 8/3/2021.
//

#pragma once
#ifndef HOMURA_FEN_H
#define HOMURA_FEN_H

#include "Board.h"

namespace Homura::FenUtility {

    constexpr const char *PieceToChar = "PRNBQK";

    constexpr uint8_t AlgebraicNotationToSquare[][8] = {
            { A1, A2, A3, A4, A5, A6, A7, A8 },
            { B1, B2, B3, B4, B5, B6, B7, B8 },
            { C1, C2, C3, C4, C5, C6, C7, C8 },
            { D1, D2, D3, D4, D5, D6, D7, D8 },
            { E1, E2, E3, E4, E5, E6, E7, E8 },
            { F1, F2, F3, F4, F5, F6, F7, F8 },
            { G1, G2, G3, G4, G5, G6, G7, G8 },
            { H1, H2, H3, H4, H5, H6, H7, H8 },
    };

    constexpr int find(const char c) {
        for (int i = 0; PieceToChar[i] != '\0'; ++i)
            if (PieceToChar[i] == c) return i;
        return -1;
    }

    constexpr bool isLowerCase(const char c)
    { return c > '`' && c < '{'; }

    constexpr Board
    parseBoard(const char *const fen,
               State *const x) {
        Board::Builder<Fen> b(*x);
        const char *c = fen;
        for (int sq = A8; sq >= H1; ++c) {
            for (; *c != '/' && *c != ' '; ++c) {
                if (*c > '0' && *c < '9')
                    sq -= (*c - '0');
                else {
                    const auto a =
                        Alliance(isLowerCase(*c));
                    b.setPiece(a,
                        PieceType(find(
                        (char)(*c - (a? 32: 0)))),
                        sq--
                    );
                }
            }
        }
        const char a = *c;
        b.setCurrentPlayer(*c); c += 2;
        if (*c != '-') {
            for (; *c != ' '; ++c)
                b.setCastlingRights<true>(*c);
            ++c;
        } else c += 2;
        if (*c != '-')
            b.setEnPassantSquare((Square)
                (AlgebraicNotationToSquare
                [*c - 'a']
                [*(c + 1) - '1'] +
                (a == 'w'? -8 : 8))
            );

        return b.build();
    }
}


#endif //HOMURA_FEN_H

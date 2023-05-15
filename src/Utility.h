//
// Created by evcmo on 6/27/2021.
//

#pragma once
#ifndef HOMURA_UTILITY_H
#define HOMURA_UTILITY_H


namespace Homura {
    constexpr int32_t BoardLength = 64;
    constexpr int64_t  MateValue = INT16_MAX;

    /**
     * The minimum possible mate value (we shouldn't
     * ever be searching past 100 plies... If we do
     * there is a serious problem).
     */
    constexpr int64_t MinMate = MateValue - 100;

    /**
     * The maximum depth that the main search is 
     * allowed to reach (assuming advanced aliens
     * who never die run this algorithm on an actual 
     * turing machine).
     */
    constexpr int32_t MaxDepth = 65;
};


#endif //HOMURA_UTILITY_H

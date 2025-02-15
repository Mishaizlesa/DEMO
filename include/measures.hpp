#include "types.hpp"

#ifndef MEASURES_HPP
#define MEASURES_HPP

inline fp default_measure(vector v) {
    if (v.size() == 0) {
        exit(-1);
    }
    fp max = v[0];
    for (auto x : v)
        if (x > max)
            max = x;

    return max;
}

inline fp Euclid_measure(vector v) {
    fp r = 0.;
    for(auto x : v) {
        r += x*x;
    }
    return sqrt(r);
}

#endif // MEASURES_HPP
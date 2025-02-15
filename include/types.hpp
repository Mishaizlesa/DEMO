#include <cmath>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <functional>
#include <algorithm>

#ifndef TYPES_HPP
#define TYPES_HPP

#define FP_64

#ifdef FP_64
typedef double fp;
typedef unsigned int uint;
constexpr fp default_inner_eps = 1e-12;
constexpr fp default_border_eps = 1e-12;
inline const fp cuberoot(const fp x) {
    return std::cbrt(x);
}
#else
typedef float fp;
constexpr fp default_inner_eps = 1e-3;
constexpr fp default_border_eps = 1e-4;
inline const fp cuberoot(const fp x) {
    return std::cbrtf(x);
}
#endif
#define IS_ALMOST_EQUAL(F1, F2, EPSILON) ( (((F1) - (F2)) < EPSILON) && (((F2) - (F1)) < EPSILON) )
#define UNUSED(x) (void)(x)
// typedef size_t uint;
enum State {
    OUTER = 0,
    INNER = 1,
    BOUND = 2
};
typedef fp time_type;
typedef std::pair<fp, fp> bounds;
typedef std::vector<fp> vector;
typedef std::vector<vector> matrix;
typedef std::vector<std::vector<State>> state_map;
typedef vector(*FUNCTION_TYPE)(fp, vector);
typedef std::function<fp(fp)> FUNCTION_X_TYPE;
typedef std::function<fp(fp, fp)> FUNCTION_XY_TYPE;
typedef std::function<fp(fp, fp, fp)> FUNCTION_XYT_TYPE;
typedef std::function<fp(fp, fp)> FUNCTION_XT_TYPE;
typedef std::function<fp(fp, fp)> FUNCTION_YT_TYPE;
typedef fp(*MEASURE_TYPE)(vector);
#define half(x) ((x) / 2)
constexpr int NMAX = 3000;


inline const vector operator*(vector X, fp val) {
    vector A(X);
    for(auto& v : A) {
        v *= val;
    }
    return A;
};
inline const vector operator*(fp val, vector X) { return X * val; };
inline const vector operator/(vector X, fp val) { return X * (1 / val); };
inline const vector operator+(const vector& a, const vector& b) {
    if (a.size() != b.size()) {
        exit(-1);
    }
    vector A(a);
    for(uint i = 0; i < A.size(); i++) {
        A[i] += b[i];
    }
    return A;
}
inline const vector operator-(const vector& a) {
    vector A(a);
    for(uint i = 0; i < A.size(); i++) {
        A[i] = -A[i];
    }
    return A;
}
inline const vector operator-(const vector& a, const vector& b) {
    return a + (-b);
}
inline const vector abs(const vector& a) {
    vector A(a);
    for(auto& v : A) {
        v = std::abs(v);
    }
    return A;
}
inline std::ostream& operator<<(std::ostream& out, const vector& a) {
    out << "[";
    for(uint i = 0;i < a.size() - 1; i++) {
        out << a[i] << ", ";
    }
    if (a.size() != 0) {
        out << a.back();
    }
    out << "]";
    return out;
}


struct __output_params {
    fp t;
    vector X;
    vector X2;
    fp diff;
    fp OLP;
    fp step;
    uint step_division_counter;
    bool step_multiply_counter;
};
struct __input_params {
    bounds time;
    vector X0;
    uint Nmax;
    fp epsilon_right_border;
    fp first_step_size;
    fp epsilon_required;
};
typedef std::vector<__output_params> output_params;
inline void expand(output_params& params) {
    params.push_back(__output_params());
}
inline void shrink_to_fit(output_params& params) {
    params.shrink_to_fit();
}

inline vector dot(const matrix & A, const vector & b) {
    uint n = A.size();
    uint m = b.size();
    vector x(n);
    for (uint i=0; i<n; i++) {
        for (uint j=0; j<m; j++) {
            x[i] += A[i][j] * b[j];
        }
    }
    return x;
}

#endif // TYPES_HPP

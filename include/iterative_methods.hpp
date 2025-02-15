#ifndef ITERATIVE_METHODS_HPP
#define ITERATIVE_METHODS_HPP

#include "types.hpp"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#define MAX_COUNT_ITER 10000
#define DEFAULT_EPS fp(1e-7)
#define DEBUG_ITERATIVE 0

#if DEBUG_ITERATIVE
#define DBG(args...) printf(args)
#else
#define DBG(arg...) ((void)0)
#endif

class IterativeMethod {
protected:
    const fp EPS_REQUIRED;
    const uint ITER_MAX;

    void setCountIter(uint iter) { count_iter = iter; }
    void setErrorEstimates(fp eps) { error_estimates = eps; }

public:
    fp inf_lag;
    fp error_estimates;
    uint count_iter;
    IterativeMethod(fp eps_required=DEFAULT_EPS, uint iter_max=MAX_COUNT_ITER) : 
        EPS_REQUIRED(eps_required), 
        ITER_MAX(iter_max),
        count_iter(std::numeric_limits<uint>::max()),
        error_estimates(std::numeric_limits<fp>::max())
        {}

    void calculate_residual_norm2(const matrix & v, const matrix & f, fp h, fp k);

    virtual void run(fp* x, const fp* b, const fp* A, uint32_t n)  = 0;
    virtual void run2d(fp* x, fp* up, fp* down, const fp* b, const fp* A, uint32_t n, uint32_t n_) =0;

    void print();
};

inline void IterativeMethod::print() {
    printf("Начальное условие (0, 0, 0, 0)\n");
    printf("Критерии остановки: \n");
    printf("\tNmax = %d\n", ITER_MAX);
    printf("\tТочность eps = %.17lf\n", EPS_REQUIRED);
    printf("Выполнено N = %d шагов\n", count_iter);
    printf("Точность на выходе eps_N = %.17lf\n", error_estimates);
}

#endif
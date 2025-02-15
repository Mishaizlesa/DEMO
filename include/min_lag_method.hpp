#include "types.hpp"
#include "measures.hpp"
#include "iterative_methods.hpp"

//#define MVR_ITER_MAX 100000

const uint64_t ITER_MAX = 500;

const double EPS_REQUIRED = 1e-9;



#ifdef __cplusplus
extern "C" {
#endif

void run(fp* x, const fp* b, const fp* A, uint32_t n);
int run2d(fp* x, fp* up, fp* down, const fp* b, const fp* A, uint32_t n, uint32_t n_);

#ifdef __cplusplus
}
#endif
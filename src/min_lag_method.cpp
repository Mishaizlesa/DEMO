// min_lag_method.cpp
#include "min_lag_method.hpp"
#include <cblas.h>

void run(
    fp* v,
    const fp* b,
    const fp* A,
    uint32_t n
) {
    uint iter = 0;
    fp eps = 0.;
    fp* r =(fp*)malloc(sizeof(fp) * (n));
    fp* Ar = (fp*)malloc(sizeof(fp) * (n));
    //inf_lag = 0.0;
    for(; iter < ITER_MAX; iter++) {
        memcpy(r, b, sizeof(fp) * (n));
        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n, n, 1, 1, 1.0, A, 3, v, 1, -1.0, r, 1);
        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n, n, 1, 1, 1.0, A, 3, r, 1, 0.0, Ar, 1); //matrix Ar = A_mult(r, hsq_inverse, ksq_inverse, mA, S);
        fp Ar_sq = cblas_ddot(n, Ar, 1, Ar, 1);
        fp Ar_r = cblas_ddot(n, r, 1, Ar, 1);
        fp tau = Ar_r/Ar_sq;
	printf("%lf\n", tau);
        eps = tau * (cblas_damax(n, r, 1));
	
        cblas_daxpy(n, -tau, r, 1, v, 1);
	
        if (fabs(eps) < EPS_REQUIRED) {
            iter++;
            break;
        }
    }
    free(Ar);
    free(r);
}
int run2d( fp* v, fp* up, fp* down, const fp* b, const fp* A, uint32_t n, uint32_t n_) {
    int iter = 0;
    fp eps = 0.;
    fp* r = new fp[n]{};
    fp* Ar = new fp[n]{};
    
    //inf_lag = 0.0;
    for(; iter < ITER_MAX; iter++) {
        memcpy(r, b, sizeof(fp) * (n));
        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n, n, 1, 1, 1.0, A, 3, v, 1, -1.0, r, 1);


        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n-n_, n-n_, 0, 0, 1.0, up, 1, &v[n_], 1, 1.0, r, 1);
        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n-n_, n-n_, 0, 0, 1.0, down, 1, &v[0], 1, 1.0, &r[n_], 1);

        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n, n, 1, 1, 1.0, A, 3, r, 1, 0.0, Ar, 1); //matrix Ar = A_mult(r, hsq_inverse, ksq_inverse, mA, S);

        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n-n_, n-n_, 0, 0, 1.0, up, 1, &r[n_], 1, 1.0, Ar, 1);
        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n-n_, n-n_, 0, 0, 1.0, down, 1, &r[0], 1, 1.0, &Ar[n_], 1);

        fp Ar_sq = cblas_ddot(n, Ar, 1, Ar, 1);
        fp Ar_r = cblas_ddot(n, r, 1, Ar, 1);
        fp tau = Ar_r/Ar_sq;


        eps = tau * (cblas_damax(n, r, 1));

        cblas_daxpy(n, -tau, r, 1, v, 1);

        if (fabs(eps) < EPS_REQUIRED) {
            iter++;
            break;
        }
    }
    delete[] Ar;
    delete[] r;
    return iter;
}
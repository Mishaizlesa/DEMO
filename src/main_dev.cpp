#include "problems.hpp"
#include "min_lag_method.hpp"
#include <cblas.h>
#include <cstdlib>

/*void minl() {
    fp eps = 1e-9;
    uint Nmax = 1000000;
 //   IterativeMethod* meth=  new MinLagMethod(eps, Nmax);
    for (int n=200; n<=3200;n*=2) test_problem(n, 2000, 1.0, 1.0);
}*/

void minl2d() {
    fp eps = 1e-9;
    uint Nmax = 40;
//    IterativeMethod* meth=  new MinLagMethod(eps, Nmax);

    for (int n=75; n<=75;n*=2) test_problem2d(n, 1000, n, 5.0, 1);
    //problem1(n, m, 1.0, 1.0, meth);
}


/*void system_check(){

    fp eps = 1e-15;
    uint Nmax = 10000;

    IterativeMethod* meth=  new MinLagMethod(eps, Nmax);
    fp A[] = {3,-1,0,0,
              -1,3,-1,0,
              0,-1,3,-1,
              0,0,-1,3};

    fp up[2] = {-1, -1};
    fp down[2] = {-1, -1};
    fp a[3 * 4];
    fp b[4] = {5,4,3,2};
    fp* x = (fp*)malloc(4*sizeof(fp));

    int kl = 1;
    int ku = 1;
    int m = 4;
    int n = 4;
    int lda = 3;
    int ldm = m;
    for (int i = 0; i < m; i++) {
        int k = kl - i;
        for (int j = std::max(0, i - kl); j < std::min(n, i + ku + 1); j++) {
            a[(k + j) + i * lda] = A[j + i * ldm];
        }
    }
    memset(x, 0, 4*sizeof(fp));


    //cblas_dgbmv(CblasRowMajor, CblasNoTrans, 4, 4, 1, 1, 1.0, a, 4, b, 1, 0.0, x, 1);

    meth->run(x, b, a, 4);
    for (int i=0;i<4;++i) printf("%lf ", x[i]);

    free(x);


}

void mult_bench(uint32_t n){

    fp* a = (fp*)malloc(n * 3 * sizeof(fp));
    fp* v = (fp*)malloc(n * sizeof(fp));
    srand(time(NULL));
    int kl = 1;
    int ku = 1;
    int n_ = n;
    int m_ = n;
    int lda = 3;
    int ldm = n_;

    for (int i = 0; i < m_; i++) {
    int k_ = kl - i;
        for (int j = std::max(0, i - kl); j < std::min(int(n_), i + ku + 1); j++) {
            //a[(k_ + j) + i * lda] = A[j + i * ldm];
            if ((j) == i) a[(k_ + j) + i * lda] = (fp)rand()/RAND_MAX;
            else if (j == i - 1 && i%(n-1)) a[(k_ + j) + i * lda] = (fp)rand()/RAND_MAX;
            else if (j== (+1) && i%(n-1)!=(n-2)) a[(k_ + j) + i * lda] = (fp)rand()/RAND_MAX;
        }
    }

    double mint = 1e15;
    for(int kk=0;kk<3;++kk){
	auto start = std::chrono::system_clock::now();

        cblas_dgbmv(CblasRowMajor, CblasNoTrans, n, n, 1, 1, 1.0, a, 3, v, 1, 0.0, v, 1);
        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (elapsed.count()<mint) mint = elapsed.count();
    }
    printf("time elapsed = ");std::cout << mint << '\n';


}

*/
int main() 
{

    //std::cin>>n>>m>>k;
    //system_check();
    minl2d();

    //for (int n = 200000; n <=32000000; n*=2) mult_bench(n);
    //return 0;
}

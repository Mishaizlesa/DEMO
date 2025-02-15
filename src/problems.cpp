#include "problems.hpp"
#include "wave.hpp"

FILE* output1 = fopen("tmp1.txt", "w+");
FILE* output2 = fopen("tmp2.txt", "w+");
 
const double pi = 3.14159265359;

const double sqrt2 = 1.41421356237;

/*void test_problem(uint n, uint m, fp T, fp a_sq, IterativeMethod* method) {

    const auto sol = [](fp x, fp t) -> fp { return std::sin(x+t)+x*(t-std::sin(t)); };

    const auto phi1 = [&](fp x) -> fp { return std::sin(x); };
    const auto phi2 = [&](fp x) -> fp { return  std::cos(x); };
    const auto xi1 = [&](fp t) -> fp { return std::sin(t); };
    const auto xi2 = [&](fp t) -> fp { return std::sin(1.0+t)+(t-std::sin(t)); };

    const auto f = [](fp x, fp t) -> fp { return x*std::sin(t); };


    matrix V; 
    auto start = std::chrono::system_clock::now();
    double mint = 1e15;
    for(int k=0;k<3;++k){
    solve_wave(V, T, 
                  n, m, a_sq,
                  f, 
                  phi1, phi2, xi1, xi2, method);
    auto end = std::chrono::system_clock::now();
    auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (elapsed.count()<mint) mint = elapsed.count();
    }
    printf("time elapsed = ");std::cout << mint << '\n';
    fprintf(output1, "%d ", method->count_iter);
    fprintf(output1, "%.17lf ", method->error_estimates);
    method->count_iter = 0;
    method->error_estimates = 0;

    fp max_diff1 = 0.;
    fp h = 1.0 / (fp)n;
    fp k = T / (fp)m;
    fp mx, mt;
    for (uint j = 1; j <= m; j++) {
        for (uint i = 1; i < n; i++) {
            fp _x = i * h;
            fp _t = j * k;
            fp diff = std::abs(sol(_x,_t) - V[j][i]);
            if (diff > max_diff1) {
                max_diff1 = diff;
                mx = _x;
                mt = _t;
            }
        }
    }

    printf("err = %.17lf\n", max_diff1);
    printf("%lf %lf\n", mx, mt);
    fprintf(output1, "%d ", method->count_iter);
    fprintf(output1, "%.17lf ", method->error_estimates);
    fprintf(output1, "%.17lf ", max_diff1); //"Погрешность решения СЛАУ по норме1 
    fprintf(output1, "%.17lf %.17lf ", mx, mt);//"Максимальная погрешность достигается в x = %.17lf, y = %.17lf\n"
    fprintf(output1, "%.17lf ", method->inf_lag);


   fprintf(output2, "i j x t V(x,t) V2(x,t) |V(x,t)-V2(x,t)|\n");
    for (uint j = 0; j <= m; j++) {
        for (uint i = 0; i <= n; i++) {
            fp _x = i * h;
            fp _t = j * k;
            fp diff = std::abs(sol(_x,_t) - V[j][i]);
            fprintf(output2, "%d %d %.10lf %.10lf %.10lf %.10lf %.10lf\n", i, j, _x, _t, V[j][i], sol(_x,_t), diff);
        }
    }

    
}


void problem1(uint n, uint m, fp T, fp a_sq, IterativeMethod* method) {

    const auto phi1 = [&](fp x) -> fp { return std::sin(2*3.14159265359*x); };
    const auto phi2 = [&](fp x) -> fp { return  std::cos(x); };
    const auto xi1 = [&](fp t) -> fp { return 0; };
    const auto xi2 = [&](fp t) -> fp { return 0; };

    const auto f = [](fp x, fp t) -> fp { return x*x + t; };

    const auto phi1 = [&](fp x) -> fp { return 0; };
    const auto phi2 = [&](fp x) -> fp { return 0; };
    const auto xi1 = [&](fp t) -> fp { return 0; };
    const auto xi2 = [&](fp t) -> fp { return 0; };

    const auto f = [](fp x, fp t) -> fp { return x + t; };


    matrix V; 
    matrix V2;

    

    //V = (fp*)malloc(sizeof(fp)*(m + 1)*(n + 1));
    //memset(V, 0, sizeof(fp)*(m + 1)*(n + 1));
    auto start = std::chrono::system_clock::now();
    solve_wave(V, T, 
                  n, m, a_sq,
                  f, 
                  phi1, phi2, xi1, xi2, method);
    //printf("%lf\n", V[2]);
    fprintf(output1, "%d ", method->count_iter);
    fprintf(output1, "%.17lf ", method->error_estimates);
    method->count_iter = 0;
    method->error_estimates = 0;


    //V2 = (fp*)malloc(sizeof(fp)*(2 *m + 1)*(2*n + 1));
   // memset(V2, 0, sizeof(fp)*(2*m + 1)*(2*n + 1));


    solve_wave(V2, T, 
                  2 * n, 2 * m, a_sq,
                  f, 
                  phi1, phi2, xi1, xi2, method);
    fp max_diff1 = 0.;
    fp h = 1.0 / (fp)n;
    fp k = T / (fp)m;
    fp mx, mt;
    for (uint j = 1; j <= m; j++) {
        for (uint i = 1; i < n; i++) {
            fp _x = i * h;
            fp _t = j * k;
            //printf("))\n");
            fp diff = std::abs(V2[j*2][i*2] - V[j][i]);
           // printf("%lf %lf\n", V2[j*2][i*2], V[i][j]);
            if (diff > max_diff1) {
                max_diff1 = diff;
                mx = _x;
                mt = _t;
            }
        }
    }

    printf("err = %.17lf\n", max_diff1);
    printf("%lf %lf\n", mx, mt);
    fprintf(output1, "%d ", method->count_iter);
    fprintf(output1, "%.17lf ", method->error_estimates);
    fprintf(output1, "%.17lf ", max_diff1); //"Погрешность решения СЛАУ по норме1 
    fprintf(output1, "%.17lf %.17lf ", mx, mt);//"Максимальная погрешность достигается в x = %.17lf, y = %.17lf\n"
    fprintf(output1, "%.17lf ", method->inf_lag);


   fprintf(output2, "i j x t V(x,t) V2(x,t) |V(x,t)-V2(x,t)|\n");
    for (uint i = 0; i <= n; i++) {
        for (uint j = 0; j <= m; j++) {
            fp _x = i * h;
            fp _t = j * k;
            fp diff = std::abs(V2[j*2][i*2] - V[j][i]);
            fprintf(output2, "%d %d %.10lf %.10lf %.10lf %.10lf %.10lf\n", i, j, _x, _t, V[j][i], V2[j*2][i*2], diff);
        }
    }
    //free(V);
    //free(V2);
}
*/

void test_problem2d(uint n, uint m, uint k, fp T, fp a_sq){


    const auto phi1 = [&](fp x, fp y) -> fp { return std::exp(0.1*(x+y)); };
    const auto phi2 = [&](fp x, fp y) -> fp { return 2; };
    const auto xi1 = [&](fp y, fp t) -> fp { return 0; };
    const auto xi2 = [&](fp y, fp t) -> fp { return 0; };
    const auto xi3 = [&](fp x, fp t) -> fp { return 0; };
    const auto xi4 = [&](fp x, fp t) -> fp { return 0; };

    const auto f = [](fp x, fp y, fp t) -> fp { return x+y*t; };

    a_sq = 4;



    matrix V; 
    run_kernel(V, T, 
                    n, m * 10, k, a_sq,
                    f, 
                    phi1, phi2, xi1, xi2, xi3, xi4);

}


#include "types.hpp"
#include "iterative_methods.hpp"
#include <chrono>

void problem1(uint n, uint m, fp T, fp a_sq, IterativeMethod* method);
void test_problem(uint n, uint m, fp T, fp a_sq, IterativeMethod* method);
void test_problem2d(uint n, uint m, uint k, fp T, fp a_sq);
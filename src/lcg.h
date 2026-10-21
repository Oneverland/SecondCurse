#ifndef LCG_H
#define LCG_H

typedef unsigned long long ull;

void lcgs(void);
int param_lcg(ull m, ull a, ull c, ull x, ull n, long res[]);
ull lcg_easy(void);
int get_c(ull m, ull output_mas[], ull cmin, ull cmax);
int prime_list(ull m, ull primes[]);
ull get_a(ull m);

#endif

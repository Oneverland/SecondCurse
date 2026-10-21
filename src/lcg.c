#include "lcg.h"
#include <string.h>
#include <time.h>

static ull current_x = 1;

// инициализация рандома
void lcgs(void) {
    current_x = (ull)time(NULL);
}


int param_lcg(ull m, ull a, ull c, ull x, ull n, long res[]) {
    if ((m <= 0) || (a <= 0) || (c <= 0) || (x <= 0) || (n <= 0) || (a >= m) || (c >= m) || (x >= m)) {
        return -1;
    }
    res[0] = x;
    int count = 0;
    int ind = 1;
    while (count < n) {
        count++;
        res[count] = (a * res[count-1] + c) % m;
        if (ind == 1 && res[count] == res[1]) {
            ind = count;
        }
    }
    
    return ind;
}


ull lcg_easy(void){
    ull m = 4294967296, c = 1013904223, a = 1664525;
    current_x = (a * current_x + c) % m;
    return current_x;
}


int get_c(ull m, ull output_mas[], ull cmin, ull cmax) {
    int counter = 0;
    if ((cmin < 0) || (cmax < 0) || (m < 0)) {
        return -1;
    }
    for (ull c = cmin; c <= cmax; c++) {
        if (c > m) {
            break;
        }
        ull a = m, b = c;
        while (b != 0) {
            ull t = a % b;
            a = b;
            b = t;
        }
        if (a == 1) {
            output_mas[counter] = c;
            counter++;
        }
    }
    return counter;
}


int prime_list(ull m, ull primes[]) {
    int count = 0;
    if (m % 2 == 0) {
        primes[count++] = 2;
        while (m % 2 == 0) m /= 2;
    }
    for (ull p = 3; p * p <= m; p += 2) {
        if (m % p == 0) {
            primes[count++] = p;
            while (m % p == 0) m /= p;
        }
    }
    if (m > 1) {
        primes[count++] = m;
    }
    return count;
}


ull get_a(ull m) {
    if (m < 0) {
        return -1;
    }
    ull a = 1;
    ull prime_m[32] = {0};
    int q = prime_list(m, prime_m);
    if (prime_m[0] == 0) {
        return 0;
    }
    for (short i = 0; i < q; i++) {
        a *= prime_m[i];
    }
    if (m % 4 == 0) {
        a *= 2;
    }
    a++;
    if (a >= m) {
        return 0;
    }
    return a;
}

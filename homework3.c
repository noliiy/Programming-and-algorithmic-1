#include <stdio.h>

typedef struct {
    int a;
    int b;
} combo;

int check(int x, int y) {
    if (x <= 0 || y <= 0) return 0;
    if (x == y) return 0;
    return 1;
}

int find(int l1, int l2, long long t, combo* arr) {
    int c = 0;
    for (int i = 0; i * l1 <= t; i++) {
        for (int j = 0; j * l2 <= t; j++) {
            if (i * l1 + j * l2 == t) {
                arr[c].a = i;
                arr[c].b = j;
                c++;
            }
        }
    }
    return c;
}

int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Extended Euclidean Algorithm to find modular inverse
int extended_gcd(int a, int b, int* x, int* y) {
    if (a == 0) {
        *x = 0;
        *y = 1;
        return b;
    }
    
    int x1, y1;
    int gcd_val = extended_gcd(b % a, a, &x1, &y1);
    
    *x = y1 - (b / a) * x1;
    *y = x1;
    
    return gcd_val;
}

int mod_inverse(int a, int m) {
    int x, y;
    int g = extended_gcd(a, m, &x, &y);
    if (g != 1) return -1; // No inverse exists
    
    return (x % m + m) % m;
}

long long count(int l1, int l2, long long t) {
    if (t == 0) return 1;
    
    int g = gcd(l1, l2);
    if (t % g != 0) return 0;
    
    // Optimized approach: use modular arithmetic efficiently
    // Solve l1*x + l2*y = t for non-negative x,y
    
    // Normalize: a*x + b*y = c where a=l1/g, b=l2/g, c=t/g
    int a = l1 / g;
    int b = l2 / g;
    long long c = t / g;
    
    // Find modular inverse of a mod b using Extended Euclidean
    int inv = mod_inverse(a, b);
    if (inv == -1) return 0;
    
    // Find x0 such that a*x0 ≡ c (mod b)
    // Reduce c modulo b to avoid overflow in inv * c
    int c_mod_b = (int)(((c % b) + b) % b);
    int x0 = (int)(((long long)inv * c_mod_b) % b);
    if (x0 < 0) x0 += b;
    
    // All solutions are x = x0 + k*b for k >= 0
    long long max_x = t / l1;
    
    // Calculate number of solutions directly
    if (x0 > max_x) return 0;
    
    return (max_x - x0) / b + 1;
}

int main() {
    int a, b;
    char c;
    long long d;
    
    printf("Track length:\n");
    if (scanf("%d %d", &a, &b) != 2) {
        printf("Invalid input.\n");
        return 0;
    }
    
    if (!check(a, b)) {
        printf("Invalid input.\n");
        return 0;
    }
    
    printf("Distance:\n");
    if (scanf(" %c %lld", &c, &d) != 2) {
        printf("Invalid input.\n");
        return 0;
    }
    
    if (c != '+' && c != '-') {
        printf("Invalid input.\n");
        return 0;
    }
    
    if (d < 0) {
        printf("Invalid input.\n");
        return 0;
    }
    
    if (c == '+') {
        combo arr[10000];
        int n = find(a, b, d, arr);
        
        if (n == 0) {
            printf("No solution.\n");
        } else {
            for (int i = 0; i < n; i++) {
                printf("= %d * %d + %d * %d\n", a, arr[i].a, b, arr[i].b);
            }
            printf("Total variants: %d\n", n);
        }
    } else {
        long long cnt = count(a, b, d);
        
        if (cnt == 0) {
            printf("No solution.\n");
        } else {
            printf("Total variants: %lld\n", cnt);
        }
    }
    
    return 0;
}

#include <stdbool.h>
#include "exo2.h"

bool sousMat1(int n, int m, int **A, int np, int mp, int **B) {
    for (int i = 0; i <= n - np; i++) {
        for (int j = 0; j <= m - mp; j++) {
            bool trouve = true;
            for (int di = 0; di < np && trouve; di++) {
                for (int dj = 0; dj < mp && trouve; dj++) {
                    if (A[i + di][j + dj] != B[di][dj]) {
                        trouve = false;
                    }
                }
            }
            if (trouve) return true;
        }
    }
    return false;
}

bool sousMat2(int n, int m, int **A, int np, int mp, int **B) {
    for (int i = 0; i <= n - np; i++) {
       
        int lo = 0, hi = m - mp, pos = -1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (A[i][mid] == B[0][0]) {
                pos = mid;
                hi = mid - 1; 
            } else if (A[i][mid] < B[0][0]) {
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }

        while (pos != -1 && pos <= m - mp && A[i][pos] == B[0][0]) {
            bool match = true;
            for (int di = 0; di < np && match; di++) {
                for (int dj = 0; dj < mp && match; dj++) {
                    if (A[i + di][pos + dj] != B[di][dj]) {
                        match = false;
                    }
                }
            }
            if (match) return true;
            pos++; 
        }
    }
    return false;
}

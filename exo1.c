#include <stdlib.h>
#include "exo1.h"

int **allouerMatrice(int n, int m)
{
    int **M = (int **)malloc(n * sizeof(int *));
    if (M == NULL) return NULL;

    for (int i = 0; i < n; i++) {
        M[i] = (int *)malloc(m * sizeof(int));
        if (M[i] == NULL) {
            libererMatrice(M, i);
            return NULL;
        }
    }
    return M;
}

void libererMatrice(int **M, int n)
{
    if (M == NULL) return;
    for (int i = 0; i < n; i++)
        free(M[i]);
    free(M);
}

void produitMatrices(int **A, int **B, int **C, int n, int m, int p)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            C[i][j] = 0;
            for (int k = 0; k < m; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}
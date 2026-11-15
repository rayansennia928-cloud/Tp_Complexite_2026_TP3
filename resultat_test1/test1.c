/*
 * Tâche 2 : génération des données et tests expérimentaux
 * Produit de matrices carrées C = A x B (algorithme classique, 3 boucles)
 *
 * Utilise exo1.h / exo1.c : allouerMatrice, libererMatrice, produitMatrices
 * (ces fichiers ne sont PAS modifiés).
 *
 * Résultat : fichier resultats.csv (colonnes : n ; T(n) en secondes)
 *
 * Usage : tache2_tests.exe [graine]
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "exo1.h"

/* ---------- Paramètres de l'expérience (modifiables) ---------- */
#ifndef N_MIN
#define N_MIN 100            /* taille minimale imposée */
#endif
#ifndef N_MAX
#define N_MAX 2000           /* taille maximale imposée */
#endif
#ifndef NB_TAILLES
#define NB_TAILLES 20        /* nombre de tailles différentes testées */
#endif
#define TEMPS_MIN_S 0.3      /* durée minimale mesurée (en secondes) */
#define MAX_REPS (1 << 20)   /* garde-fou sur le nombre de répétitions */
#define VAL_MAX 10           /* éléments aléatoires dans [0, 9] */
#define FICHIER_CSV "resultats.csv"
#define CSV_SEP ';'          /* ';' pour Excel en version française */
#define VIRGULE_DECIMALE 1   /* 1 : "0,123456" (Excel FR) ; 0 : "0.123456" */

/* ---------- Génération aléatoire ---------- */

/* Entier aléatoire uniforme dans [a, b] (évite le biais du modulo) */
static int entierAleatoire(int a, int b)
{
    double u = (double)rand() / ((double)RAND_MAX + 1.0); /* u dans [0, 1[ */
    return a + (int)(u * (b - a + 1));
}

/*
 * Tailles aléatoires "stratifiées" : on découpe [N_MIN, N_MAX] en
 * nb intervalles de même largeur et on tire UNE taille aléatoire dans
 * chacun. Les tailles sont aléatoires, toutes différentes, déjà triées,
 * et couvrent toute la plage (pas de trou).
 */
static void genererTailles(int *tailles, int nb)
{
    int etendue = N_MAX - N_MIN + 1;
    for (int i = 0; i < nb; i++) {
        int bas  = N_MIN + (int)((long long)i * etendue / nb);
        int haut = N_MIN + (int)((long long)(i + 1) * etendue / nb) - 1;
        tailles[i] = entierAleatoire(bas, haut);
    }
}

static void remplirMatrice(int **M, int n, int m)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            M[i][j] = rand() % VAL_MAX;
}

/* Somme de tous les éléments : sert de "témoin" que le calcul a bien eu lieu */
static long long sommeMatrice(int **M, int n, int m)
{
    long long s = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            s += M[i][j];
    return s;
}

/* ---------- Mesure ---------- */

/*
 * Mesure le temps d'UN produit A x B (n x n).
 * Seul produitMatrices() est entre les deux appels à clock().
 * Si le temps total est trop court (< TEMPS_MIN_S), on double le nombre
 * de répétitions et on recommence, puis on renvoie temps_total / reps.
 */
static double mesurerProduit(int **A, int **B, int **C, int n, int *repsUtilisees)
{
    int reps = 1;
    for (;;) {
        clock_t debut = clock();
        for (int r = 0; r < reps; r++)
            produitMatrices(A, B, C, n, n, n);
        clock_t fin = clock();

        double total = (double)(fin - debut) / CLOCKS_PER_SEC;
        if (total >= TEMPS_MIN_S || reps >= MAX_REPS) {
            *repsUtilisees = reps;
            return total / reps;
        }
        reps *= 2;
    }
}

/* ---------- Écriture CSV ---------- */

static void ecrireLigne(FILE *f, int n, double t)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%.6f", t);
#if VIRGULE_DECIMALE
    for (char *p = buf; *p; p++)
        if (*p == '.') *p = ',';
#endif
    fprintf(f, "%d%c%s\n", n, CSV_SEP, buf);
    fflush(f); /* le fichier reste exploitable même si on interrompt le programme */
}

/* ---------- Programme principal ---------- */

int main(int argc, char *argv[])
{
    unsigned int graine = (argc > 1) ? (unsigned int)strtoul(argv[1], NULL, 10)
                                     : (unsigned int)time(NULL);
    srand(graine);
    printf("Graine aleatoire : %u (relancer avec cette valeur pour reproduire)\n",
           graine);
    printf("CLOCKS_PER_SEC = %ld\n\n", (long)CLOCKS_PER_SEC);

    int tailles[NB_TAILLES];
    genererTailles(tailles, NB_TAILLES);

    FILE *f = fopen(FICHIER_CSV, "w");
    if (f == NULL) {
        perror("Impossible de creer " FICHIER_CSV);
        return EXIT_FAILURE;
    }
    fprintf(f, "n%cT(n) en secondes\n", CSV_SEP);

    for (int i = 0; i < NB_TAILLES; i++) {
        int n = tailles[i];

        int **A = allouerMatrice(n, n);
        int **B = allouerMatrice(n, n);
        int **C = allouerMatrice(n, n);
        if (A == NULL || B == NULL || C == NULL) {
            fprintf(stderr, "Erreur d'allocation memoire pour n = %d\n", n);
            if (A) libererMatrice(A, n);
            if (B) libererMatrice(B, n);
            if (C) libererMatrice(C, n);
            fclose(f);
            return EXIT_FAILURE;
        }

        remplirMatrice(A, n, n);   /* hors mesure */
        remplirMatrice(B, n, n);   /* hors mesure */

        int reps = 0;
        double t = mesurerProduit(A, B, C, n, &reps);

        /* Témoin (hors mesure) : empêche de considérer le calcul comme inutile */
        long long temoin = sommeMatrice(C, n, n);

        ecrireLigne(f, n, t);
        printf("[%2d/%d] n = %4d  T = %.6f s  (reps = %d, somme(C) = %lld)\n",
               i + 1, NB_TAILLES, n, t, reps, temoin);

        libererMatrice(A, n);
        libererMatrice(B, n);
        libererMatrice(C, n);
    }

    fclose(f);
    printf("\nResultats enregistres dans %s\n", FICHIER_CSV);
    return EXIT_SUCCESS;
}
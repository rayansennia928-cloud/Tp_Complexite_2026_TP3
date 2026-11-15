
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "exo2.h"

#ifdef AVEC_V2
bool sousMat2_v2(int n, int m, int **A, int np, int mp, int **B);
#endif

/* ---------- Parametres ---------- */
#define VALEUR_MAX 10000   
#define TEMPS_MIN_S 0.3      
#define MAX_REPS (1 << 20)   
#define CSV_SEP ';'
#define VIRGULE_DECIMALE 1   

/* ---------- Fonctions testees ---------- */
typedef bool (*Recherche)(int, int, int **, int, int, int **);
typedef struct { const char *nom; Recherche f; } Algo;

static const Algo ALGOS[] = {
    { "sousMat1", sousMat1 },
    { "sousMat2", sousMat2 },
#ifdef AVEC_V2
    { "sousMat2_v2", sousMat2_v2 },
#endif
};
#define NB_ALGOS ((int)(sizeof ALGOS / sizeof ALGOS[0]))

typedef enum { SCEN_ALEATOIRE, SCEN_PIRE } Scenario;
typedef struct { int n, m, np, mp; } Config;

/* ---------- Matrices dynamiques ---------- */

static void liberer(int **M, int n)
{
    if (M == NULL) return;
    for (int i = 0; i < n; i++) free(M[i]);  
    free(M);
}

static int **allouer(int n, int m)
{
    if (n <= 0 || m <= 0) return NULL;
    int **M = calloc((size_t)n, sizeof *M);   
    if (M == NULL) return NULL;
    for (int i = 0; i < n; i++) {
        M[i] = malloc((size_t)m * sizeof **M);
        if (M[i] == NULL) { liberer(M, n); return NULL; }
    }
    return M;
}

/* ---------- Generation des donnees ---------- */

static int comparer(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static void remplirTrie(int **M, int n, int m)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) M[i][j] = rand() % VALEUR_MAX;
        qsort(M[i], (size_t)m, sizeof(int), comparer);   
    }
}

static void remplirConstante(int **M, int n, int m, int v)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) M[i][j] = v;
}

static void genererDonnees(Scenario s, int **A, int n, int m,
                           int **B, int np, int mp)
{
    if (s == SCEN_ALEATOIRE) {
        remplirTrie(A, n, m);
        remplirTrie(B, np, mp);
        B[np - 1][mp - 1] = VALEUR_MAX;   
    } else {
        remplirConstante(A, n, m, 0);
        remplirConstante(B, np, mp, 0);
        B[np - 1][mp - 1] = 1;            
    }
}

static bool lignesTriees(int **M, int n, int m)
{
    for (int i = 0; i < n; i++)
        for (int j = 1; j < m; j++)
            if (M[i][j - 1] > M[i][j]) return false;
    return true;
}


static double mesurer(Recherche f, int n, int m, int **A, int np, int mp,
                      int **B, bool *resultat, int *reps_utilisees)
{
    int reps = 1;
    for (;;) {
        volatile bool r = false;
        clock_t debut = clock();
        for (int k = 0; k < reps; k++)
            r = f(n, m, A, np, mp, B);
        clock_t fin = clock();

        double total = (double)(fin - debut) / CLOCKS_PER_SEC;
        if (total >= TEMPS_MIN_S || reps >= MAX_REPS) {
            *resultat = r;
            *reps_utilisees = reps;
            return total / reps;
        }
        reps *= 2;
    }
}

/* ---------- CSV ---------- */

static void ecrireLigne(FILE *f, const Config *c, const double *t)
{
    fprintf(f, "%d%c%d%c%d%c%d", c->n, CSV_SEP, c->m, CSV_SEP, c->np, CSV_SEP, c->mp);
    for (int a = 0; a < NB_ALGOS; a++) {
        char buf[64];
        snprintf(buf, sizeof buf, "%.9f", t[a]);
#if VIRGULE_DECIMALE
        for (char *p = buf; *p; p++)
            if (*p == '.') *p = ',';
#endif
        fprintf(f, "%c%s", CSV_SEP, buf);
    }
    fprintf(f, "\n");
    fflush(f);
}

/* ---------- Un point de mesure ---------- */

static int executerPoint(Scenario s, const Config *c, FILE *f)
{
    int code = -1;
    int **A = NULL, **B = NULL;
    double t[NB_ALGOS];

    A = allouer(c->n, c->m);
    B = allouer(c->np, c->mp);
    if (A == NULL || B == NULL) {
        fprintf(stderr, "Erreur d'allocation memoire (n=%d m=%d np=%d mp=%d)\n",
                c->n, c->m, c->np, c->mp);
        goto fin;
    }

    genererDonnees(s, A, c->n, c->m, B, c->np, c->mp);

    if (!lignesTriees(A, c->n, c->m) || !lignesTriees(B, c->np, c->mp)) {
        fprintf(stderr, "Erreur interne : lignes non triees\n");
        goto fin;
    }

    printf("n=%4d m=%4d np=%3d mp=%3d :", c->n, c->m, c->np, c->mp);
    for (int a = 0; a < NB_ALGOS; a++) {
        bool res = false;
        int reps = 0;
        t[a] = mesurer(ALGOS[a].f, c->n, c->m, A, c->np, c->mp, B, &res, &reps);
        if (res) {   /* B est absente par construction : true = bug */
            fprintf(stderr, "\nErreur : %s renvoie true alors que B est absente\n",
                    ALGOS[a].nom);
            goto fin;
        }
        printf("  %s = %.6f s (x%d)", ALGOS[a].nom, t[a], reps);
    }
    printf("\n");

    ecrireLigne(f, c, t);
    code = 0;

fin:
    liberer(A, c->n);
    liberer(B, c->np);
    return code;
}


static int executerSerie(Scenario s, const char *fichier, const Config *cfg, int nb)
{
    FILE *f = fopen(fichier, "w");
    if (f == NULL) {
        perror(fichier);
        return -1;
    }

    fprintf(f, "n%cm%cnp%cmp", CSV_SEP, CSV_SEP, CSV_SEP);
    for (int a = 0; a < NB_ALGOS; a++)
        fprintf(f, "%ctemps_%s", CSV_SEP, ALGOS[a].nom);
    fprintf(f, "\n");

    for (int i = 0; i < nb; i++) {
        const Config *c = &cfg[i];
        if (c->np <= 0 || c->mp <= 0 || c->np >= c->n || c->mp >= c->m) {
            fprintf(stderr, "Dimensions invalides : n=%d m=%d np=%d mp=%d\n",
                    c->n, c->m, c->np, c->mp);
            fclose(f);
            return -1;
        }
        if (executerPoint(s, c, f) != 0) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    printf("-> %s ecrit\n\n", fichier);
    return 0;
}

/* ---------- Programme principal ---------- */

int main(int argc, char *argv[])
{
    unsigned int graine = (unsigned int)time(NULL);
    bool rapide = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rapide") == 0) rapide = true;
        else graine = (unsigned int)strtoul(argv[i], NULL, 10);
    }
    srand(graine);
    printf("Graine : %u  |  mode %s  |  CLOCKS_PER_SEC = %ld\n",
           graine, rapide ? "rapide (test)" : "complet", (long)CLOCKS_PER_SEC);
    printf("Fonctions mesurees :");
    for (int a = 0; a < NB_ALGOS; a++) printf(" %s", ALGOS[a].nom);
    printf("\n\n");

    /* Serie A : n = m varient, np = mp fixes */
    Config serieA[16];
    int nbA = 0;
    {
        int pas = rapide ? 100 : 200;
        int fin = rapide ? 500 : 2000;
        for (int n = pas; n <= fin; n += pas)
            serieA[nbA++] = (Config){ n, n, 10, 10 };
    }

    Config serieB[16];
    int nbB = 0;
    {
        static const int tailles_complet[] = { 10, 20, 30, 40, 50, 75, 100, 125, 150, 200 };
        static const int tailles_rapide[]  = { 5, 10, 20, 40, 60, 80 };
        const int *tb = rapide ? tailles_rapide : tailles_complet;
        int nt  = rapide ? 6 : 10;
        int nfix = rapide ? 200 : 500;
        for (int i = 0; i < nt; i++)
            serieB[nbB++] = (Config){ nfix, nfix, tb[i], tb[i] };
    }

    printf("=== Scenario aleatoire (cas moyen) ===\n");
    if (executerSerie(SCEN_ALEATOIRE, "resultats_exo2_varier_A.csv", serieA, nbA) != 0)
        return EXIT_FAILURE;
    if (executerSerie(SCEN_ALEATOIRE, "resultats_exo2_varier_B.csv", serieB, nbB) != 0)
        return EXIT_FAILURE;

    printf("=== Scenario pire cas ===\n");
    if (executerSerie(SCEN_PIRE, "resultats_exo2_varier_A_pire_cas.csv", serieA, nbA) != 0)
        return EXIT_FAILURE;
    if (executerSerie(SCEN_PIRE, "resultats_exo2_varier_B_pire_cas.csv", serieB, nbB) != 0)
        return EXIT_FAILURE;

    printf("Termine.\n");
    return EXIT_SUCCESS;
}
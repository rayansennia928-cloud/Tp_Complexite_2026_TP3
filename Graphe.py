import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

DOSSIER_DONNEES = os.path.dirname(os.path.abspath(__file__))
SORTIE = "graphes"
os.makedirs(SORTIE, exist_ok=True)

plt.rcParams.update({"font.size": 11, "axes.grid": True, "grid.alpha": .3, "figure.dpi": 130})
ROUGE, BLEU, VERT = "#d62728", "#1f77b4", "#2ca02c"


def lire(chemin):
    nom = os.path.basename(chemin)
    return pd.read_csv(os.path.join(DOSSIER_DONNEES, nom), sep=";", decimal=",")

def enregistrer(fig, nom):
    fig.tight_layout()
    fig.savefig(os.path.join(SORTIE, nom))
    plt.close(fig)

d = lire("resultat_test1/resultats1.csv")
d.columns = ["n", "T"]
n, T = d["n"].values.astype(float), d["T"].values

# Modèle théorique T = a·n³, constante a calée (moindres carrés) sur les petites tailles n <= 1000
petit = n <= 1000
a = np.sum(T[petit] * n[petit] ** 3) / np.sum(n[petit] ** 6)
pente = np.polyfit(np.log(n), np.log(T), 1)[0]
print(f"a = {a:.3e} ; pente log-log (toutes valeurs) = {pente:.2f}")
print(f"pente log-log pour n <= 1000 : {np.polyfit(np.log(n[petit]), np.log(T[petit]), 1)[0]:.2f}")
xs = np.linspace(150, 2000, 300)

#T en fonction de n

fig, ax = plt.subplots(figsize=(7, 4.5))
ax.plot(n, T, "o", color=ROUGE, label="Mesures expérimentales")
ax.plot(xs, a * xs ** 3, "--", color=VERT, label="Modèle théorique  T = a·n³  (a calé sur n ≤ 1000)")
ax.set_xlabel("n (taille des matrices n×n)")
ax.set_ylabel("T (secondes)")
ax.set_title("Exercice 1 – Temps du produit matriciel en fonction de n")
ax.legend()
enregistrer(fig, "ex1_T_en_fonction_de_n.png")

# échelle log-log
fig, ax = plt.subplots(figsize=(7, 4.5))
ax.loglog(n, T, "o", color=ROUGE, label=f"Mesures (pente ajustée ≈ {pente:.2f})")
ax.loglog(xs, T[0] * (xs / n[0]) ** 3, "--", color=VERT, label="Pente 3 (n³) de référence")
ax.set_xlabel("n (échelle log)")
ax.set_ylabel("T en secondes (échelle log)")
ax.set_title("Exercice 1 – Échelle log-log")
ax.legend()
ax.grid(True, which="both", alpha=.25)
enregistrer(fig, "ex1_loglog.png")

# T / n³ (constant si le modèle en n³ est parfait)
fig, ax = plt.subplots(figsize=(7, 4.5))
ax.plot(n, T / n ** 3 * 1e9, "o-", color=BLEU)
ax.set_xlabel("n")
ax.set_ylabel("T(n) / n³  (nanosecondes)")
ax.set_title("Exercice 1 – Temps par opération élémentaire (T/n³)\nconstant si le modèle en n³ était parfait")
enregistrer(fig, "ex1_T_sur_n3.png")


# Exercice 2 : sous-matrice
def serie(fichier, colonne_x, titre, nom, xlabel, modele=None, label_modele="", logy=False):
    s = lire(fichier)
    x = s[colonne_x].values
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(x, s.temps_sousMat1, "o-", color=ROUGE, label="sousMat1 (naïve)")
    ax.plot(x, s.temps_sousMat2, "s-", color=BLEU, label="sousMat2 (sur lignes triées)")
    if modele is not None:
        th = modele(s)
        k = np.sum(s.temps_sousMat1.values * th) / np.sum(th ** 2)  
        ax.plot(x, k * th, ":", color=VERT, lw=2, label=label_modele)
    if logy:
        ax.set_yscale("log")
    ax.set_xlabel(xlabel)
    ax.set_ylabel("T (secondes)" + (" – échelle log" if logy else ""))
    ax.set_title(titre)
    ax.legend()
    enregistrer(fig, nom)
    return s


# modèles théoriques (n' = np dans les CSV)
def pos_positions(s):            # nombre de positions de départ ~ (n-n'+1)²   (n = m, n' = m')
    return (s.n.values - s.np.values + 1.0) ** 2


def pire_cas(s):                 # (n-n'+1)(m-m'+1)·n'm'  avec n = m, n' = m'
    return (s.n.values - s.np.values + 1.0) ** 2 * s.np.values ** 2


serie("resultats_test2/resultats_exo2_varier_A.csv", "n",
      "Exo 2 – Cas moyen, n = m variable (n' = m' = 10)", "ex2_A_cas_moyen.png", "n = m",
      pos_positions, "Modèle (n−n'+1)²  (calé)", logy=True)

serie("resultats_test2/resultats_exo2_varier_A_pire_cas.csv", "n",
      "Exo 2 – Pire cas, n = m variable (n' = m' = 10)", "ex2_A_pire_cas.png", "n = m",
      pire_cas, "Modèle (n−n'+1)²·n'²  (calé)")

serie("resultats_test2/resultats_exo2_varier_B.csv", "np",
      "Exo 2 – Cas moyen, n' = m' variable (n = m = 500)", "ex2_B_cas_moyen.png", "n' = m'",
      logy=True)

serie("resultats_test2/resultats_exo2_varier_B_pire_cas.csv", "np",
      "Exo 2 – Pire cas, n' = m' variable (n = m = 500)", "ex2_B_pire_cas.png", "n' = m'",
      pire_cas, "Modèle (n−n'+1)²·n'²  (calé)")

# pentes log-log de la série A (comparaison avec l'ordre théorique)
for f in ["varier_A", "varier_A_pire_cas"]:
    s = lire(f"resultats_test2/resultats_exo2_{f}.csv")
    for c in ["temps_sousMat1", "temps_sousMat2"]:
        print(f"{f:20s} {c}: pente log-log = {np.polyfit(np.log(s.n), np.log(s[c]), 1)[0]:.2f}")

print(f"\nGraphiques écrits dans ./{SORTIE}/")
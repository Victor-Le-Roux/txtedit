# `txtedit` — Éditeur de fichiers texte en C

## Sujet de projet final

> **Durée cible :** 5 à 7 jours  
> **Langage :** C  
> **Environnement cible :** Linux / POSIX  
> **Objectif principal :** maîtriser les pointeurs, les listes chaînées, l'allocation dynamique et la propriété de la mémoire.

---

## 1. Présentation

`txtedit` est un éditeur de texte en ligne de commande capable d'ouvrir, modifier et sauvegarder un véritable fichier `.txt`.

Le programme ne doit pas modifier directement quelques caractères isolés sur le disque. Il doit :

1. ouvrir le fichier demandé ;
2. lire son contenu ;
3. représenter chaque ligne par un nœud d'une liste chaînée en mémoire ;
4. exécuter les commandes de l'utilisateur sur cette liste ;
5. reconstruire le fichier lors de la sauvegarde.

Le projet doit produire un véritable outil utilisable :

```bash
./txtedit document.txt
```

Exemple de session :

```text
Opened document.txt: 124 lines

txtedit> print 1 10
txtedit> insert 5 Nouvelle ligne
txtedit> delete 8 12
txtedit> move 15 20 3
txtedit> copy 2 5
txtedit> paste 30
txtedit> undo
txtedit> redo
txtedit> save
Saved document.txt
txtedit> quit
```

---

## 2. Objectifs pédagogiques

À la fin du projet, tu dois être capable de :

- construire une liste simplement chaînée et une liste doublement chaînée ;
- insérer, retirer, déplacer, inverser et détruire des nœuds ;
- manipuler une tête ou un sommet de pile avec un double pointeur ;
- distinguer un pointeur possédé d'un pointeur seulement emprunté ;
- différencier une copie profonde d'une copie superficielle ;
- déplacer un groupe de nœuds sans recopier son contenu ;
- lire un fichier par blocs sans imposer de longueur maximale aux lignes ;
- gérer proprement `malloc`, `realloc` et `free` ;
- traiter les erreurs sans perdre de mémoire ni corrompre le fichier original ;
- expliquer précisément la durée de vie de chaque allocation.

---

## 3. Travail de conception obligatoire

**Aucune structure n'est fournie dans ce sujet.** Leur conception fait partie de l'exercice.

Avant d'écrire les fonctions du programme, tu dois déterminer toi-même comment représenter :

- une ligne de texte ;
- le document complet ;
- le début et la fin du document ;
- la ligne courante ;
- le nombre de lignes ;
- le chemin du fichier ouvert ;
- le fait que le document a été modifié ;
- un groupe continu de lignes détachées ;
- le presse-papiers ;
- une action annulable ;
- les piles `undo` et `redo` ;
- un texte en construction dont la capacité doit pouvoir augmenter.

Le document doit être une **liste doublement chaînée**. Les historiques `undo` et `redo` doivent être des **piles simplement chaînées**.

Tu dois également décider :

1. quelles structures possèdent quelles allocations ;
2. quelles fonctions détruisent ou transfèrent ces allocations ;
3. comment représenter un fichier vide ;
4. comment distinguer une dernière ligne terminée par `\n` d'une dernière ligne sans `\n` ;
5. ce que devient la ligne courante lorsqu'elle est supprimée ;
6. comment remettre le programme dans un état valide après une erreur d'allocation ;
7. quelles informations une action doit conserver pour pouvoir être annulée puis rétablie.

Ces décisions doivent être consignées dans un court fichier `DESIGN.md` avant l'implémentation.

---

## 4. Contraintes générales

- Le projet doit être écrit en C.
- Les variables globales sont interdites.
- Le document ne doit pas être stocké dans un tableau de lignes.
- Chaque ligne doit correspondre à un nœud distinct de la liste du document.
- Un tableau fixe peut être utilisé comme tampon temporaire de lecture.
- La longueur maximale d'une ligne ne doit pas dépendre de la taille du tampon de lecture.
- `getline()` est interdit : la construction dynamique des lignes fait partie de l'exercice.
- Une bibliothèque fournissant déjà des listes chaînées est interdite.
- Réordonner le document doit modifier les liens entre les nœuds, et non uniquement échanger leurs textes.
- Toutes les ressources doivent être libérées, y compris lorsqu'une opération échoue au milieu de son exécution.
- Le programme doit compiler sans avertissement avec les options demandées.

Compilation minimale :

```bash
cc -std=c17 -Wall -Wextra -Werror -Wpedantic
```

Compilation de développement recommandée :

```bash
cc -std=c17 -Wall -Wextra -Werror -Wpedantic \
    -g3 -fsanitize=address,undefined \
    -fno-omit-frame-pointer
```

---

## 5. Chargement d'un document

Le chemin du fichier à ouvrir est reçu en argument :

```bash
./txtedit document.txt
```

Le programme doit refuser proprement :

- l'absence d'argument ;
- plusieurs chemins de fichiers ;
- un fichier inexistant ;
- un fichier impossible à lire ;
- un chemin correspondant à un répertoire.

### Lecture par blocs

Le fichier doit être lu progressivement avec `read()`. Un tampon de quelques kilo-octets peut être utilisé, mais il ne constitue jamais une limite de longueur pour une ligne.

Une ligne de 100 000 caractères doit donc être reconstruite à partir de plusieurs lectures avant de devenir un seul nœud.

Le chargeur doit préserver :

- les lignes vides ;
- plusieurs lignes vides consécutives ;
- les espaces et tabulations ;
- les octets UTF-8 sans tenter de les décoder ;
- l'absence éventuelle de retour à la ligne final.

Dans le cadre du projet, les fichiers contenant un octet nul peuvent être refusés comme n'étant pas des fichiers texte pris en charge.

### Fidélité obligatoire

Charger puis sauvegarder immédiatement un document sans modification doit produire un fichier strictement identique, octet par octet.

```bash
cp document.txt test.txt
./txtedit test.txt
# Dans l'éditeur : save, puis quit
cmp document.txt test.txt
```

`cmp` ne doit afficher aucune différence.

---

## 6. Numérotation et positions

- Les numéros de ligne commencent à `1`.
- La ligne `1` est la première ligne du document.
- Une position d'insertion peut aller de `1` à `nombre_de_lignes + 1`.
- La position `nombre_de_lignes + 1` signifie « à la fin du document ».
- Un intervalle `FIRST LAST` est inclusif : les deux lignes appartiennent à l'intervalle.
- Toute position invalide doit produire une erreur sans modifier le document.

Pour `move FIRST LAST POSITION`, l'intervalle est d'abord retiré. `POSITION` est ensuite interprétée dans le document restant. Le groupe est inséré avant cette position ; la position située après la dernière ligne permet de l'ajouter à la fin.

---

## 7. Commandes obligatoires

### `help`

Affiche la liste et la syntaxe des commandes disponibles.

```text
help
```

### `status`

Affiche au minimum :

- le chemin du fichier ;
- le nombre de lignes ;
- si le document contient des modifications non sauvegardées ;
- la position de la ligne courante, si elle existe.

### `print`

Affiche un intervalle de lignes avec leurs numéros.

```text
print FIRST LAST
```

Exemple :

```text
print 3 5
```

### `insert`

Insère une nouvelle ligne avant la position indiquée.

```text
insert POSITION TEXT
```

Le texte peut contenir des espaces et des tabulations.

### `append`

Insère une nouvelle ligne après la ligne indiquée.

```text
append LINE TEXT
```

### `replace`

Remplace le contenu d'une ligne sans changer sa position.

```text
replace LINE TEXT
```

### `delete`

Supprime un intervalle de lignes.

```text
delete FIRST LAST
```

### `move`

Déplace un intervalle complet sans recréer ses nœuds.

```text
move FIRST LAST POSITION
```

### `reverse`

Inverse l'ordre des nœuds d'un intervalle.

```text
reverse FIRST LAST
```

L'opération doit modifier les liens entre les nœuds. Échanger uniquement les pointeurs vers les textes est interdit.

### `copy`

Copie profondément un intervalle dans le presse-papiers.

```text
copy FIRST LAST
```

Le presse-papiers doit devenir indépendant du document : modifier ou détruire l'un ne doit pas invalider l'autre.

### `cut`

Détache un intervalle du document et le place dans le presse-papiers.

```text
cut FIRST LAST
```

Les nœuds coupés doivent être transférés au presse-papiers sans être recréés.

Si le presse-papiers contenait déjà des données, celles-ci doivent être libérées correctement avant son remplacement.

### `paste`

Colle le presse-papiers avant la position indiquée.

```text
paste POSITION
```

Le presse-papiers doit rester utilisable après le collage. Deux commandes `paste` consécutives doivent donc produire deux copies indépendantes.

### `undo`

Annule la dernière modification prise en charge par l'historique.

```text
undo
```

Au minimum, l'historique doit prendre en charge :

- `insert` ;
- `append` ;
- `replace` ;
- `delete`.

Prendre également en charge `move`, `reverse`, `cut` et `paste` constitue l'objectif de maîtrise complète.

### `redo`

Rétablit la dernière action annulée.

```text
redo
```

Après un `undo`, toute nouvelle modification doit vider entièrement la pile `redo`.

### `save`

Sauvegarde le document sur son chemin actuel.

```text
save
```

### `saveas`

Sauvegarde le document vers un nouveau chemin et utilise ensuite ce chemin comme chemin courant.

```text
saveas PATH
```

### `quit`

Quitte seulement si aucune modification non sauvegardée n'existe.

```text
quit
```

### `quit!`

Quitte en abandonnant les modifications non sauvegardées.

```text
quit!
```

---

## 8. Listes chaînées attendues

### Document

Le document doit être représenté par une liste doublement chaînée afin de permettre :

- le parcours vers l'avant et vers l'arrière ;
- l'insertion autour d'une ligne connue ;
- le retrait d'une ligne connue ;
- le détachement d'un intervalle ;
- la réinsertion d'un groupe de nœuds ;
- la conservation d'une tête et d'une queue cohérentes.

### Historique

Les historiques `undo` et `redo` doivent fonctionner comme deux piles reposant sur des listes simplement chaînées.

Au moins une fonction de gestion de pile doit utiliser un double pointeur afin de pouvoir remplacer le sommet détenu par l'appelant.

### Presse-papiers

Le presse-papiers doit pouvoir posséder un groupe complet de lignes indépendamment du document.

Il faut distinguer :

- la copie profonde effectuée par `copy` ;
- le transfert de nœuds effectué par `cut` ;
- la nouvelle copie effectuée par `paste` pour préserver le presse-papiers.

---

## 9. Sauvegarde sûre

La sauvegarde ne doit pas détruire le fichier original si une écriture échoue.

Procédure minimale :

1. créer un fichier temporaire dans le même répertoire ;
2. écrire toutes les lignes dans leur ordre actuel ;
3. gérer correctement les écritures partielles ;
4. vérifier les erreurs d'écriture et de fermeture ;
5. remplacer l'original seulement lorsque l'écriture complète a réussi ;
6. supprimer le fichier temporaire si la sauvegarde échoue.

Une fonction dédiée doit garantir qu'une quantité complète d'octets est écrite même lorsqu'un appel à `write()` n'en écrit qu'une partie.

Le marqueur « document modifié » ne doit être effacé qu'après une sauvegarde réussie.

---

## 10. Gestion des erreurs

Une erreur doit :

- être affichée sur la sortie d'erreur ;
- expliquer l'opération concernée ;
- laisser le document dans un état cohérent ;
- libérer les allocations temporaires devenues inutiles ;
- ne pas effacer les données valides déjà chargées ;
- ne pas faire passer une sauvegarde échouée pour une réussite.

Le programme doit notamment traiter :

- les erreurs de `open`, `read`, `write`, `close` et `rename` ;
- les erreurs d'allocation ;
- les commandes inconnues ;
- les arguments manquants ;
- les nombres invalides ou dépassant leur type ;
- les intervalles inversés ;
- les positions hors limites ;
- les opérations impossibles sur un document vide ;
- `undo` ou `redo` avec une pile vide ;
- `paste` avec un presse-papiers vide.

---

## 11. Invariants obligatoires

Après chaque modification, le programme doit pouvoir vérifier les propriétés suivantes :

- un document vide ne possède ni première ni dernière ligne ;
- un document non vide possède une première et une dernière ligne ;
- la première ligne n'a pas de précédente ;
- la dernière ligne n'a pas de suivante ;
- chaque relation vers la ligne suivante possède la relation inverse correcte ;
- chaque relation vers la ligne précédente possède la relation inverse correcte ;
- aucun cycle non prévu n'existe ;
- le nombre de nœuds réellement accessibles correspond au compteur mémorisé ;
- le parcours avant et le parcours arrière retrouvent exactement les mêmes nœuds ;
- la ligne courante est absente ou appartient réellement au document ;
- aucun nœud n'est possédé simultanément par le document et le presse-papiers ;
- les piles d'historique ne partagent pas une allocation dont elles pourraient provoquer la double libération.

Une fonction de validation doit retourner un état de réussite ou d'échec. Elle doit être appelée systématiquement dans les tests après toute opération mutante.

---

## 12. Tests obligatoires

### Fichiers de test

Préparer au minimum :

```text
tests/empty.txt
tests/one_line.txt
tests/only_newline.txt
tests/no_final_newline.txt
tests/empty_lines.txt
tests/spaces_tabs.txt
tests/utf8.txt
tests/long_line.txt
tests/large_document.txt
```

`long_line.txt` doit contenir une ligne nettement plus grande que le tampon de lecture. `large_document.txt` doit contenir plusieurs milliers de lignes.

### Cas structurels

Chaque opération de liste doit être testée sur :

- une liste vide ;
- un seul nœud ;
- deux nœuds ;
- la tête ;
- la queue ;
- un nœud du milieu ;
- le document entier ;
- un intervalle d'un seul nœud ;
- plusieurs opérations consécutives.

### Cas de propriété

Tester au minimum :

- `copy`, puis destruction du document ;
- `copy`, puis remplacement du presse-papiers ;
- `cut`, puis `paste` ;
- `cut`, puis sortie sans collage ;
- plusieurs `paste` du même presse-papiers ;
- suppression, `undo`, puis `redo` ;
- `undo`, nouvelle modification, puis tentative de `redo` ;
- destruction du programme avec les deux piles d'historique remplies.

### Tests mémoire

Les tests doivent être exécutés avec AddressSanitizer et UndefinedBehaviorSanitizer.

Le résultat final ne doit comporter :

- aucune fuite ;
- aucun accès après libération ;
- aucune double libération ;
- aucun dépassement de tampon ;
- aucune lecture de mémoire non initialisée détectable ;
- aucun comportement indéfini signalé.

---

## 13. Organisation conseillée du dépôt

L'organisation exacte et le découpage des modules font partie de tes décisions de conception. Le dépôt doit néanmoins contenir au minimum :

```text
txtedit/
├── Makefile
├── README.md
├── DESIGN.md
├── include/
├── src/
└── tests/
```

Le `Makefile` doit proposer au minimum :

```text
all
clean
fclean
re
debug
test
```

---

## 14. Plan recommandé sur 7 jours

### Jour 1 — Lecture et construction dynamique

- ouvrir et lire un véritable fichier ;
- reconstruire des lignes de longueur arbitraire ;
- créer progressivement la liste ;
- gérer le fichier vide et l'absence de `\n` final ;
- détruire complètement un document partiellement ou totalement construit.

### Jour 2 — Opérations fondamentales de liste

- parcourir dans les deux directions ;
- retrouver une ligne par son numéro ;
- insérer avant et après ;
- retirer la tête, la queue et le milieu ;
- maintenir tous les invariants après chaque opération.

### Jour 3 — Intervalles de nœuds

- détacher un intervalle ;
- réinsérer un intervalle ;
- déplacer un groupe sans le recréer ;
- inverser les liens d'un groupe ;
- construire `copy`, `cut` et `paste`.

### Jour 4 — Sauvegarde

- reconstruire le fichier ;
- gérer les écritures partielles ;
- sauvegarder via un fichier temporaire ;
- garantir le test de fidélité avec `cmp`.

### Jour 5 — Interface de commandes

- lire une commande ;
- séparer son nom de ses arguments ;
- convertir et valider les numéros ;
- conserver le reste de la ligne comme texte ;
- relier chaque commande à l'opération correspondante.

### Jour 6 — Historique

- créer les piles `undo` et `redo` ;
- enregistrer les informations nécessaires ;
- annuler et rétablir les opérations minimales ;
- gérer précisément la propriété des données conservées dans l'historique.

### Jour 7 — Validation et durcissement

- compléter la fonction de validation ;
- tester tous les cas limites ;
- exécuter les sanitizers ;
- vérifier les chemins d'erreur ;
- nettoyer le découpage du code et la documentation ;
- réaliser l'épreuve finale.

---

## 15. Épreuve finale de maîtrise

Le projet est considéré comme maîtrisé seulement si tu peux, sans consulter une solution extérieure :

1. dessiner l'état mémoire du document avant et après une insertion ;
2. expliquer chaque modification de pointeur lors du retrait d'un nœud ;
3. détacher et réinsérer un intervalle sans perdre ses extrémités ;
4. expliquer pourquoi `copy` exige une copie profonde ;
5. expliquer pourquoi `cut` est un transfert de propriété ;
6. expliquer dans quel cas un double pointeur est nécessaire ;
7. détruire proprement une liste partiellement construite après une erreur ;
8. identifier une fuite, un pointeur pendant, une double libération et une copie superficielle ;
9. prédire quelles adresses changent et lesquelles restent identiques pendant `move` ;
10. reprogrammer de mémoire l'insertion en tête, le retrait, l'inversion et la destruction d'une liste simple.

Le programme doit ensuite réussir ce scénario :

1. ouvrir un document de plusieurs milliers de lignes ;
2. imprimer un intervalle ;
3. insérer et supprimer des lignes ;
4. déplacer et inverser des groupes ;
5. copier, couper et coller ;
6. annuler et rétablir plusieurs opérations ;
7. sauvegarder ;
8. rouvrir le fichier sauvegardé ;
9. retrouver exactement le contenu attendu ;
10. quitter sans erreur détectée par les sanitizers.

---

## 16. Critères de réussite

Le projet est terminé lorsque :

- un vrai fichier `.txt` peut être ouvert, modifié et sauvegardé ;
- les lignes du document sont réellement organisées en liste doublement chaînée ;
- les historiques utilisent réellement des listes simplement chaînées ;
- un groupe de lignes peut être déplacé sans recréer ses nœuds ;
- `copy`, `cut` et `paste` respectent leurs propriétés de mémoire respectives ;
- le fichier original reste intact lorsqu'une sauvegarde échoue ;
- un document non modifié est reproduit octet par octet ;
- tous les invariants sont vérifiés ;
- tous les tests obligatoires réussissent ;
- les sanitizers ne signalent aucune erreur ;
- tu peux justifier toi-même la conception de toutes tes structures.

---

## 17. Restrictions d'apprentissage

Pour que le projet remplisse son objectif pédagogique :

- ne copie pas une implémentation complète d'éditeur ou de liste ;
- ne demande pas directement le code complet d'une opération ;
- commence chaque opération complexe par un dessin des nœuds et des liens ;
- écris qui possède chaque allocation avant de programmer sa libération ;
- teste immédiatement chaque cas limite ;
- demande d'abord un indice conceptuel lorsque tu bloques ;
- ne passe à l'historique qu'après avoir rendu les opérations fondamentales parfaitement fiables.

Le résultat attendu n'est pas seulement un programme fonctionnel. Tu dois comprendre pourquoi chaque pointeur contient son adresse actuelle, qui possède l'objet désigné et jusqu'à quel moment cette adresse reste valide.

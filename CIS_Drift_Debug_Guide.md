# Guide d'utilisation du debug de correction de dérive CIS

## Activation du debug

### Debug simple (correction de dérive)

Pour activer l'affichage des valeurs de correction de dérive en temps réel :

```c
// Activer le debug avec affichage toutes les 100 lignes
cis_enableDriftDebug(100);

// Ou avec un intervalle différent (ex: toutes les 50 lignes)
cis_enableDriftDebug(50);
```

### Debug détaillé (38 pixels inactifs)

Pour voir les valeurs individuelles des 38 pixels inactifs :

```c
// Activer le debug détaillé avec affichage toutes les 500 lignes
cis_enableDetailedDebug(500);

// Ou pour un debug plus fréquent (attention au flood de la console)
cis_enableDetailedDebug(100);
```

### Utilisation manuelle

Pour afficher les pixels inactifs d'une lane/couleur spécifique :

```c
// Afficher les pixels inactifs de la lane 0, couleur rouge (0=Rouge, 1=Vert, 2=Bleu)
cis_printInactivePixels(cisDataCpy, 0, 0);

// Afficher les pixels inactifs de la lane 1, couleur verte
cis_printInactivePixels(cisDataCpy, 1, 1);
```

## Désactivation du debug

```c
// Désactiver le debug simple
cis_disableDriftDebug();

// Désactiver le debug détaillé
cis_disableDetailedDebug();
```

## Format de sortie

Quand le debug est activé, vous verrez des messages comme :

```
Drift correction debug ENABLED (interval: 100 lines)
DRIFT DEBUG - Line 100:
  Lane 0: R=5 G=-2 B=3
  Lane 1: R=7 G=1 B=-1
  Lane 2: R=-3 G=4 B=2
DRIFT DEBUG - Line 200:
  Lane 0: R=6 G=-1 B=4
  Lane 1: R=8 G=2 B=0
  Lane 2: R=-2 G=5 B=1
```

## Interprétation des valeurs

- **R, G, B** : Correction d'offset appliquée pour chaque couleur (Rouge, Vert, Bleu)
- **Lane** : Numéro de la lane ADC (0, 1, 2...)
- **Valeurs positives** : Les pixels inactifs actuels sont plus élevés que la référence de calibration
- **Valeurs négatives** : Les pixels inactifs actuels sont plus bas que la référence de calibration
- **Valeurs proches de 0** : Peu ou pas de dérive détectée

## Seuils d'alerte

- **±10 comptes ADC** : Dérive normale, correction automatique
- **±25 comptes ADC** : Dérive modérée, surveillance recommandée
- **±50 comptes ADC** : Dérive importante (seuil par défaut)
- **>±50 comptes ADC** : Dérive excessive, correction limitée par le seuil

## Exemple d'utilisation complète

```c
void example_drift_debug_usage(void)
{
    // 1. Activer le debug
    cis_enableDriftDebug(100);

    // 2. Utiliser la calibration avec correction de dérive
    cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);

    // 3. Observer les valeurs dans la console
    // Les messages apparaîtront automatiquement toutes les 100 lignes

    // 4. Désactiver le debug quand terminé
    cis_disableDriftDebug();
}
```

## Conseils d'utilisation

1. **Démarrage** : Activez le debug au début pour vérifier que la correction fonctionne
2. **Intervalle** : Utilisez un intervalle de 100-500 lignes pour éviter de surcharger la console
3. **Surveillance** : Surveillez les tendances de dérive au fil du temps
4. **Performance** : Désactivez le debug en production pour optimiser les performances
5. **Diagnostic** : Utilisez le debug pour diagnostiquer les problèmes de capteur

## Désactivation pour la production

N'oubliez pas de désactiver le debug avant la mise en production :

```c
// En production, désactiver le debug
cis_disableDriftDebug();

// Ou simplement ne pas l'activer
```

Le debug n'a aucun impact sur les performances quand il est désactivé.

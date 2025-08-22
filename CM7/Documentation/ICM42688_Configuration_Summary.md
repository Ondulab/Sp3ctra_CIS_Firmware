# Configuration ICM42688 Optimisée pour l'Analyse de Gestes

## Résumé des Modifications

La configuration de l'ICM42688 a été optimisée pour l'analyse de gestes de la main côté ordinateur.

### Configuration Appliquée

| Paramètre | Valeur Précédente | Valeur Optimisée | Justification |
|-----------|-------------------|------------------|---------------|
| **Accéléromètre FS** | ±16G (`gpm16`) | ±4G (`gpm4`) | Meilleure résolution pour les mouvements de main |
| **Gyroscope FS** | ±500 dps (`dps500`) | ±500 dps (`dps500`) | Déjà optimal pour les gestes |
| **ODR Accéléromètre** | Non défini (défaut) | 200Hz (`odr200`) | Fréquence optimale pour l'analyse de gestes |
| **ODR Gyroscope** | Non défini (défaut) | 200Hz (`odr200`) | Synchronisé avec l'accéléromètre |
| **Filtres** | Désactivés (`false, false`) | Activés (`true, true`) | Signaux plus propres pour l'analyse |

### Avantages de cette Configuration

1. **Résolution améliorée** : ±4G offre 4x plus de résolution que ±16G pour les mouvements de main
2. **Fréquence optimale** : 200Hz capture efficacement les gestes sans surcharge
3. **Signaux propres** : Les filtres anti-aliasing et notch réduisent le bruit
4. **Consommation équilibrée** : Configuration performante sans gaspillage d'énergie

### Plages de Mesure Finales

- **Accéléromètre** : ±4G (résolution ~0.122 mg/LSB)
- **Gyroscope** : ±500 dps (résolution ~15.26 mdps/LSB)
- **Fréquence d'échantillonnage** : 200Hz pour les deux capteurs
- **Filtres** : Activés (anti-aliasing + notch pour gyro, anti-aliasing pour accéléromètre)

### Utilisation

Cette configuration est maintenant appliquée par défaut lors de l'appel à `icm42688_init()`. 
Aucune modification de code n'est nécessaire dans l'application - la configuration optimisée 
est automatiquement utilisée.

### Compatibilité

- ✅ API inchangée - compatibilité totale avec le code existant
- ✅ Même interface de fonctions
- ✅ Mêmes unités de sortie (g pour accéléromètre, dps pour gyroscope)
- ✅ Calibration automatique maintenue

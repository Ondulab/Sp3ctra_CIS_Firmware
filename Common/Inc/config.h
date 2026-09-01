/**
 ******************************************************************************
 * @file           : config.h
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
 *
 ******************************************************************************
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stm32h7xx_hal.h"

/* Driver IMU : firmware CM7 uniquement (le bootloader, compilé avec -DBOOTLOADER,
 * n'embarque pas icm42688.h et n'en a pas besoin). */
#if defined(CORE_CM7) && !defined(BOOTLOADER)
#include "icm42688.h"
#endif

/**************************************************************************************/
/*******************              General definitions               *******************/
/**************************************************************************************/
#include "sp3ctra_link.h"

#define FW_VERSION_MAJOR 4
#define FW_VERSION_MINOR 0
#define FW_VERSION_PATCH 0
#define FW_VERSION_STR_(x) #x
#define FW_VERSION_STR(x) FW_VERSION_STR_(x)
#define FW_VERSION FW_VERSION_STR(FW_VERSION_MAJOR) "." FW_VERSION_STR(FW_VERSION_MINOR) "." FW_VERSION_STR(FW_VERSION_PATCH)
#define HW_REVISION 1

/**************************************************************************************/
/********************              Debug definitions               ********************/
/**************************************************************************************/
//#define PRINTF_CM4

//#define DEBUG_LWIP_STATS
//#define HTTP_SERVER_DEBUG

//#define CIS_PRINT_COUNTER

//#define DEBUG_ICM42688

//#define USE_WDG

// CIS Drift Correction Configuration
#define CIS_DRIFT_DEBUG_ENABLED                 (0)     // Debug drift correction (0=disabled, 1=enabled)
#define CIS_DRIFT_DEBUG_INTERVAL                (100)   // Print debug info every N lines
#define CIS_DETAILED_DEBUG_ENABLED              (0)     // Detailed debug with pixel values (0=disabled, 1=enabled)
#define CIS_DETAILED_DEBUG_INTERVAL             (100)   // Print detailed debug every N lines

// Sonde de phase TEMPORAIRE : imprime, une ligne sur N, les 80 premiers echantillons
// BRUTS copies par le MDMA (voie 0, R/G/B), avant application de la calibration.
// Contrairement aux lectures SWD, ce dump est COHERENT : pris d'un bloc dans la tache
// de scan, sur une seule et meme ligne. Il montre la structure reelle de la trame
// (plateau noir, impulsion SP, overscan, montee LED, actif) et, compare d'un dump au
// suivant, toute derive de phase echantillon par echantillon. A remettre a 0 une fois
// l'alignement fixe.
#define CIS_PHASE_DEBUG_ENABLED                 (0)
#define CIS_PHASE_DEBUG_INTERVAL                (8192)  // ~8 s a 1063 lps (dump lourd)

/**************************************************************************************/
/********************              UI definitions                 *********************/
/**************************************************************************************/
// Default UI configuration values
#define DEFAULT_GUI_SHOW_IMU                    (0)      // Set to 0 to hide IMU panel and use full image area
#define DEFAULT_GUI_INVERT_CIS_IMAGE            (1)      // Set to 1 to invert CIS image colors (white background, dark content)
#define DEFAULT_SCREENSAVER_TIMEOUT_SEC         (60)     // Screensaver timeout in seconds (default: 60s)
#define DEFAULT_SCREENSAVER_DISPLAY_OFF_SEC     (600)    // OLED switched OFF this long after the screensaver started (burn-in protection)
#define DEFAULT_MOTION_THRESHOLD_ACC            (0.08f)  // Accelerometer motion threshold in g (default: 0.08g)
#define DEFAULT_MOTION_THRESHOLD_GYRO           (2.0f)   // Gyroscope motion threshold in dps (default: 2.0 dps)

// UI configuration value ranges
#define MIN_SCREENSAVER_TIMEOUT_SEC             (1)      // Minimum: 1 second
#define MAX_SCREENSAVER_TIMEOUT_SEC             (1000)   // Maximum: 1000 seconds
#define MIN_MOTION_THRESHOLD_ACC                (0.01f)  // Minimum: 0.01g (very sensitive)
#define MAX_MOTION_THRESHOLD_ACC                (1.0f)   // Maximum: 1.0g (less sensitive)
#define MIN_MOTION_THRESHOLD_GYRO               (0.5f)   // Minimum: 0.5 dps (very sensitive)
#define MAX_MOTION_THRESHOLD_GYRO               (10.0f)  // Maximum: 10.0 dps (less sensitive)

#define DEFAULT_CIS_HANDEDNESS 					(1)

#define	NUMBER_OF_BUTTONS						(3)

#define	NUMBER_OF_LEDS							(3)

/**************************************************************************************/
/*******************              Storage definitions               *******************/
/**************************************************************************************/
#define FILE_NAME_MAX_LENGTH                    (256)  //Max filename length

#define CALIBRATION_FILE_PATH_FORMAT 			"0:/CIS_CALIB_%ddpi.BIN"
#define CONFIG_FILE_PATH 						"0:/CONFIG.TXT"
#define IMU_CALIBRATION_FILE_PATH 				"0:/IMU_CALIB.BIN"

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
// Network configurations
#define DEFAULT_NETWORK_IP 						{192, 168, 100, 1}
#define DEFAULT_NETWORK_NETMASK 				{255, 255, 255, 0}
#define DEFAULT_NETWORK_GW 						{0, 0, 0, 0}
#define DEFAULT_NETWORK_DEST_IP 				{192, 168, 100, 10}

// CIS image streaming (UDP)
// This UDP port is used to transmit CIS image fragments to DEFAULT_NETWORK_DEST_IP.
#define DEFAULT_NETWORK_CIS_UDP_PORT 			(55151)
#define DEFAULT_NETWORK_LINK_PORT 			(SLP_CTRL_PORT)      // SLP control channel (device listens)
#define DEFAULT_STREAM_WHEN_UNBOUND 			(1)                  // keep streaming to DEFAULT_NETWORK_DEST_IP without a host session


/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define POLYNOMIAL_CALIBRATION

// CIS configurations
#define DEFAULT_CIS_PRINT_CALIBRATION 0
#define DEFAULT_CIS_RAW 0
#define DEFAULT_CIS_DPI 400
#define DEFAULT_CIS_OVERSAMPLING 1

// Horloge CIS (CP), par mode.
// Datasheet M118-232C3 : en 400 dpi le capteur sort DEUX pixels par periode de CP
// (definition 2 + figure 4), en 200 dpi un seul. La ligne reelle en 400 dpi fait donc
// ~602 horloges pour 1204 echantillons -- mesure sonde v3 : structure period 602,
// scene dupliquee 6x dans l'ancienne trame de 1204 horloges.
// A 2 MHz avec echantillonnage sur les deux phases (TIM1 CH3 en PWM asymetrique,
// conversions a 125 et 375 ns, ADC sur les deux fronts), le debit d'echantillons reste
// 4 Mech/s : tampons, MDMA, calibration et reseau inchanges, et cette fois les 1152
// pixels PHYSIQUES de chaque voie sont tous captures.
#define CIS_400DPI_CLK_FREQ						(2000000)
#define CIS_200DPI_CLK_FREQ						(4000000)

#define CIS_CAPTURE_TIMEOUT 					(100)

#define CIS_ADC_OUT_LANES						(3)
#define COLOR_CHANNELS        			 		(3)

/* Geometrie de la trame -- v5, etablie par l'essai du trait discontinu (2026-08-31).
   Le capteur est bien en 400 dpi : chaque sortie delivre 1152 pixels, a 2 px par
   periode CP. A CP = 2 MHz avec l'echantillonnage double front (TIM15), on capture
   1 echantillon PAR pixel : la ligne complete = 2 (SP) + 38 (inactifs) + 1152
   (effectifs) + 12 (overscan) = 1204 echantillons -- les constantes historiques.
   L'episode intermediaire "576 px, tete 18, noir 8" etait un artefact : le SP cadence
   a 602 echantillons TRONQUAIT chaque ligne a mi-parcours (moitie de chaque tiers
   invisible a l'ecran, plateaux quand le doigt longeait la coupure). Les timers
   esclaves comptent DEUX ticks par periode CP : leurs periodes/graines s'expriment en
   ECHANTILLONS (lane_size), ce qui donne un SP toutes les 1204 ticks = 602 horloges =
   301 us = la duree exacte d'une ligne. */
#define CIS_SP_WIDTH							(2)
#define CIS_BLACK_PIXELS						(38)
#define CIS_IGNORE_FIRST_BLACK_PIXELS			(8)
#define CIS_USEFUL_BLACK_PIXELS					(24)
#define CIS_INACTIVE_WIDTH						((CIS_BLACK_PIXELS) + (CIS_SP_WIDTH))
#define CIS_OVER_SCAN							(12)

#define CIS_400DPI_PIXELS_PER_LANE		        (1152)
#define CIS_200DPI_PIXELS_PER_LANE		        (576)

#define CIS_400DPI_PIXELS_NB					(3456)
#define CIS_200DPI_PIXELS_NB					(1728)

#define CIS_MAX_PIXELS_PER_LANE					(CIS_400DPI_PIXELS_PER_LANE)
#define CIS_MAX_PIXELS_NB 		 				(CIS_400DPI_PIXELS_NB)

#define CIS_MAX_PIXEL_AERA_STOP					((CIS_INACTIVE_WIDTH) + (CIS_MAX_PIXELS_PER_LANE))

#define CIS_MAX_LANE_SIZE 						((CIS_MAX_PIXEL_AERA_STOP) + (CIS_OVER_SCAN))

#define CIS_LED_ON								(CIS_SP_WIDTH)

/* Trim de phase optique, en coups d'horloge CIS.
   Mesure SWD sur cible (fenetres noires des 3 couleurs, voies 0 et 1) : la trame optique
   arrive ~20 echantillons trop tot dans le tampon ADC -- le saut noir->actif tombe a
   l'indice 17-18 des 38 pixels masques au lieu de rester hors fenetre. Les 20 derniers
   "pixels noirs" sont donc des pixels actifs eclaires, et la correction de derive
   (moyenne des pixels 8..31, recalculee chaque ligne) suit alors le contenu de l'image
   au bord de la voie : bandes de teinte entieres au rythme des gestes de l'operateur.
   Ce retard s'applique d'un bloc aux graines de TIM8 (SP) et TIM3/4/5 (LEDs), qui
   gardent leurs phases relatives ; il ramene SP aux echantillons 0-1, le noir en 2..39,
   l'actif en 40+, comme le suppose la copie MDMA. Latence interne du CIS en nombre de
   coups d'horloge : identique en 200 et 400 DPI. */
#define CIS_PHASE_TRIM							(7)   /* en ticks esclaves (= echantillons) ; cale le plateau noir sur copied[0..7] */

/* Pipeline d'echantillonnage, en echantillons. Le declenchement ADC sur le plateau
   ETABLI (65 ns) echantillonne la valeur du pixel PRECEDENT : decalage global d'un
   pixel, absorbe par l'auto-localisation PARTOUT SAUF aux frontieres de ligne, ou le
   premier echantillon effectif porte le DERNIER pixel de la ligne precedente --
   l'extremite opposee de la voie, dans la couleur d'avant : 1-2 rangees racontant
   « une autre partie de l'image » aux trois intersections, stables et identiques a
   chaque boot (vecu). Ce decalage s'ajoute au depart de copie trouve par
   l'auto-localisation. */
/* Pipeline = 2, valide par la provenance du CONTENU (2 rangees etrangeres en tete de
   voie a 0, zero a 2 -- observation utilisateur sur mire). Metrologie a la regle acier
   0,5 mm (demodulation de phase) : les marches de -0,9/-0,7 px aux coutures 1152/2304
   sont INDEPENDANTES de cette constante -- 1152 = 6 x 192 exactement : ce sont les
   jonctions d'aboutage des sous-ensembles du capteur (18 puces de 192 px, dent de scie
   de pas ±0,3 px par puce, mesurees). Remede futur : LUT geometrique de
   reechantillonnage sous-pixel construite a partir d'un scan de regle. */
#define CIS_SAMPLE_PIPELINE                     (2)

// LED illumination durations in microseconds
/* Fenetre d'eclairage par couleur, en VRAIES microsecondes depuis la correction du
   domaine dans cis_configure (les timers LED tiquent en echantillons, 2 par periode
   CP ; l'ancien calcul divisait par la periode CP et produisait une fenetre moitie
   moindre qu'annonce). 225 us sur les 301 us de la fenetre d'integration 400 dpi :
   mesure sur papier blanc, p95 = 10 400 comptes avec l'ancienne fenetre de 150 us
   -> ~13 900 attendus, soit ~85 % des 16383 pleine echelle. Monter au-dela sature
   les voies bleues au blanc de calibration. RECALIBRATION OBLIGATOIRE apres tout
   changement ici. */
#define CIS_400DPI_LED_DURATION_US              (225)
#define CIS_200DPI_LED_DURATION_US              (112)

#define CIS_MAX_ADC_BUFF_SIZE 	 	 		    ((CIS_MAX_LANE_SIZE) * (COLOR_CHANNELS))
#define CIS_MAX_USEFUL_DATA_SIZE 			    (((CIS_BLACK_PIXELS) + (CIS_400DPI_PIXELS_PER_LANE)) * (COLOR_CHANNELS))

/* Resolution de l'ADC. Le CIS n'excursionne que sur ~1/5 de la pleine echelle (noir 272,
   blanc ~490 en 10 bits, mesure sur cible) : en 10 bits il ne restait que ~216 comptages
   utiles, donc environ 11 niveaux pour les 5 % les plus sombres -- le pas de quantification
   y depassait le bruit, plus rien ne le tramait, d'ou le banding des basses lumieres.
   A 4 Mech/s la periode est de 250 ns ; l'horloge ADC est a HCLK/2 = 100 MHz, et la
   conversion 10 bits n'occupait que 7 cycles, soit 70 ns. La marge etait donc enorme.
   En 14 bits : 7,5 cycles de SAR + 8,5 d'echantillonnage = 16 cycles = 160 ns, encore
   36 % de marge -- et 85 ns d'acquisition au lieu de 15, ce qui laisse enfin le temps a
   la sortie analogique du CIS de s'etablir. */
#define CIS_ADC_BITS                            (14)
#define CIS_ADC_FULL_SCALE                      ((1 << CIS_ADC_BITS) - 1)

#define CIS_LEDS_MAX_PWM                        (101)
#define CIS_LEDS_MAX_POMER		                (CIS_LEDS_MAX_PWM)

// Derive maximale corrigee, en comptages ADC : exprimee en fraction de la pleine echelle
// pour suivre automatiquement CIS_ADC_BITS -- meme fraction (100/1023) que le seuil
// eprouve en 10 bits, ni resserree ni relachee.
#define CIS_DRIFT_THRESHOLD                     ((CIS_ADC_FULL_SCALE * 100) / 1023)

/**************************************************************************************/
/***                  ISP : affine par pixel + courbe partagée (LUT)                ***/
/**************************************************************************************/

// Deux grandeurs physiquement distinctes étaient jusqu'ici mélangées dans un même
// jeu de tableaux par pixel :
//   - ce qui VARIE d'un pixel à l'autre : le niveau d'obscurité et la sensibilité.
//     C'est de la dispersion de fabrication, elle est forcément par pixel.
//   - la FORME de la réponse (non-linéarité LED + capteur + ADC), qui est une propriété
//     de la chaîne, commune aux pixels d'une même couleur sur une même voie.
//
// On les sépare :
//     n   = clamp((raw - offset[p]) * gain[p] >> CIS_CAL_GAIN_SHIFT, 0, CURVE_MAX)
//     out = curve[couleur][voie][n]
//
// Conséquence décisive : le coût runtime est CONSTANT quel que soit le nombre de
// niveaux de calibration, puisque la courbe est tabulée une fois pour toutes. C'est
// ce qui permet de dépasser 3 points — l'ancien schéma coûtait deux tableaux par pixel
// de plus à chaque niveau ajouté, et la DTCM n'en avait plus les moyens.
// 11 bits (2048 entrees) et non 10 : avec 14 bits en entree la dynamique utile atteint
// ~3456 comptages, et renormaliser sur 1024 en jetterait la moitie. 2048 laisse 8 pas
// normalises entre deux codes de sortie consecutifs, assez fin pour que le bruit du
// capteur trame la sortie au lieu de la faire marcher.
#define CIS_CAL_CURVE_BITS                      (11)
#define CIS_CAL_CURVE_SIZE                      (1 << CIS_CAL_CURVE_BITS)  // 2048 entrées
#define CIS_CAL_CURVE_MAX                       (CIS_CAL_CURVE_SIZE - 1)

// Gains en Q4.12 : pas de 1/4096, soit ~0,02 % d'erreur relative sur un gain proche de
// 1, contre ~1 % avec l'ancien Q8.8 — c'est autant de bruit à motif fixe en moins.
// Plafond représentable 7,99, atteint seulement par un pixel dont la dynamique
// blanc-noir tomberait sous ~128 comptages ADC, c'est-à-dire un pixel mort.
#define CIS_CAL_GAIN_SHIFT                      (12)
#define CIS_CAL_GAIN_UNITY                      (1 << CIS_CAL_GAIN_SHIFT)

// Niveaux de stimulus, en % de rapport cyclique LED. Le PREMIER doit être 0 (trame
// d'obscurité -> offset par pixel) et le DERNIER 100 (blanc -> gain par pixel).
// Les intermédiaires ne servent qu'à la forme de la courbe : ils ne sont jamais
// conservés pixel à pixel, seulement moyennés par couleur et par voie. Le coût
// mémoire est donc indépendant de leur nombre.
#define CIS_CAL_LEVELS                          { 0, 15, 30, 45, 60, 80, 100 }
#define CIS_CAL_LEVEL_COUNT                     (7)

// Blanc et noir fixent gain et offset PAR PIXEL : ils méritent la moyenne longue.
// Un niveau intermédiaire ne produit que 9 scalaires, chacun déjà moyenné sur 1152
// pixels à chaque ligne : quelques centaines de lignes suffisent très largement, et
// ça raccourcit d'autant le geste demandé à l'opérateur.
#define CIS_CAL_ITER_ANCHOR                     (1000)
#define CIS_CAL_ITER_LEVEL                      (200)

// La disposition du fichier de calibration change. Sans marqueur, un fichier de
// l'ancien format serait relu comme des données valides (la lecture ne teste que la
// taille, et l'ancien fichier est plus GRAND) et produirait une image aberrante.
#define CIS_CAL_FILE_MAGIC                      (0x53503343UL)  /* "SP3C" */
#define CIS_CAL_FILE_VERSION                    (7UL)  /* tableaux canoniques R,G,B */

/* Asservissement thermique des LED.
   Mesure inter-boots : l'eclairement global derive de ~10 % entre deux etats
   thermiques (geometrie parfaitement stable par ailleurs : decalage 0, parite 0).
   Les pixels noirs masques ne voyant pas les LED, la correction de derive est aveugle
   a l'eclairement -- chaque calibration photographie un etat thermique. Remede sans
   cout par pixel : corriger le rapport cyclique des LED en fonction de l'ecart entre
   la temperature courante (IMU, publiee dans le PONG) et celle enregistree a la
   calibration. Coefficient en ppm de duty par milli-degre... unite pratique :
   ppm de duty par degre x100. 0 = asservissement coupe (defaut tant que le
   coefficient n'est pas mesure). */
#define CIS_LED_TEMP_COEFF_X100                 (0)

// Puissance LED de la capture "noir". 0 = LEDs réellement éteintes (PWM1 + CCR=0 ->
// sortie jamais active), donc vraie référence d'obscurité sans signal résiduel.
#define CIS_BLACK_LED_POWER                     (0)

// Garde-fou de mouvement pendant la calibration.
// Les références sont la moyenne de iterationNb lignes : si le capteur ne se déplace
// pas, on moyenne iterationNb fois LA MÊME portion de papier et son grain se retrouve
// gravé dans les gains par pixel, définitivement. Les lignes acquises à l'arrêt ne sont
// donc pas comptabilisées — la barre de progression se fige, ce qui dit à l'opérateur
// exactement quoi faire.
// Les seuils sont ceux, déjà réglables (SLP / web), du détecteur de mouvement de
// l'économiseur d'écran : shared_config.motion_threshold_acc / _gyro.
#define CIS_CAL_REQUIRE_MOTION                  (1)

/* Rafraichissement de l'ancre noire a chaque cis_startCapture : lignes moyennees,
   LEDs eteintes (~200 ms a 1000 lps). Motive par la mesure du 2026-08-31 : residu
   par pixel de 16-21 LSB14 entre offsets stockes et noir courant -- la derive DSNU
   depuis la calibration -- expliquant 50-70 %% du banding des teintes sombres. */
#define CIS_CAL_DARK_REFRESH_ITER               (200)

/* Journal RAM des moyennes PAR LIGNE (fenetre noire brute + sortie calibree, par
   couleur et par voie) : instrument du chantier banding horizontal, lu par SWD
   (symboles cisLineLog / cisLineLogHead). 768 lignes ~ 0,72 s a 1063 lps, assez
   pour voir un 100 Hz secteur (~10 lignes de periode). 0 en production. */
#define CIS_LINE_LOG_ENABLED                    (1)
#define CIS_LINE_LOG_N                          (768)

/* Reactivite de l'IIR de derive (1/2^n sur l'erreur ; l'etat reste en Q3, dont le
   >>3 de lecture est distinct). Historique : 3 (coupure ~20 Hz), choisi quand
   l'injection du bruit de fenetre noire dominait. Mesure du 2026-09-01 (journal par
   ligne) : le piedestal ondule a 42,5 Hz (periode 25 lignes, additive, presente dans
   les pixels masques, coherente inter-voies) et l'IIR 1/8 la chasse avec retard de
   phase -> banding horizontal, sortie anti-correlee a la fenetre noire (-0,3..-0,8).
   CONCLUSION du 2026-09-01 apres mesure du couplage (journal v2, actifs bruts) :
   k(noir->actifs) = -0,05..-0,40 -- les photodiodes ne voient PAS l'ondulation du
   piedestal (elle est dans la LUMIERE/l'analogique, voies extremes anti-correlees) ;
   accelerer l'IIR n'apporte rien et injecte le bruit de la fenetre noire. On reste
   a 3 : l'IIR ne corrige que sa vraie cible, la derive thermique. Le residuel
   ~0,3-0,5 %% a 40-56 Hz est un chantier MATERIEL (alimentation). */
#define CIS_DRIFT_IIR_SHIFT                     (3)
// Un échantillon IMU au-dessus du seuil garde la porte ouverte pendant ce temps : le
// mouvement est détecté par à-coups, sans rémanence la porte battrait à chaque échantillon.
#define CIS_CAL_MOTION_HOLD_MS                  (250)
// Abandon si les lignes en mouvement ne sont pas réunies dans ce délai (IMU muet,
// opérateur parti). La calibration échoue proprement sans écraser le fichier existant.
#define CIS_CAL_MOTION_TIMEOUT_MS               (60000)

// Les références de calibration (noir / intermédiaire / blanc) sont moyennées sur
// iterationNb lignes puis conservées en virgule fixe à CIS_CAL_FRAC_BITS bits de
// fraction, au lieu d'être tronquées au comptage ADC entier. Ça ne coûte rien au
// runtime (tout est reconverti en entier avant d'être stocké dans cisCals) mais ça
// rend le calcul des gains ~16x plus précis.
#define CIS_CAL_FRAC_BITS                       (4)
#define CIS_CAL_FRAC_SCALE                      (1 << CIS_CAL_FRAC_BITS)
#define CIS_CAL_FRAC_ROUND                      (1 << (CIS_CAL_FRAC_BITS - 1))

// Format Q8.8 pour les gains (optimisation mémoire)
#define UNITY_Q8_8                              (1 << 8)    // 1.0 en format Q8.8
#define CLIP_INT16(x)                           ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

/**************************************************************************************/
/***                            Packet Management Definitions                      ***/
/**************************************************************************************/

// Number of UDP packets per line
#define UDP_MAX_NB_PACKET_PER_LINE              (12)

// Ensure UDP_LINE_FRAGMENT_SIZE is an integer
#if (CIS_MAX_PIXELS_NB % UDP_MAX_NB_PACKET_PER_LINE) != 0
  #error "CIS_MAX_PIXELS_NB must be divisible by UDP_NB_PACKET_PER_LINE."
#endif

// Size of each UDP line fragment (number of pixels per packet)
#define UDP_LINE_FRAGMENT_SIZE                  (CIS_MAX_PIXELS_NB / UDP_MAX_NB_PACKET_PER_LINE)


/**************************************************************************************/
/********************              GYRO definitions                ********************/
/**************************************************************************************/
#define ICM42688P

#define IMU_CLKIN_FREQ			                (32000)

// Gyroscope sensitivity configuration for handheld usage
// Available options: dps2000, dps1000, dps500, dps250, dps125, dps62_5, dps31_25, dps15_625
#define DEFAULT_GYRO_SENSITIVITY                dps250  // Lower sensitivity for better precision in handheld use

// Accelerometer sensitivity configuration for handheld usage
// Available options: gpm16, gpm8, gpm4, gpm2
#define DEFAULT_ACCEL_SENSITIVITY		        gpm4    // Moderate sensitivity for handheld movement detection

// Calibration sample count for handheld usage (reduced for faster startup)
#define HANDHELD_CALIB_SAMPLES				    (50)      // Reduced from 100 for faster calibration

#endif // __CONFIG_H__

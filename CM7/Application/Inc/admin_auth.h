/**
 ******************************************************************************
 * @file           : admin_auth.h
 * @brief          : Identifiants d'administration protegeant toute requete qui
 *                   MODIFIE l'appareil.
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

#ifndef __ADMIN_AUTH_H__
#define __ADMIN_AUTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "lwip/api.h"

/* Nom d'utilisateur unique : la seule chose secrete est le mot de passe. */
#define ADMIN_AUTH_USER "admin"

/* Prepare les identifiants au demarrage.
 *
 * Si la configuration n'en porte pas encore -- machine neuve, ou sortie de
 * factory reset -- un mot de passe est tire au sort et enregistre. Il est
 * ensuite publie pour l'ecran du CM4, qui est le SEUL moyen de le decouvrir :
 * il ne doit jamais pouvoir etre lu par le reseau, faute de quoi il ne
 * protegerait plus rien.
 *
 * A appeler juste apres file_initConfig(). */
void adminAuth_init(void);

/* Vrai si la requete porte un en-tete Authorization: Basic valide.
 * `request` doit etre termine par un NUL. */
bool adminAuth_check(const char *request);

/* Repond 401 avec le defi Basic. */
void adminAuth_sendChallenge(struct netconn *conn);

/* Le mot de passe vient de servir : l'ecran de demarrage cesse de l'afficher.
 * Sans cela il resterait expose a quiconque passe devant la machine. */
void adminAuth_markUsed(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADMIN_AUTH_H__ */

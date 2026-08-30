/**
 ******************************************************************************
 * @file           : admin_auth.c
 * @brief          : Identifiants d'administration.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rng.h"

#include "admin_auth.h"
#include "config.h"
#include "file_manager.h"
#include "globals.h"

#include <stdio.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
#define AUTH_HEADER    "Authorization: Basic "
#define AUTH_MAX_CREDS 64

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Decode une chaine base64 (sans espaces ni retours a la ligne).
 * @retval Longueur decodee, ou -1 si un caractere invalide apparait.
 */
static int adminAuth_base64Decode(const char *in, size_t inLen, uint8_t *out, size_t outMax)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    uint32_t acc = 0;
    int bits = 0;
    size_t written = 0;

    for (size_t i = 0; i < inLen; i++)
    {
        const char c = in[i];

        if (c == '=')
        {
            break;
        }

        const char *p = memchr(alphabet, c, 64);
        if (p == NULL)
        {
            return -1;
        }

        acc = (acc << 6) | (uint32_t)(p - alphabet);
        bits += 6;

        if (bits >= 8)
        {
            bits -= 8;
            if (written >= outMax)
            {
                return -1;
            }
            out[written++] = (uint8_t)((acc >> bits) & 0xFFu);
        }
    }

    return (int)written;
}

/**
 * @brief  Comparaison sans sortie anticipee.
 *
 * Le mot de passe fait 60 bits et l'appareil vit sur un reseau local, donc une
 * attaque temporelle est theorique ; la garantie coute trois lignes, autant la
 * prendre.
 */
static bool adminAuth_equals(const char *a, const char *b, size_t len)
{
    uint8_t diff = 0;

    for (size_t i = 0; i < len; i++)
    {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }
    return diff == 0;
}

/**
 * @brief  Tire un mot de passe au sort dans l'alphabet non ambigu.
 */
static void adminAuth_generate(char *out)
{
    static const char alphabet[] = ADMIN_PASSWORD_ALPHABET;
    const uint32_t alphabetLen = (uint32_t)(sizeof(alphabet) - 1u);

    for (uint32_t i = 0; i < ADMIN_PASSWORD_LEN; i++)
    {
        uint32_t r = 0;

        if (HAL_RNG_GenerateRandomNumber(&hrng, &r) != HAL_OK)
        {
            /* Sans alea, mieux vaut ne rien ecrire qu'un mot de passe
             * previsible : l'appelant verra une chaine vide et refusera de
             * demarrer le service protege. */
            out[0] = '\0';
            return;
        }

        out[i] = alphabet[r % alphabetLen];
    }
    out[ADMIN_PASSWORD_LEN] = '\0';
}

/* Exported functions --------------------------------------------------------*/

void adminAuth_init(void)
{
    if (shared_config.admin_password[0] == '\0')
    {
        char generated[ADMIN_PASSWORD_LEN + 1];

        adminAuth_generate(generated);
        if (generated[0] == '\0')
        {
            printf("ADMIN: RNG failure, no password generated\n");
            return;
        }

        memcpy((void *)shared_config.admin_password, generated, sizeof(generated));
        shared_config.admin_password_ack = 0;

        if (file_writeConfig(CONFIG_FILE_PATH, &shared_config) != FILEMANAGER_OK)
        {
            printf("ADMIN: failed to persist the generated password\n");
        }
        printf("ADMIN: password generated on first boot\n");
    }

    /* Publie pour l'ecran du CM4 tant qu'il n'a jamais servi. L'ecran est le
     * seul chemin de decouverte : le mot de passe ne sort jamais par le
     * reseau, sans quoi la protection serait vide de sens. */
    memcpy((void *)shared_feedback.admin_password,
           (const void *)shared_config.admin_password,
           ADMIN_PASSWORD_LEN + 1);
    shared_feedback.admin_show_password = (shared_config.admin_password_ack == 0) ? 1u : 0u;

    if (shared_config.admin_password_ack == 0)
    {
        /* Trace serie tant que le mot de passe n'a jamais servi : l'UART est
         * derriere le capot, donc au meme niveau de confiance que la dalle
         * OLED. C'est ce qui rend le banc de test utilisable. */
        printf("ADMIN: password %s (shown on the boot screen until first use)\n",
               (const char *)shared_config.admin_password);
    }
    else
    {
        printf("ADMIN: authentication enabled, password already used\n");
    }
}

bool adminAuth_check(const char *request)
{
    if (shared_config.admin_password[0] == '\0')
    {
        /* Aucun mot de passe utilisable : on refuse plutot que d'ouvrir. */
        return false;
    }

    const char *header = strstr(request, AUTH_HEADER);
    if (header == NULL)
    {
        return false;
    }
    header += strlen(AUTH_HEADER);

    /* La valeur base64 s'arrete au premier caractere de fin de ligne. */
    size_t encodedLen = 0;
    while (header[encodedLen] != '\0' && header[encodedLen] != '\r' &&
           header[encodedLen] != '\n' && header[encodedLen] != ' ')
    {
        encodedLen++;
    }

    uint8_t decoded[AUTH_MAX_CREDS + 1];
    const int decodedLen = adminAuth_base64Decode(header, encodedLen, decoded, AUTH_MAX_CREDS);
    if (decodedLen <= 0)
    {
        return false;
    }
    decoded[decodedLen] = '\0';

    char expected[sizeof(ADMIN_AUTH_USER) + 1 + ADMIN_PASSWORD_LEN + 1];
    const int expectedLen = snprintf(expected, sizeof(expected), "%s:%s", ADMIN_AUTH_USER,
                                     (const char *)shared_config.admin_password);

    if (expectedLen <= 0 || decodedLen != expectedLen)
    {
        return false;
    }

    return adminAuth_equals((const char *)decoded, expected, (size_t)expectedLen);
}

void adminAuth_sendChallenge(struct netconn *conn)
{
    const char *response =
        "HTTP/1.1 401 Unauthorized\r\n"
        "WWW-Authenticate: Basic realm=\"Sp3ctra\"\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 62\r\n"
        "\r\n"
        "Administrator credentials required for any change to the device";

    netconn_write(conn, response, strlen(response), NETCONN_COPY);
}

void adminAuth_markUsed(void)
{
    if (shared_config.admin_password_ack != 0)
    {
        return;
    }

    shared_config.admin_password_ack = 1;
    shared_feedback.admin_show_password = 0;

    if (file_writeConfig(CONFIG_FILE_PATH, &shared_config) != FILEMANAGER_OK)
    {
        printf("ADMIN: failed to persist the password acknowledgement\n");
    }
    else
    {
        printf("ADMIN: password used, no longer shown on the boot screen\n");
    }
}

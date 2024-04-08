#include "ff.h"

#if !FF_FS_READONLY
/* Convertir un caractère Unicode en majuscule */
WCHAR ff_wtoupper (WCHAR chr)
{
    /* Votre implémentation ici, par exemple: */
    if (chr >= 'a' && chr <= 'z') {
        chr -= 'a' - 'A';
    }
    return chr;
}
#endif

/* Convertir un caractère Unicode en OEM ou OEM en Unicode */
WCHAR ff_convert (WCHAR chr, UINT dir)
{
    /* Votre implémentation ici. La conversion dépend de la page de code OEM utilisée.
       `dir` indique la direction de la conversion : 1 pour Unicode vers OEM, 0 pour OEM vers Unicode. */
    return chr;
}

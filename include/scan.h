#ifndef _SCAN_H_
#define _SCAN_H_

#include "globals.h"

/* Variáveis globais do scanner */
extern char tokenString[256];

/* Função principal do scanner */
TokenType getToken(void);

#endif
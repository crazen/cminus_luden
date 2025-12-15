1. Introdução e Objetivo
Este projeto consiste na implementação completa de um compilador para a linguagem C- (C-Minus), um subconjunto da linguagem C, conforme especificado no Apêndice A do livro "Compiler Construction: Principles and Practice" de Kenneth C. Louden.

O objetivo foi desenvolver todas as fases do processo de compilação — análise léxica, sintática, semântica e geração de código — transformando o código-fonte de alto nível em código assembly para a Tiny Machine (TM).

O compilador foi desenvolvido em linguagem C, utilizando as ferramentas Flex (para o scanner) e Bison (para o parser), e atende a todos os requisitos funcionais e de robustez estipulados na especificação do projeto.

2. Arquitetura do Sistema
O compilador opera em uma arquitetura de passagem única modificada (Single-Pass Architecture with AST), estruturada nos seguintes módulos:

Scanner (Frontend): Lê o fluxo de caracteres e produz tokens.

Parser (Frontend): Consome tokens e constrói a Árvore Sintática Abstrata (AST).

Analisador Semântico (Middle-end): Percorre a AST para construir a Tabela de Símbolos, verificar escopos e validar tipos.

Gerador de Código (Backend): Percorre a AST anotada para emitir instruções de máquina baseadas em pilha.

3. Detalhamento dos Módulos e Implementação
3.1. Análise Léxica (src/cminus.l)
Este módulo é responsável por reconhecer os lexemas da linguagem e categorizá-los em tokens.

Ferramenta: Flex.

Funcionalidades:

Identificação de palavras-chave (if, else, int, return, void, while).

Reconhecimento de identificadores (ID) e números (NUM).

Tratamento de comentários aninhados e de múltiplas linhas utilizando estados exclusivos do Flex (%x COMMENT).


Tratamento de Erros: Implementa uma regra "catch-all" (ponto .) que captura caracteres inválidos e emite a mensagem padronizada ERRO LEXICO: 'lexema' - LINHA: X, interrompendo a compilação de forma segura.

Compatibilidade: A regra de espaços em branco foi ajustada ([ \t\r]+) para ignorar corretamente o caractere de retorno de carro (\r), garantindo compatibilidade com arquivos editados no Windows.

3.2. Análise Sintática (src/cminus.y)
Este módulo valida a estrutura gramatical do código e constrói a AST.

Ferramenta: Bison.

Construção da AST: A cada regra gramatical reduzida, nós da árvore (TreeNode) são alocados e conectados dinamicamente.

Resolução de Conflitos e Desafios:

Ambiguidade do void: A gramática foi ajustada para distinguir explicitamente params -> VOID (lista vazia) de param -> INT ID, resolvendo conflitos de shift/reduce.

Declarações Múltiplas: Foi implementada a lógica de fatoração para permitir a declaração de múltiplas variáveis na mesma linha (ex: int x, y, z;), recurso não presente na gramática original do Louden.

Declaração de Vetores: A gramática foi fatorada (declaration_suffix) para distinguir corretamente entre variáveis simples (int x;), vetores (int v[10];) e funções (int f(...)), evitando erros de sintaxe no início da declaração.


Tratamento de Erros: Utiliza a função yyerror para reportar falhas gramaticais no formato ERRO SINTATICO: ... LINHA: X.

3.3. Tabela de Símbolos (src/symtab.c)
Estrutura central para o armazenamento de metadados das variáveis e funções.

Estrutura de Dados: Utiliza uma Tabela Hash com Encadeamento para armazenamento eficiente.

Gerenciamento de Escopo: Implementa uma Pilha de Escopos (Scope Stack).

Ao entrar em uma função ou bloco, um novo escopo é empilhado (scope_push).

Ao sair, o escopo é desempilhado (scope_pop), mas mantido em histórico para impressão final.


Atributos Registrados: Nome, Tipo de Dado (Integer/Void), Localização de Memória (Offset) e Escopo de Declaração.

3.4. Análise Semântica (src/analyze.c)
Realiza a validação lógica do programa em duas passadas pela AST.

Primeira Passada (Construção e Alocação):

Insere identificadores na Tabela de Símbolos.

Verifica declarações duplicadas no mesmo escopo.

Calcula o tamanho de memória necessário (ex: vetores v[5] incrementam o contador de memória em 5 posições).

Propagação de Tipos: Anota os nós da árvore com os tipos recuperados da tabela, essencial para a verificação posterior.

Segunda Passada (Checagem de Tipos):

Verifica se todas as variáveis utilizadas foram declaradas.

Valida compatibilidade de tipos (ex: impede operações aritméticas com void ou atribuições inválidas).

Verifica a existência da função main.

3.5. Geração de Código (src/cgen.c)
Este é o módulo mais avançado do projeto. Ele traduz a AST para instruções Assembly da Tiny Machine.

Modelo de Execução: Implementa uma Máquina de Pilha (Stack Machine), superando a alocação estática simples.

Registradores Utilizados:

GP (Reg 5): Ponteiro para variáveis globais.

MP (Reg 6): Memory Pointer (aponta para o topo do registro de ativação da função atual).

PC (Reg 7): Contador de Programa.

AC (Reg 0): Acumulador.

Suporte a Recursão:

Cada chamada de função (CallK) cria dinamicamente um novo Registro de Ativação (Stack Frame) na pilha.

O compilador salva automaticamente o endereço de retorno e o antigo valor de MP antes de pular para a função.

Isso permite recursão profunda (como no teste do Fatorial e Fibonacci).

Suporte a Vetores:

O endereço de acesso a vetores (v[i]) é calculado em tempo de execução: Endereço = EndereçoBase + Valor(i).

Backpatching:

Utiliza uma lista encadeada para resolver endereços de chamadas de função que ainda não foram compiladas no momento da leitura (essencial para compilação em passada única).

4. Testes e Validação
A robustez do compilador foi verificada através de um conjunto abrangente de testes:

Testes de Erro:

Léxico: Caracteres inválidos (strings, char) foram rejeitados corretamente.

Sintático: Erros de pontuação e estrutura foram detectados.

Semântico: Uso de variáveis não declaradas e tipagem incorreta foram barrados.

Testes Funcionais:

Aritmética: Expressões complexas respeitando precedência.

Controle de Fluxo: Loops while e condicionais if-else aninhados.

Vetores: O teste de soma de vetores (teste7.c-) provou que a alocação e o acesso indexado à memória estão corretos.

Recursão: O teste do Fatorial (teste.c-) calculou 5! = 120 corretamente, validando o gerenciamento de pilha.

Escopo: O teste de shadowing provou que variáveis locais em blocos internos não sobrescrevem variáveis de mesmo nome em escopos externos.

5. Conclusão
O compilador desenvolvido atende integralmente aos requisitos propostos. Além das funcionalidades básicas, o projeto destaca-se pela implementação de um ambiente de execução baseado em pilha, permitindo o suporte completo a recursos avançados da linguagem C- como recursividade e vetores locais, gerando código intermediário correto e executável.

6. Instruções de Compilação e Execução
Para compilar o projeto, utilizamos o Makefile fornecido:

Bash

# Limpar arquivos antigos e compilar o compilador
make clean
make

# Compilar o simulador (necessário para rodar o código gerado)
gcc tm.c -o tm
Exemplo de Uso:

Bash

# 1. Compilar um arquivo fonte C-
./cminus tests/teste_exemplo.c-

# 2. Executar o código gerado no simulador
# (O 'echo' fornece os inputs automaticamente)
echo -e "g\n10\nq" | ./tm output/teste_exemplo.tm

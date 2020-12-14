// 文字の処理を行う関数群: isspace など
#include <ctype.h>
// 可変長引数の関数群: va_start など
#include <stdarg.h>
// 真偽値 true, false
#include <stdbool.h>
// 入出力 printf など
#include <stdio.h> 
// 汎用 malloc, rand など
#include <stdlib.h>
// 文字列の処理: strcpy など
#include <string.h>

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ, // ==
  ND_NEQ, // !=
  ND_LT, // <
  ND_LTE, // <=
  ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺
  Node *rhs;     // 右辺
  int val;       // kindがND_NUMの場合のみ使う
};

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // 文字列の開始位置
  int len;        // 文字列の長さ
};

Token *tokenize(char *p);

// 入力プログラム の宣言
extern char *user_input;

// 現在着目しているトークン の宣言
extern Token *token;

Node *expr();
void gen(Node *node);
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
  ND_ASSIGN, // =
  ND_LVAR,   // ローカル変数
  ND_RETURN, // return
  ND_IF, // if
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺。if のときは then 式
  Node *rhs;     // 右辺。if のときは else 式
  Node *cond;    // if のときだけ使う。条件式。
  int val;       // kindがND_NUMの場合のみ使う
  int offset;    // kindがND_LVARの場合のみ使う。RBPからその変数へのオフセット。
};

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_RETURN,   // return
  TK_IF,       // if
  TK_ELSE,     // else
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

void program();
void gen(Node *node);

// プログラムを構成する文の並びを入れておく
Node *code[100];

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...);

// ローカル変数 Local Var を LVar という型で表す
typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next; // 次の変数かNULL
  char *name; // 変数の名前
  int len;    // 名前の長さ
  int offset; // RBPからのオフセット
};

// ローカル変数リストの先頭アドレスを覚えておく
// 新しい要素は先頭につないでいくので、先頭アドレスは最後に足した要素を指す
LVar *locals;
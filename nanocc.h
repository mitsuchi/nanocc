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

// 値の型
struct Type {
  enum { INT, PTR, ARRAY } kind;
  struct Type *ptr_to;
  size_t array_size; // 配列のときのみ使う。配列の要素数。
};
typedef struct Type Type;

// ローカル変数 Local Var を LVar という型で表す
typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next; // 次の変数かNULL
  char *name; // 変数の名前
  int len;    // 名前の長さ
  int offset; // RBPからのオフセット
  Type *type;  // 変数の型
};

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
  ND_WHILE, // while
  ND_FOR, // for
  ND_BLOCK, // ブロック
  ND_CALL, // 関数呼び出し
  ND_FUNC_DEF, // 関数定義
  ND_PARAM, // 仮引数
  ND_ADDR, // & 変数のアドレス
  ND_DEREF, // * デリファレンス
  ND_DECL, // 変数宣言
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺。if のときは then 式。while のときは本体。for なら初期化式。
  Node *rhs;     // 右辺。if のときは else 式。for では増加式。
  Node *cond;    // if と while, for のときは条件式。
  Node *body;    // while と関数定義のときのみ使う。本体。
  Node *next;    // ブロック のときのみ使う。次の文へのポインタ。
  int val;       // kindがND_NUMの場合のみ使う
  int offset;    // kindがND_LVARの場合のみ使う。RBPからその変数へのオフセット。
  Type *type;    // 式の場合のみ使う。その式が表す値の型。
  char *str;     // 関数呼び出しと定義のときだけ使う。関数名の文字列の開始位置
  int len;       // 関数呼び出しと定義のときだけ使う。関数名の文字列の長さ
  Node *args[6];  // 関数呼び出しのときは、実引数を入れる、最大6つ分の配列
                  // 関数定義のときは、仮引数を入れる。
  int argc;       // 関数呼び出しのときは、実引数の個数。
                  // 関数定義のときは、仮引数の個数
  LVar *locals;   // 関数定義の際に、ローカル変数のリストの先頭を指す
                  // 新しい要素は先頭につないでいくので、先頭アドレスは最後に足した要素を指す
};

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_RETURN,   // return
  TK_IF,       // if
  TK_ELSE,     // else
  TK_WHILE,    // while
  TK_FOR,      // for
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
  TK_INT,      // int
  TK_SIZEOF,   // sizeof 演算子
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

// トップレベルにある関数定義の並びを入れておく
Node *func_defs[100];

// いまパーズ中の関数定義のノードを入れておく
Node *cur_func;

// その型の値を持つのに必要なサイズ
int value_size (int kind);

// エラー出力
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// tokenizer
bool at_eof();
bool consume(char *op);
bool consume_reserved(int kind);
void expect_rword(int kind, char *rword);
Token *consume_ident();
Token *expect_ident();
void expect(char *op);
int expect_number();

// type
Type *new_type(int kind);
Type *append_type(int kind, Type **head, Type **tail);

// var
LVar *find_lvar(Token *tok);
void register_var(char *str, int len, Type *type);

// node
Node *new_node_unary(NodeKind kind, Node *lhs);
Node *new_node_bin(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node(NodeKind kind);
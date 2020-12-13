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
  char *str;      // トークン文字列の開始位置
};

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// エラー箇所を報告する
// エラー箇所を指すポインタと、残りは printfと同じ引数を取る
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_at(token->str, "'%c'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

// argc はコンパイラーへの引数の数(+1)
// argv は引数の文字列の先頭へのポインターを納めた配列へのポインター
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // エラー表示に使うためにプログラムの先頭を指しておく
  user_input = argv[1];

  // トークナイズする
  token = tokenize(argv[1]);

  // intel記法を使う
  printf(".intel_syntax noprefix\n");
  // main をリンク時に外のファイルから見れるようにする
  printf(".globl main\n");
  // main ラベル
  printf("main:\n");

  // 式の最初は数でなければならないので、それをチェックして
  // 最初のmov命令を出力
  // mov rax : rax レジスタに値をセットする
  printf("  mov rax, %d\n", expect_number());

  // `+ <数>`あるいは`- <数>`というトークンの並びを消費しつつ
  // アセンブリを出力
  while (!at_eof()) { // eof でない限り
    if (consume('+')) { // 次のトークンが + であれば消費する
      // 次のトークンとして数を期待して消費し、足し算命令を出力する
      printf("  add rax, %d\n", expect_number());
      continue;
    }

    // + でなければ - が来るしかないのでそれを期待して消費する
    expect('-');
    // 次のトークンとして数を期待して消費し、引き算命令を出力する
    printf("  sub rax, %d\n", expect_number());
  }

  // rax レジスタの値を返す
  printf("  ret\n");
  // 正常終了コードを返す
  return 0;
}
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

    if ( *p == '+' || *p == '-'
      || *p == '*' || *p == '/'
      || *p == '(' || *p == ')'
    ) {
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

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
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

// 二項演算のASTノードを作る
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  // Node1つぶんのメモリを確保して0でクリアする
  Node *node = calloc(1, sizeof(Node));
  // ノードの種類: 足し算か数字かなど
  node->kind = kind;
  // 左辺
  node->lhs = lhs;
  // 右辺
  node->rhs = rhs;
  return node;
}

// 数字のASTノードを作る
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *num();
Node *mul();
Node *unary();
Node *primary();

// 全体の EBNF
// expr = mul ('+' mul | '-' mul)*
// mul = unary ('*' unary | '/' unary)*
// unary   = ("+" | "-")? primary
// primary = '(' expr ')' | num
// num := [1-9][0-9]*

// 式をパーズする
// expr = mul ('+' mul | '-' mul)*
Node *expr() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_node(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// 掛け算と割り算の項をパーズする
// mul = unary ('*' unary | '*' unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_node(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// 単項演算子を含むかもしれない項をパーズする
// unary   = ("+" | "-")? primary
Node *unary() {
  if (consume('+')) {
    // 単項 + 演算子は実質なにもしない
    return primary();
  }
  if (consume('-')) {
    // 単項 - 演算子は 0 - primary に変換する
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  // 単項演算子がなければただの primary
  return primary();
}

// 数値かカッコ式をパーズする
// primary = '(' expr ')' | num
Node *primary() {
  // カッコが来てればカッコに挟まれた式
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  // そうでなければ数値のはず
  return num();
}

// 数値をパーズする
// num := [1-9][0-9]*
Node *num() {
  // 次のトークンとして数値を期待して消費し、その数値を取得して数値ノードを作る
  return new_node_num(expect_number());
}

// ASTからアセンブリを出力する
void gen(Node *node) {
  // 数値なら push する
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  // 二項演算なら左辺と右辺がそれぞれ最終的に
  // スタックトップに push されるようなコードを生成する
  gen(node->lhs);
  gen(node->rhs);

  // スタックを pop し、先頭を rdi, 2番めを rax に入れる
  // 左辺、右辺の順でコード生成しているので、先頭が右辺で、2番めが左辺になる
  // 64bitレジスタは rax, rdi, rsi, rdx, rcx, rbp, rsp, rbx, r8, r9, ..
  // のような順序で使うらしい
  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    // 左辺 - 右辺
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    // imul は素直な掛け算
    // mul は mul src	=> RDX:RAX = RAX * src となる128bitの掛け算
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    // 割り算は 128bit に拡張するやつしかない
    // idiv は次のようになる
    // RAX = RDX:RAX / r64
    // RDX = RDX:RAX % r64
    // なので cqo で RAXを128ビットに符号拡張してRDX:RAXにストア する
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
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

  // 全体を式として構文解析する
  Node *node = expr();

  // intel記法を使う
  printf(".intel_syntax noprefix\n");
  // main をリンク時に外のファイルから見れるようにする
  printf(".globl main\n");
  // main ラベル
  printf("main:\n");

  // 抽象構文木を下りながらコード生成
  gen(node);

  // スタックトップに式全体の値が残っているはずなので
  // それをRAXにロードして関数からの返り値とする
  printf("  pop rax\n");
  // rax レジスタの値を返す
  printf("  ret\n");
  // 正常終了コードを返す
  return 0;
}
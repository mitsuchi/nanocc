#include "nanocc.h"

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
bool consume(char *op) {
  // トークンの種類がそもそも記号でないか
  if (token->kind != TK_RESERVED
    // 記号列の長さがトークン文字列の長さと違うか
    || strlen(op) != token->len
    // 記号列がトークン文字列と違う
    || memcmp(token->str, op, token->len))
    // なら期待する記号ではない
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  // トークンの種類がそもそも記号でないか
  if (token->kind != TK_RESERVED
    // 記号列の長さがトークン文字列の長さと違うか
    || strlen(op) != token->len
    // 記号列がトークン文字列と違う
    || memcmp(token->str, op, token->len))
    // なら期待する記号ではない
    error_at(token->str, "'%s'ではありません", *op);
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

bool starts_with(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
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
    // 2文字の記号
    if (starts_with(p, "==") || starts_with(p, "!=")
      || starts_with(p, "<=") || starts_with(p, ">=")
    ) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }
    // 1文字の記号
    if (strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      // ポインタが進んだ分が桁数
      cur->len = p - q;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

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

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *num();

// 全体の EBNF
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"
// num := [1-9][0-9]*

// 式をパーズする
// expr       = equality
Node *expr() {
  return equality();
}

// 等式または不等式をパーズする
// equality   = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NEQ, node, relational());
    else
      return node;
  }
}

// 不等式をパーズする
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume("<="))
      node = new_node(ND_LTE, node, add());
    else if (consume(">"))
      // > は左辺と右辺を逆転した < としてしまう
      node = new_node(ND_LT, add(), node);
    else if (consume(">="))
      // >= は左辺と右辺を逆転した <= としてしまう
      node = new_node(ND_LTE, add(), node);
    else
      return node;
  }
}

// 足し算と引き算の項をパーズする
// add        = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
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
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// 単項演算子を含むかもしれない項をパーズする
// unary   = ("+" | "-")? primary
Node *unary() {
  if (consume("+")) {
    // 単項 + 演算子は実質なにもしない
    return primary();
  }
  if (consume("-")) {
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
  if (consume("(")) {
    Node *node = expr();
    expect(")");
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
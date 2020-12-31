#include "nanocc.h"

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

// 次のトークンが期待している予約語のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_reserved(int kind) {
  // トークンの種類がそもそも記号でないか
  if (token->kind == kind) {
    token = token->next;
    return true;
  }
  return false;
}

// 次のトークンが指定した予約語の場合、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect_rword(int kind, char *rword) {
  if (token->kind != kind) {
    char msg[255];
    strncpy(msg, token->str, token->len);
    msg[token->len] = '\0';
    error_at(token->str, "予約語 %s ではありません", rword);
  }
  token = token->next;
}

// 次のトークンが識別子のときには、トークンを1つ読み進めてから
// もとの識別子トークンを返す。そうでない場合には NULL を返す。
Token *consume_ident() {
  // トークンの種類が識別子なら
  if (token->kind == TK_IDENT) {
    Token *ident_token = token;
    token = token->next;
    return ident_token;
  }

  return NULL;
}

// 次のトークンが識別子の場合、トークンを1つ読み進めてその数値を返す。
// 識別子トークンを返す。それ以外の場合にはエラーを報告する。
Token *expect_ident() {
  if (token->kind != TK_IDENT)
    error_at(token->str, "識別子ではありません");
  Token *ident_token = token;
  token = token->next;
  return ident_token;
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

int is_alnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         (c == '_');
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
    if (strchr("+-*/(){}<>=;,&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
    // return だったらそれを返す
    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }
    // if だったらそれを返す
    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }
    // else だったらそれを返す
    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }
    // while 
    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }
    // for
    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }
    // int
    if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_INT, cur, p, 3);
      p += 3;
      continue;
    }
    // sizeof
    if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_SIZEOF, cur, p, 6);
      p += 6;
      continue;
    }
    // 1文字のアルファベットを見つけたら
    if ('a' <= *p && *p <= 'z') {
      // 開始位置を覚えておいて
      char *p0 = p;
      p++;
      // アルファベットが続く限り読み進める
      while ('a' <= *p && *p <= 'z') {
        p++;
      }
      // p0 から p - p0 文字の長さの名前の識別子トークンとして追加する
      cur = new_token(TK_IDENT, cur, p0, p - p0);
      cur->len = p - p0;
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
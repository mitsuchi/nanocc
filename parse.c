#include "nanocc.h"

// 全体の EBNF
// program    = func_def*
// func_def   = ident ("(" ident? ("," ident)* ")")? "{" stmt* "}"
// stmt       = expr ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
//            | "*" unary
//            | "&" unary
// primary    = num
//            | ident
//            | ident ("(" expr? ("," expr)* ")")?
//            | "(" expr ")"

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
  // 変数名のリストを先頭から順に見ていって
  for (LVar *var = cur_func->locals; var; var = var->next)
    // 既存のものと長さが一緒で文字列が一緒ならそれを返す
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  // 見つからなければNULLを返す
  return NULL;
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

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
    if (strchr("+-*/(){}<>=;,&", *p)) {
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

Node *func_def();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *num();
void register_var(char *str, int len);

// プログラムをパーズする
// program    = func_def*
void program() {
  int i = 0;
  while (!at_eof())
    func_defs[i++] = func_def();
  func_defs[i] = NULL;
}

// 関数定義をパーズする
// func_def   = ident ("(" ident? ("," ident)* ")")? "{" stmt* "}"
Node *func_def() {
  // 識別子がくるはず
  Token *tok = expect_ident();

  // "(" が来るはず
  expect("(");

  // 関数定義のノードを作る
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNC_DEF;
  node->str = tok->str; // 関数名
  node->len = tok->len; // 関数名の長さ
  // 現在処理中の関数としてグローバルに持っておく
  cur_func = node;
  int i = 0;
  // (ident ("," ident)*)? ")")
  // 次が ")" でないのなら識別子が続く
  while (!consume(")")) {
    // 次は識別子のはず
    tok = expect_ident();
    // 仮引数の文字列を入れる領域を確保する
    Node *param = calloc(1, sizeof(Node));
    // ASTノードの種類を仮引数とする
    param->kind = ND_PARAM;
    // 仮引数名はトークンが持つ値をそのまま使う
    param->str = tok->str;
    // 仮引数名の長さも同じ
    param->len = tok->len;
    // 仮引数を関数定義に追加する
    node->args[i++] = param;
    // "," が来たら読み捨てる
    consume(",");
    // 仮引数名を変数リストに追加する
    register_var(tok->str, tok->len);
  }
  node->argc = i;

  // ブロックが来るはず
  expect("{");
  // ブロックを表すノードを用意する
  // node->next で複数の文をつないでいく
  Node *block = new_node(ND_BLOCK, NULL, NULL);
  // 文のリストの最後を指しておく
  Node *last = block;
  while (!consume("}")) {
    // 文を1つパーズしてノードをつくる
    Node *cur_node = stmt();
    // それを文のリストにつなげる
    last->next = cur_node;
    // 最後を交代する
    last = cur_node;
  }
  // 終末の次はNULLにしておく
  last->next = NULL;
  // 関数定義の本体をブロックにする
  node->body = block;
  return node;
}

// 変数名をリストに追加する
void register_var(char *str, int len) {
  LVar *lvar = calloc(1, sizeof(LVar));
  // 新しい要素を先頭につなぐ
  lvar->next = cur_func->locals;
  // 変数名はトークンが持つ値をそのまま使う
  lvar->name = str;
  // 変数名の長さも同じ
  lvar->len = len;
  // 変数名のスタックベースからのオフセットは、
  // 最後に追加された変数のオフセット + 8にする
  if (cur_func->locals) {
    lvar->offset = cur_func->locals->offset + 8;
  } else {
    // 最初に見つかった変数ならオフセットは 8 にする
    lvar->offset = 8;
  }
  // 変数リストの先頭アドレスをいま追加したものとする
  cur_func->locals = lvar;
}

// 文をパーズする
// stmt       = expr ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
Node *stmt() {
  Node *node;
  // block
  if (consume("{")) {
    // ブロックを表すノードを用意する
    // node->next で複数の文をつないでいく
    node = new_node(ND_BLOCK, NULL, NULL);
    // 文のリストの最後を指しておく
    Node *last = node;
    while (!consume("}")) {
      // 文を1つパーズしてノードをつくる
      Node *cur_node = stmt();
      // それを文のリストにつなげる
      last->next = cur_node;
      // 最後を交代する
      last = cur_node;
    }
    // 終末の次はNULLにしておく
    last->next = NULL;
  // return
  } else if (consume_reserved(TK_RETURN)) {
    node = new_node(ND_RETURN, expr(), NULL);
    expect(";");
  // if
  } else if (consume_reserved(TK_IF)) {
    node = new_node(ND_IF, NULL, NULL);
    expect("(");
    node->cond = expr();
    expect(")");
    node->lhs = stmt();
    if (consume_reserved(TK_ELSE)) {
      node->rhs = stmt();
    }
  // while
  } else if (consume_reserved(TK_WHILE)) {
    node = new_node(ND_WHILE, NULL, NULL);
    expect("(");
    node->cond = expr();
    expect(")");
    node->lhs = stmt();
  // for 
  // "for" "(" expr? ";" expr? ";" expr? ")" stmt
  } else if (consume_reserved(TK_FOR)) {
    node = new_node(ND_FOR, NULL, NULL);
    expect("(");
    if (!consume(";")) {
      node->lhs = expr(); // lhs に初期化式を入れる
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr(); // cond にループ条件を入れる
      expect(";");
    } else {
      // 条件式が空だったら常に真で1にしておく
      node->cond = new_node_num(1);
    }
    if (!consume(")")) {
      node->rhs = expr(); // rhs に増加式を入れる
      expect(")");
    }
    node->body = stmt();
  } else {
    node = expr();
    expect(";");
  }
  return node;
}

// 式をパーズする
// expr       = assign
Node *expr() {
  return assign();
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
//         | "*" unary
//         | "&" unary
Node *unary() {
  if (consume("+")) {
    // 単項 + 演算子は実質なにもしない
    return primary();
  }
  if (consume("-")) {
    // 単項 - 演算子は 0 - primary に変換する
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  if (consume("&")) {
    // 単項 & 演算子は、変数へのアドレスを表す
    Node *node = unary();
    return new_node(ND_ADDR, node, NULL);
  }
  if (consume("*")) {
    // 単項 * 演算子は、値をアドレスだと思ってその指す値を取り出す
    Node *node = unary();
    return new_node(ND_DEREF, node, NULL);
  }

  // 単項演算子がなければただの primary
  return primary();
}

// 数値かカッコ式をパーズする
// primary    = num
//            | ident ("(" expr? ("," expr)* ")")?
//            | "(" expr ")"
Node *primary() {
  // カッコが来てればカッコに挟まれた式
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // アルファベットが来てれば識別子または関数呼び出し
  Token *tok = consume_ident();

  // 識別子がきて、つぎが "(" なら関数呼び出し
  if (tok && consume("(")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_CALL;
    node->str = tok->str;
    node->len = tok->len;
    int i = 0;
    // (expr ("," expr)*)? ")")
    // 次が ")" でないのなら式が続く
    while (!consume(")")) {
      // 次は式のはず
      node->args[i++] = expr();
      // "," が来たら読み捨てる
      consume(",");
    }
    node->argc = i;
    return node;
  } else if (tok) {
    // 関数呼び出しでなければただの識別子

    // 変数の指す値を入れる領域を確保する
    Node *node = calloc(1, sizeof(Node));
    // ASTノードの種類を左辺値とする
    node->kind = ND_LVAR;
    // ベースポインターからのオフセットを決めるために、
    // これまでのローカル変数リストから変数名を探す
    LVar *lvar = find_lvar(tok);
    
    if (lvar) {
      // 見つかればオフセットはそれと同じになる
      node->offset = lvar->offset;
    } else {
      // 新しい変数名であれば、変数名リストに追加する
      lvar = calloc(1, sizeof(LVar));
      // 新しい要素は先頭につなぐ
      lvar->next = cur_func->locals;
      // 変数名はトークンが持つ値をそのまま使う
      lvar->name = tok->str;
      // 変数名の長さも同じ
      lvar->len = tok->len;
      // 変数名のスタックベースからのオフセットは、
      // 最後に追加された変数のオフセット + 8にする
      if (cur_func->locals) {
        lvar->offset = cur_func->locals->offset + 8;
      } else {
        // 最初に見つかった変数ならオフセットは 8 にする
        lvar->offset = 8;
      }
      // ASTノードに持つオフセットはそれを使う
      node->offset = lvar->offset;
      // 変数リストの先頭アドレスをいま追加したものとする
      cur_func->locals = lvar;
    }
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

// 代入式をパーズする
// assign     = equality ("=" assign)?
Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}
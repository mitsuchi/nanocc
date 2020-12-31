#include "nanocc.h"

// 全体の EBNF
// program    = func_def*
// func_def   = "int" "*"* ident ("(" ("int" "*"* ident)? ("," "int" "*"* ident)* ")")? "{" stmt* "}"
// stmt       = expr ";"
//            | "int" "*"* ident ("[" num "]")? ";"
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
//            | "sizeof" unary
// primary    = num
//            | ident
//            | ident ("(" expr? ("," expr)* ")")?
//            | "(" expr ")"

// 新しい型の部品を作成する
Type *new_type(int kind) {
  // 新しい型をつくる
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  type->ptr_to = NULL;
  return type;
}

// 新しい型の部品を作成してリスト末尾に繋げる
Type *append_type(int kind, Type **head, Type **tail) {
  // 新しい型をつくる
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  // リストが空でないなら末尾に繋げる
  if (*tail) {
    (*tail)->ptr_to = type;
    *tail = type;
  } else {
    // リストが空なら head と tail が新しい型を指すようにする
    *head = type;
    *tail = type;
  }
  return type;
}

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
  // 型
  // 二項演算の結果の型は基本は INT
  node->type = new_type(INT);
  if (kind == ND_ADD || kind == ND_SUB) {
    // PTR の加減算では PTR
    if (node->lhs->type->kind == PTR) {
      node->type = node->lhs->type;
    } else if (node->rhs->type->kind == PTR) {
      node->type = node->rhs->type;
    }
    // ARRAY の加減算では、その要素へのポインター型
    if (node->lhs->type->kind == ARRAY) {
      Type *type = new_type(PTR);
      type->ptr_to = node->lhs->type->ptr_to;
      node->type = type;
    }
  }
  if (kind == ND_ASSIGN) {
    // 代入式では右辺の型
    node->type = node->rhs->type;
  }
  if (kind == ND_ADDR) {
    // &変数 の形では、  変数の型へのポインター型
    Type *type = new_type(PTR);
    type->ptr_to = node->lhs->type;
    node->type = type;
  }
  if (kind == ND_DEREF) {
    // *値 の形では、値が配列ならその要素の型
    // それ以外では値が指す値の型
    // どちらの場合も ptr_to が指している
    node->type = node->lhs->type->ptr_to;
  }
  return node;
}

// 数字のASTノードを作る
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = new_type(INT);
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
void register_var(char *str, int len, Type *type);

// プログラムをパーズする
// program    = func_def*
void program() {
  int i = 0;
  while (!at_eof())
    func_defs[i++] = func_def();
  func_defs[i] = NULL;
}

// 関数定義をパーズする
// func_def   = "int" "*"* ident ("(" ("int" "*"* ident)? ("," "int" "*"* ident)* ")")? "{" stmt* "}"
Node *func_def() {
  // "int" が来るはず
  expect_rword(TK_INT, "int");
  // 次には "*"* が来る
  while (consume("*")) {
    ;
  }
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
    // 次は "int" のはず
    expect_rword(TK_INT, "int");
    // 次には "*"* が来る
    Type *head = NULL;
    Type *tail = NULL;
    while (consume("*")) {
      // * が一つ来るごとに、* -> * -> .. -> Int の先頭のリストを伸ばす
      append_type(PTR, &head, &tail);
    }
    // 型のリストの末尾はつねに INT
    append_type(INT, &head, &tail);
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
    register_var(tok->str, tok->len, head);
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
void register_var(char *str, int len, Type *type) {
  LVar *lvar = calloc(1, sizeof(LVar));
  // 新しい要素を先頭につなぐ
  lvar->next = cur_func->locals;
  // 変数名はトークンが持つ値をそのまま使う
  lvar->name = str;
  // 変数名の長さも同じ
  lvar->len = len;
  // 変数名のスタックベースからのオフセットは、
  // 最後に追加された変数のオフセット + 値のサイズにする
  // 値のサイズは、INT なら 4, PTR なら 8
  // ARRAY なら要素のサイズ x 要素数
  int size;
  if (type->kind == ARRAY) {
    size = value_size(type->ptr_to->kind) * type->array_size;
  } else {
    size = value_size(type->kind);
  }
  if (cur_func->locals) {
    lvar->offset = cur_func->locals->offset + size;
  } else {
    // 最初に見つかった変数ならオフセットは値のサイズにする
    lvar->offset = size;
  }
  // 変数の型
  lvar->type = type;
  // 変数リストの先頭アドレスをいま追加したものとする
  cur_func->locals = lvar;
}

// 文をパーズする
// stmt       = expr ";"
//            | "int" "*"* ident ("[" num "]")? ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
Node *stmt() {
  Node *node;
  // 変数定義 "int" "*"* ident ("[" num "]")? ";"
  if (consume_reserved(TK_INT)) {
    // 次には "*"* が来る
    Type *head = NULL; // 型を表すリストの先頭
    Type *tail = NULL; // リストの末尾
    while (consume("*")) {
      // * が一つ来るごとに、* -> * -> .. -> Int の先頭のリストを伸ばす
      append_type(PTR, &head, &tail);
    }
    // 型のリストの末尾はつねに INT
    append_type(INT, &head, &tail);
    // 次は識別子のはず
    Token *tok = expect_ident();
    // 次に "[" が来たら配列の宣言
    if (consume("[")) {
      // 次は数字が来るはず
      Node *num_node = num();
      expect("]");
      // もし配列であれば、型は配列型で、
      // 要素の型 は head が指すものとする
      Type *type = new_type(ARRAY);
      type->ptr_to = head;
      type->array_size = num_node->val;
      head = type;
    }
    // 変数宣言のノードをつくる
    node = new_node(ND_DECL, NULL, NULL);
    // ローカル変数に登録する
    register_var(tok->str, tok->len, head);

    expect(";");
  // block
  } else if (consume("{")) {
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
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NEQ, node, relational());
    } else
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
//         | "sizeof" unary
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
  if (consume_reserved(TK_SIZEOF)) {
    Node *node = unary();
    node->type->array_size;
    int size;
    if (node->type->kind == INT) {
      size = 4;
    } else if (node->type->kind == ARRAY) {
      int elem_size;
      if (node->type->ptr_to->kind == INT) {
        elem_size = 8;
      } else {
        elem_size = 8;
      }
      size = node->type->array_size * elem_size;
    } else {
      size = 8;
    }
    return new_node_num(size);
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
    // todo: 関数の返り値の型を見る必要があるがひとまず INT としてしまう
    node->type = new_type(INT);
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
    // ASTノードの種類をローカル変数とする
    node->kind = ND_LVAR;
    // ベースポインターからのオフセットを決めるために、
    // これまでのローカル変数リストから変数名を探す
    LVar *lvar = find_lvar(tok);
    
    if (lvar) {
      // 見つかればオフセットはそれと同じになる
      node->offset = lvar->offset;
      // 型は変数の型
      // この時点でTの配列の型をTへのポインタ型としてしまうことはできない
      // sizeof があるから
      node->type = lvar->type;
    } else {
      // 見つからなければエラー
      error_at(tok->str, "定義されていない変数です");
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
#include "nanocc.h"

// 全体の EBNF
// program    = global_var_or_funcs*
// global_var_or_funcs
//            = ( type "*"* ident ("(" (type "*"* ident)? ("," type "*"* ident)* ")")? "{" stmt* "}"
//            | type "*"* ident ("[" num "]")? ";" )*
// stmt       = expr ";"
//            | type "*"* ident ("[" num "]")? ("=" expr)? ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
// type       = "int" | "char"
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
//            | '"' string '"'
//            | ident ("(" expr? ("," expr)* ")")?
//            | ident "[" expr "]"
//            | "(" expr ")"
Node *global_var_or_funcs();
Node *func_def();
Node *stmt();
Node *block();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *num();

// プログラムをパーズする
// program    = func_def*
void program() {
  int func_i = 0;
  int var_i = 0;
  while (!at_eof()) {
    Node *node = global_var_or_funcs();
    if (node->kind == ND_FUNC_DEF) {
      func_defs[func_i++] = node;
    }
  }
  func_defs[func_i] = NULL;
}

// 関数定義をパーズする
// global_var_or_funcs 
//            = ( type "*"* ident ("(" (type "*"* ident)? ("," type "*"* ident)* ")")? "{" stmt* "}"
//            | type "*"* ident ("[" num "]")? ";" )*
Node *global_var_or_funcs() {
  // "int" または "char" が来るはず
  int type_kind = expect_type();
  // 次には "*"* が来る
  Type *head = NULL;
  Type *tail = NULL;
  while (consume("*")) {
    // * が一つ来るごとに、* -> * -> .. -> Int の先頭のリストを伸ばす
    append_type(PTR, &head, &tail);
  }
  // 型のリストの末尾は、最初が int なら INT, char なら CHAR
  append_type(type_kind, &head, &tail);
  // 識別子がくるはず
  Token *tok = expect_ident();
  // "(" が来れば関数定義
  if (consume("(")) {
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
      // "int" または "char" が来るはず
      int type_kind = expect_type();
      // 次には "*"* が来る
      Type *head = NULL;
      Type *tail = NULL;
      while (consume("*")) {
        // * が一つ来るごとに、* -> * -> .. -> Int の先頭のリストを伸ばす
        append_type(PTR, &head, &tail);
      }
      // 型のリストの末尾は int または char
      append_type(type_kind, &head, &tail);
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
      // 関数の返り値の型を入れておく
      node->type = head;
      // "," が来たら読み捨てる
      consume(",");
      // 仮引数名を変数リストに追加する
      register_var(tok->str, tok->len, head);
    }
    node->argc = i;

    // ブロックが来るはず
    expect("{");
    // 関数定義の本体をブロックにする
    node->body = block();
    return node;
  } else {
    // "[" が来れば配列の宣言
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
    Node *node = new_node(ND_DECL);
    // グローバル変数に登録する
    register_global_var(tok->str, tok->len, head);
    expect(";");
  }
}

// {} で囲まれたブロックをパーズする
Node *block() {
  // ブロックを表すノードを用意する
  // node->next で複数の文をつないでいく
  Node *node = new_node(ND_BLOCK);
  // 文のリストの最後を指しておく
  Node *last = node;
  while (!consume("}")) {
    // 文を1つパーズしてノードをつくる
    Node *cur_node = stmt();
    // それを文のリストにつなげる
    last->next = cur_node;
    // 最後を交代する
    last = cur_node;
    while (last->next) {
      // stmt() がノードのリストを返すことがありうるので、
      // 末尾まで移動しておく
      last = last->next;
    }
  }
  // 終末の次はNULLにしておく
  last->next = NULL;
  return node;
}

// 文をパーズする
// stmt       = expr ";"
//            | type "*"* ident ("[" num "]")? ("=" expr)? ";"
//            | "{" stmt* "}"
//            | "return" expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
Node *stmt() {
  Node *node;
  // 変数定義 
  // type "*"* ident ("[" num "]")? ("=" expr)? ";"
  int type_kind = consume_type();
  if (type_kind) {
    // 次には "*"* が来る
    Type *head = NULL; // 型を表すリストの先頭
    Type *tail = NULL; // リストの末尾
    while (consume("*")) {
      // * が一つ来るごとに、* -> * -> .. -> Int の先頭のリストを伸ばす
      append_type(PTR, &head, &tail);
    }
    // 型のリストの末尾は int または char
    append_type(type_kind, &head, &tail);
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
    node = new_node(ND_DECL);
    // ローカル変数に登録する
    register_var(tok->str, tok->len, head);

    if (consume("=")) {
      // int x = 3 のような形の初期化式は
      // int x と x = 3 の二つの文に分解する
      Node *var_node = new_node(ND_LVAR);
      LVar *lvar = find_lvar(tok);
      var_node->offset = lvar->offset;
      var_node->var = lvar;
      var_node->type = lvar->type;

      Node *assign_node = new_node_bin(ND_ASSIGN, var_node, expr());
      node->next = assign_node;
    }
    expect(";");
  // block
  } else if (consume("{")) {
    // ブロックを表すノードを用意する
    node = block();
  // return
  } else if (consume_reserved(TK_RETURN)) {
    node = new_node_unary(ND_RETURN, expr());
    node->str = token->str;
    expect(";");
  // if
  } else if (consume_reserved(TK_IF)) {
    node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->lhs = stmt();
    if (consume_reserved(TK_ELSE)) {
      node->rhs = stmt();
    }
  // while
  } else if (consume_reserved(TK_WHILE)) {
    node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->lhs = stmt();
  // for 
  // "for" "(" expr? ";" expr? ";" expr? ")" stmt
  } else if (consume_reserved(TK_FOR)) {
    node = new_node(ND_FOR);
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
    node->src_pos = token->str;
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
      node = new_node_bin(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node_bin(ND_NEQ, node, relational());
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
      node = new_node_bin(ND_LT, node, add());
    else if (consume("<="))
      node = new_node_bin(ND_LTE, node, add());
    else if (consume(">"))
      // > は左辺と右辺を逆転した < としてしまう
      node = new_node_bin(ND_LT, add(), node);
    else if (consume(">="))
      // >= は左辺と右辺を逆転した <= としてしまう
      node = new_node_bin(ND_LTE, add(), node);
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
      node = new_node_bin(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node_bin(ND_SUB, node, mul());
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
      node = new_node_bin(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node_bin(ND_DIV, node, unary());
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
    return new_node_bin(ND_SUB, new_node_num(0), primary());
  }
  if (consume("&")) {
    // 単項 & 演算子は、変数へのアドレスを表す
    Node *node = unary();
    return new_node_unary(ND_ADDR, node);
  }
  if (consume("*")) {
    // 単項 * 演算子は、値をアドレスだと思ってその指す値を取り出す
    Node *node = unary();
    return new_node_unary(ND_DEREF, node);
  }
  if (consume_reserved(TK_SIZEOF)) {
    Node *node = unary();
    int size = type_size(node->type);
    return new_node_num(size);
  }
  // 単項演算子がなければただの primary
  return primary();
}

// 数値かカッコ式をパーズする
// primary    = num
//            | '"' string '"'
//            | ident ("(" expr? ("," expr)* ")")?
//            | "(" expr ")"
//            | ident "[" expr "]"
Node *primary() {
  // カッコが来てればカッコに挟まれた式
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // アルファベットが来てれば識別子または関数呼び出し
  Token *tok = consume_ident();

  if (tok && consume("[")) {
    // 変数の指す値を入れる領域を確保する
    Node *var_node = calloc(1, sizeof(Node));
    // ASTノードの種類をローカル変数とする
    var_node->kind = ND_LVAR;
    // ベースポインターからのオフセットを決めるために、
    // これまでのローカル変数リストから変数名を探す
    LVar *lvar = find_lvar(tok);
    if (lvar) {
      // 見つかればオフセットはそれと同じになる
      var_node->offset = lvar->offset;
      var_node->var = lvar;
      // 型は変数の型
      // この時点でTの配列の型をTへのポインタ型としてしまうことはできない
      // sizeof があるから
      var_node->type = lvar->type;
    } else {
      // ローカル変数に見つからなければ
      // グローバル変数を探す
      lvar = find_global_var(tok);
      if (lvar) {
        // 見つかればグローバル変数ということになる
        var_node->kind = ND_GVAR;
        var_node->var = lvar;
        // オフセットはそれと同じになる
        var_node->offset = lvar->offset;
        // 型は変数の型
        var_node->type = lvar->type;
      } else {
        // 見つからなければエラー
        error_at(tok->str, "定義されていない変数です");
      }
    }

    // 識別子がきて、つぎが "[" なら配列の特定の要素の値
    // 配列の添字
    Node *index = expr();
    expect("]");
    Node *array_access_node = new_node_bin(ND_ADD, var_node, index);
    Node *deref_node = new_node_unary(ND_DEREF, array_access_node);
    return deref_node;
  } else if (tok && consume("(")) {
    // 識別子がきて、つぎが "(" なら関数呼び出し    
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
      node->var = lvar;
      // 型は変数の型
      // この時点でTの配列の型をTへのポインタ型としてしまうことはできない
      // sizeof があるから
      node->type = lvar->type;
    } else {
      // ローカル変数に見つからなければ
      // グローバル変数を探す
      lvar = find_global_var(tok);
      if (lvar) {
        // 見つかればグローバル変数ということになる
        node->kind = ND_GVAR;
        node->var = lvar;
        // オフセットはそれと同じになる
        node->offset = lvar->offset;
        // 型は変数の型
        node->type = lvar->type;
      } else {
        // 見つからなければエラー
        error_at(tok->str, "定義されていない変数です");
      }
    }
    return node;
  }
  // そうでなければ数値または文字列のはず
  String *string;
  if (string = consume_string()) {
    return new_node_string(string);
  } else {
    return num();
  }
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
    node = new_node_bin(ND_ASSIGN, node, assign());
  return node;
}
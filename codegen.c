#include "nanocc.h"

// ラベルの末尾につける通し番号
int label_id = 1;

// 左辺値の表すアドレスをスタックに積むコードを出力する
void gen_lval(Node *node) {
  printf("  # gen lval\n");
  if (node->kind == ND_LVAR) {
    // ローカル変数の場合
    // rax にベースポインタをもってくる
    printf("  mov rax, rbp\n");
    // ベースポインタからその変数へのオフセットを引くことで、変数のアドレスを得る
    printf("  sub rax, %d\n", node->offset);
    // 変数のアドレスをスタックに積む
    printf("  push rax # variable's address\n");
  } else if (node->kind == ND_DEREF) {
    // * の右側を普通の値だと思ってコンパイルする
    gen(node->lhs);
    // スタックの先頭にアドレスが積まれるので、これでOK
  } else {
    error("代入の左辺値が変数またはデリファレンスではありません");
  }
}

char *arg_registers(int i, size_t size) {
  // ABI に定められた引数を格納するべきレジスター群
  char *arg_registers_8[6] = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"
  };
  char *arg_registers_4[6] = {
    "edi", "esi", "edx", "ecx", "e8", "e9"
  };
  if (size == 4) {
    return arg_registers_4[i];
  }
  if (size == 8) {
    return arg_registers_8[i];
  }
}

// ASTからアセンブリを出力する
void gen(Node *node) {
  // ブロック中の現在注目する文
  Node *cur_stmt;
  // 関数名
  char func_name[255];

  // 値なら push する
  switch (node->kind) {
  // 数値
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  // ローカル変数
  case ND_LVAR:
    printf("  # lvar\n");
    // 左辺値の指すアドレスをスタックに積むコードを生成
    gen_lval(node);
    if (node->type->kind == ARRAY) {
      // 配列ならアドレスを積んでおしまい
      return;
    }
    // 配列以外ならアドレスの指す値を持ってくる
    // 左辺値の指すアドレスを rax に持ってくる
    printf("  pop rax\n");
    // アドレスの指す値を rax に持ってくる
    if (type_size(node->type) == 4) {
      printf("  mov eax, DWORD PTR [rax]\n");
    } else {
      printf("  mov rax, [rax]\n");
    }
    // 持ってきた値をスタックに積む
    printf("  push rax\n");
    return;
  // 代入式
  case ND_ASSIGN:
    printf("  # assign\n");
    // まず左辺のアドレスをスタックに積む
    gen_lval(node->lhs);
    // 右辺値をスタックに積む
    gen(node->rhs);

    // 右辺値を rdi に持ってくる
    printf("  pop rdi\n");
    // 左辺値のアドレスを rax に持ってくる
    printf("  pop rax\n");
    // 左辺値のアドレスに右辺値の値をストアする
    if (type_size(node->rhs->type) == 4) {
      printf("  mov DWORD PTR [rax], edi\n");
    } else {
      printf("  mov [rax], rdi\n");
    }
    // 右辺値をスタックに積む。つまり代入式の値は右辺値。
    printf("  push rdi\n");
    return; 
  // return
  case ND_RETURN:
    // return 式 の 式を積む
    gen(node->lhs);
    // 返すべき値を rax に取ってきて
    printf("  pop rax\n");
    // ここからエピローグ。スタックは現時点でこうなっている。
    // 戻りアドレス
    // 呼び出し時点の rbp <- rbp
    // 変数群
    // ..               <- rsp
    // 返すべき式
    
    // 関数呼び出し時点のベースポインタをスタックポインタが指すようにして
    printf("  mov rsp, rbp\n");
    // ベースポインタを呼び出し時点のものに戻す
    printf("  pop rbp\n");
    // rax に持っている値を返し、戻りアドレスに戻る
    printf("  ret\n");
    return;
  // if
  case ND_IF:
    printf("  # if\n");
    // 条件式 をコンパイルしてスタックトップに積む
    gen(node->cond);
    // 条件式を 0 と比較する
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    // else がない場合
    if (node->rhs == NULL) {
      // 条件が偽ならなにもせず抜ける
      printf("  je .Lend%d\n", label_id);
      // 条件が真なら then 部分を計算
      gen(node->lhs);
    } else {
      // else がある場合
      // 条件が偽なら else へジャンプ
      printf("  je .Lelse%d\n", label_id);
      // 条件が真なら then 部分を計算して抜ける
      gen(node->lhs);
      printf("  jmp .Lend%d\n", label_id);
      // 偽の場合は else 部分を計算する
      printf(".Lelse%d:\n", label_id);
      gen(node->rhs);
    }
    printf(".Lend%d:\n", label_id);
    // 通し番号を足しておく
    label_id++;
    return;
  // while
  case ND_WHILE:
    printf(".Lbegin%d:\n", label_id);
    // 条件式をコンパイル
    gen(node->cond);
    // 条件式を 0 と比較する
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    // 条件が偽なら end へジャンプ
    printf("  je .Lend%d\n", label_id);
    // 本体をコンパイル
    gen(node->lhs);
    // whileの最初に戻る
    printf("  jmp .Lbegin%d\n", label_id);
    printf(".Lend%d:\n", label_id);
    // 通し番号を足しておく
    label_id++;
    return;
  // for
  case ND_FOR:
    // 初期化式をコンパイル
    gen(node->lhs); 
    printf(".Lbegin%d:\n", label_id);
    // 条件式をコンパイル
    gen(node->cond);
    // 条件式を 0 と比較する
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    // 条件が偽なら end へジャンプ
    printf("  je .Lend%d\n", label_id);
    // 本体をコンパイル
    gen(node->body);
    // 増加式をコンパイル
    gen(node->rhs);
    // ループの最初に戻る
    printf("  jmp .Lbegin%d\n", label_id);
    printf(".Lend%d:\n", label_id);
    label_id++;
    return;
  // ブロック
  case ND_BLOCK:
    // いま注目している文を指しておく
    cur_stmt = node->next;
    while (cur_stmt != NULL) {
      // 文を1つコンパイルする
      gen(cur_stmt);
      // 次の文に進む
      cur_stmt = cur_stmt->next;
      if (cur_stmt != NULL) {
        // 最後の文以外では、積んだ値はムダなので捨てる
        printf("  pop rax\n");
      }
    }
    return;
  // 関数呼び出し
  case ND_CALL:
    printf("  # call\n");
    // 引数を順にコンパイルする
    for (int i = 0; i < node->argc; i++) {
      gen(node->args[i]);
    }
    // ABIで定められた各レジスタに pop する
    for (int i = node->argc - 1; i >= 0; i--) {
      printf("  pop %s\n", arg_registers(i, 8));
    }
    // rax には引数の個数を入れる
    printf("  mov rax, %d\n", node->argc);
    // 関数名をコピーしてくる
    strncpy(func_name, node->str, node->len);
    func_name[node->len] = '\0';
    // todo: rsp が16の倍数になっていなければ調整、のコードを入れる
    printf("  call %s\n", func_name);
    // 関数の戻り値が rax に入っているのでスタックに積む
    printf("  push rax\n");
    return;
  // 関数定義
  case ND_FUNC_DEF:
    // 関数名をコピーしてくる
    strncpy(func_name, node->str, node->len);
    func_name[node->len] = '\0';
    // 関数をリンク時に外のファイルから見れるようにする
    printf(".globl %s\n", func_name);
    // ラベルを出力する
    printf("%s:\n", func_name);
    // プロローグ
    // 現時点のスタックベースポインタをスタックに積む
    printf("  # prologue\n");
    printf("  push rbp\n");
    // 現在のスタックの先頭をベースポインタとする
    printf("  mov rbp, rsp\n");
    // レジスタにある引数を、引数の個数分だけ、定められたオフセットに割り当てる
    LVar *cur = node->locals;
    // ローカル変数のリストを、ローカル変数、実引数の順で逆順に持たせる
    LVar *vars[255];
    int num_locals = 0;
    while (cur) {
      vars[num_locals] = cur;
      cur = cur->next;
      num_locals++;
    }
    char var_name[255];
    for (int i = num_locals - 1; i >= 0; i--) {
      strncpy(var_name, vars[i]->name, vars[i]->len);
      printf("  # offset %s %d\n", var_name, vars[i]->offset);
    }
    // 引数の個数分だけ、スタックに値を割り当てる
    for (int i = 0; i < node->argc; i++) {
      printf("  mov rbx, rsp\n");
      printf("  sub rbx, %d\n", vars[num_locals - 1 - i]->offset);
      int size = type_size(vars[num_locals - 1 - i]->type);
      printf("  mov [rbx], %s\n", arg_registers(i, size));
    }
    // 引数とローカル変数の全体分の領域を確保する        
    if (node->locals) {
      // node->locals は最後に登録された変数を指す
      printf("  mov rsp, rbp\n");
      printf("  sub rsp, %d\n", node->locals->offset);
    }

    // 本体であるブロックをコンパイルする
    printf("  # function body\n");
    gen(node->body);
    // エピローグ
    // 関数呼び出し時点のベースポインタをスタックから取得し
    printf("  # epilogue\n");
    printf("  mov rsp, rbp\n");
    // ベースポインタをそれに戻す
    printf("  pop rbp\n");
    // 最後の式の結果がRAXに残っているのでそれが返り値になる  
    printf("  ret\n");
    return;
  // & 変数名のアドレス
  case ND_ADDR:
    // 変数は lhs に入っている
    // 変数のアドレスは、変数を左辺値として評価すれば取り出せる
    gen_lval(node->lhs);
    return;
  // * デリファレンス：値をアドレスだと思って、その指す値を取り出す
  case ND_DEREF:
    printf("  # deref\n");
    // 値は lhs に入っている
    gen(node->lhs);
    // 値を rax に持ってくる
    printf("  pop rax\n");
    // 値をアドレスだと思って、その指す値を rax に取り出す
    if (type_size(node->lhs->type) == 4) {
      printf("  mov eax, DWORD PTR [rax]\n");
    } else {
      printf("  mov rax, [rax]\n");
    }
    // rax をスタックに積む
    printf("  push rax\n");
    return;
  // 変数宣言
  case ND_DECL:
    // なにもしないが、何かを積む約束になっているので 0 を積む
    printf("  push 0 # do nothing\n");
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
  printf("  pop rdi\n"); // 右辺
  printf("  pop rax\n"); // 左辺

  switch (node->kind) {
  case ND_ADD:
    if (node->lhs->kind == ND_LVAR || node->lhs->kind == ND_ADDR ) {
      // 足し算の左辺が変数のときだけ
      if (node->lhs->type->kind == PTR || node->lhs->type->kind == ARRAY) {
        printf("  imul rdi, %d\n", type_size(node->lhs->type->ptr_to));
      }
    }
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    // 左辺 - 右辺
    if (node->lhs->kind == ND_LVAR || node->lhs->kind == ND_ADDR ) {
      // 足し算の左辺が変数のときだけ
      if (node->lhs->type->kind == PTR || node->lhs->type->kind == ARRAY) {
        printf("  imul rdi, %d\n", type_size(node->lhs->type->ptr_to));
      }
    }    
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
  // == 
  case ND_EQ:
    // 等しさを比べる
    printf("  cmp rax, rdi\n");
    // 結果を al レジスタに入れる。al は rax の下位8bit の別名
    printf("  sete al\n");
    // rax 全体を 0 か 1 にしたいので、rax の上位 56 bit を 0クリアする
    printf("  movzb rax, al\n");
    break;
  // != 
  case ND_NEQ:
    // cmp, setne で等しくなさを比べる 
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  // <
  case ND_LT:
    // cmp, setl で < での大小比較を行う
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  // <=
  case ND_LTE:
    // cmp, setl で <= での大小比較を行う
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break; 
  }

  printf("  push rax\n");
}


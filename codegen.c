#include "nanocc.h"

// 左辺値の表すアドレスをスタックに積むコードを出力する
void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("代入の左辺値が変数ではありません");

  // rax にベースポインタをもってくる
  printf("  mov rax, rbp\n");
  // ベースポインタからその変数へのオフセットを引くことで、変数のアドレスを得る
  printf("  sub rax, %d\n", node->offset);
  // 変数のアドレスをスタックに積む
  printf("  push rax\n");
}

// ASTからアセンブリを出力する
void gen(Node *node) {
  // ラベルの末尾につける通し番号
  int label_id = 1;
  // 値なら push する
  switch (node->kind) {
  // 数値
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  // 左辺値
  case ND_LVAR:
    // 左辺値の指すアドレスをスタックに積むコードを生成
    gen_lval(node);
    // 左辺値の指すアドレスを rax に持ってくる
    printf("  pop rax\n");
    // アドレスの指す値を rax に持ってくる
    printf("  mov rax, [rax]\n");
    // 持ってきた値をスタックに積む
    printf("  push rax\n");
    return;
  // 代入式
  case ND_ASSIGN:
    // まず左辺のアドレスをスタックに積む
    gen_lval(node->lhs);
    // 右辺値をスタックに積む
    gen(node->rhs);

    // 右辺値を rdi に持ってくる
    printf("  pop rdi\n");
    // 左辺値のアドレスを rax に持ってくる
    printf("  pop rax\n");
    // 左辺値のアドレスに右辺値の値をストアする
    printf("  mov [rax], rdi\n");
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
    // 条件式 をコンパイルしてスタックトップに積む
    gen(node->cond);
    // 条件式を 0 を比較する
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


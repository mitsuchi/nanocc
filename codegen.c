#include "nanocc.h"

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


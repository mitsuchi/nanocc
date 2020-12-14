#include "nanocc.h"

// 入力プログラム の定義
char *user_input;

// 現在着目しているトークン の定義
Token *token;

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
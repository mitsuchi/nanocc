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

  // 全体を文の並びとして構文解析する
  program();

  // intel記法を使う
  printf(".intel_syntax noprefix\n");
  // main をリンク時に外のファイルから見れるようにする
  printf(".globl main\n");
  // main ラベル
  printf("main:\n");

  // プロローグ
  // 変数26個分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  // 先頭の式から順にコード生成
  for (int i = 0; code[i]; i++) {
    gen(code[i]);

    // 式の評価結果としてスタックに一つの値が残っている
    // はずなので、スタックが溢れないようにポップしておく
    printf("  pop rax\n");
  }    

  // エピローグ
  // 関数呼び出し時点のベースポインタをスタックから取得し
  printf("  mov rsp, rbp\n");
  // ベースポインタをそれに戻す
  printf("  pop rbp\n");
  // 最後の式の結果がRAXに残っているのでそれが返り値になる  
  printf("  ret\n");
  // 正常終了コードを返す
  return 0;
}
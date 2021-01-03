#include "nanocc.h"

// 入力プログラム の定義
char *user_input;

// 現在着目しているトークン の定義
Token *token;

// ローカル変数リストの先頭アドレスを覚えておく
// 新しい要素は先頭につないでいくので、先頭アドレスは最後に足した要素を指す
LVar *locals = NULL;

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

  // グローバル変数を出力する
  gen_global_var();

  // 文字列を出力する
  gen_strings();
  
  // 先頭の関数定義から順にコード生成
  for (int i = 0; func_defs[i]; i++) {
     gen(func_defs[i]);
  }

  // 正常終了コードを返す
  return 0;
}
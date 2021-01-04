#include "nanocc.h"

// 入力プログラム の定義
char *user_input;

// 現在着目しているトークン の定義
Token *token;

// ローカル変数リストの先頭アドレスを覚えておく
// 新しい要素は先頭につないでいくので、先頭アドレスは最後に足した要素を指す
LVar *locals = NULL;

// 指定されたファイルの内容を返す
char *read_file(char *path);

// argc はコンパイラーへの引数の数(+1)
// argv は引数の文字列の先頭へのポインターを納めた配列へのポインター
int main(int argc, char **argv) {
  if (argc <= 1 || 4 <= argc) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // 第二引数をオプションにする
  char *option;
  if (argc == 3) {
    option = argv[2];
  } else {
    option = "";
  }
  
  // エラー表示に使うためにプログラムの先頭を指しておく
  filename = argv[1];
  user_input = read_file(filename);

  // トークナイズする
  token = tokenize(user_input);

  // 全体を文の並びとして構文解析する
  program();

  if (strcmp(option, "-d") == 0) { // debug
    print_ast();
    return 0;
  }

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

// 指定されたファイルの内容を返す
char *read_file(char *path) {
  // ファイルを開く
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  // ファイル内容を読み込む
  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず"\n\0"で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}
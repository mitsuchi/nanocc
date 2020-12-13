#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // p はプログラムとなる文字列の先頭を指すポインター
  char *p = argv[1];

  // intel記法を使う
  printf(".intel_syntax noprefix\n");
  // main をリンク時に外のファイルから見れるようにする
  printf(".globl main\n");
  // main ラベル
  printf("main:\n");
  // strtol で ポインターの指す文字列を10進数の数として解釈する
  // それによって p は数字の終わりまで移動する
  // %ld は long int
  // mov rax : rax レジスタに値をセットする
  printf("  mov rax, %ld\n", strtol(p, &p, 10));

  // p の指す先が NULL でない限り
  while (*p) {
    // もし + なら一文字進めて、次の数字を読んで足し算を出力する
    if (*p == '+') {
      p++;
      printf("  add rax, %ld\n", strtol(p, &p, 10));
      continue;
    }

    // もし - なら一文字進めて、次の数字を読んで引き算を出力する
    if (*p == '-') {
      p++;
      printf("  sub rax, %ld\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "予期しない文字です: '%c'\n", *p);
    return 1;
  }

  // rax レジスタの値を返す
  printf("  ret\n");
  return 0;
}
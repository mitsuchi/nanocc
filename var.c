#include "nanocc.h"

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
  int size = type_size(type);
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
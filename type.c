#include "nanocc.h"

// 新しい型の部品を作成する
Type *new_type(int kind) {
  // 新しい型をつくる
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  type->ptr_to = NULL;
  return type;
}

// 新しい型の部品を作成してリスト末尾に繋げる
Type *append_type(int kind, Type **head, Type **tail) {
  // 新しい型をつくる
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  // リストが空でないなら末尾に繋げる
  if (*tail) {
    (*tail)->ptr_to = type;
    *tail = type;
  } else {
    // リストが空なら head と tail が新しい型を指すようにする
    *head = type;
    *tail = type;
  }
  return type;
}

int type_size (Type *type) {
  switch (type->kind) {
    case CHAR:
      return 1;    
    case INT:
      return 4;
    case PTR:
      return 8;
    case ARRAY:
      return type_size(type->ptr_to) * type->array_size;
  }
  error("unreachable: type_size");
}
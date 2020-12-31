#include "nanocc.h"

// 基本のASTノードを作る
Node *new_node(NodeKind kind) {
  // Node1つぶんのメモリを確保して0でクリアする
  Node *node = calloc(1, sizeof(Node));
  // ノードの種類: 足し算か数字かなど
  node->kind = kind;

  return node;
}

// 単項演算のASTノードを作る
Node *new_node_unary(NodeKind kind, Node *lhs) {
  // Node1つぶんのメモリを確保して0でクリアする
  Node *node = calloc(1, sizeof(Node));
  // ノードの種類: 足し算か数字かなど
  node->kind = kind;
  // 左辺
  node->lhs = lhs;
  if (kind == ND_ADDR) {
    // &変数 の形では、変数の型へのポインター型
    Type *type = new_type(PTR);
    type->ptr_to = node->lhs->type;
    node->type = type;
  }
  if (kind == ND_DEREF) {
    // *値 の形では、値が配列ならその要素の型
    // それ以外では値が指す値の型
    // どちらの場合も ptr_to が指している
    node->type = node->lhs->type->ptr_to;
  }
  return node;
}

// 二項演算のASTノードを作る
Node *new_node_bin(NodeKind kind, Node *lhs, Node *rhs) {
  // Node1つぶんのメモリを確保して0でクリアする
  Node *node = calloc(1, sizeof(Node));
  // ノードの種類: 足し算か数字かなど
  node->kind = kind;
  // 左辺
  node->lhs = lhs;
  // 右辺
  node->rhs = rhs;
  // 型
  // 二項演算の結果の型は基本は INT
  node->type = new_type(INT);
  if (kind == ND_ADD || kind == ND_SUB) {
    // PTR の加減算では PTR
    if (node->lhs->type->kind == PTR) {
      node->type = node->lhs->type;
    } else if (node->rhs->type->kind == PTR) {
      node->type = node->rhs->type;
    }
    // ARRAY の加減算では、その要素へのポインター型
    if (node->lhs->type->kind == ARRAY) {
      Type *type = new_type(PTR);
      type->ptr_to = node->lhs->type->ptr_to;
      node->type = type;
    }
  }
  if (kind == ND_ASSIGN) {
    // 代入式では右辺の型
    node->type = node->rhs->type;
  }
  if (kind == ND_ADDR) {
    // &変数 の形では、  変数の型へのポインター型
    Type *type = new_type(PTR);
    type->ptr_to = node->lhs->type;
    node->type = type;
  }
  if (kind == ND_DEREF) {
    // *値 の形では、値が配列ならその要素の型
    // それ以外では値が指す値の型
    // どちらの場合も ptr_to が指している
    node->type = node->lhs->type->ptr_to;
  }
  return node;
}

// 数字のASTノードを作る
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = new_type(INT);
  return node;
}
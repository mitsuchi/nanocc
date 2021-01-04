#include "nanocc.h"

void print_func(Node *node);
void print_node(Node *node, int depth);

// グローバル変数と、各関数のローカル変数と本体を表示する
void print_ast() {
  LVar *gv = global_var_list;
  if (gv)
    printf("global vars\n");
  while (gv) {
    // 変数名
    printf("- %s: ", gv->name);
    // 型
    Type *type = gv->type;
    while (type) {
      printf("%s ", type_name(type));
      type = type->ptr_to;
    }
    if (gv->type->kind == ARRAY) {
      printf("x %ld", gv->type->array_size);
    }
    printf("\n");
    gv = gv->next;
  }

  // 先頭の関数定義から順に表示
  printf("functions\n");
  for (int i = 0; func_defs[i]; i++) {
    print_func(func_defs[i]);
  }
}

// 関数定義を表示
void print_func(Node *node) {
  // ローカル変数
  char func_name[255];
  strncpy(func_name, node->str, node->len);
  func_name[node->len] = '\0';
  printf("- %s\n", func_name);
  LVar *lvar = node->locals;
  if (lvar) {
    printf("  - local vars\n");    
  }
  while (lvar) {
    printf("    - %s: ", lvar->name);
    // 型
    Type *type = lvar->type;
    while (type) {
      printf("%s ", type_name(type));
      type = type->ptr_to;
    }
    if (lvar->type->kind == ARRAY) {
      printf("x %ld", lvar->type->array_size);
    }
    printf("\n");    
    lvar = lvar->next;
  }
  // 本体
  Node *stmt = node->body->next;
  while (stmt) {
    // 文を1つ表示する
    print_node(stmt, 2);
    // 次の文に進む
    stmt = stmt->next;
  }
}

void print_node(Node *node, int depth) {
  char func_name[255];

  // インデント
  for (int i = 0; i < depth; i++)
    printf(" ");

  // 本体
  switch (node->kind) {
  case ND_NUM:
    printf("- num: %d\n", node->val);
    break;
  case ND_GVAR:
    printf("- global var: %s\n", node->var->name);
    break;
  case ND_LVAR:
    printf("- local var: %s\n", node->var->name);
    break;
  case ND_CALL:
    // 関数名をコピーしてくる
    strncpy(func_name, node->str, node->len);
    func_name[node->len] = '\0';
    printf("call %s\n", func_name);
    break;
  case ND_ASSIGN:
    printf("- assign\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_DEREF:
    printf("- deref\n");
    // 値は lhs に入っている
    print_node(node->lhs, depth + 2);
    break; 
  case ND_ADD:
    printf("- +\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_SUB:
    printf("- -\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_MUL:
    printf("- *\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_DIV:
    printf("- /\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_EQ:
    printf("- ==\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_NEQ:
    printf("- !=\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_LT:
    printf("- <\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_LTE:
    printf("- <=\n");
    print_node(node->lhs, depth + 2);
    print_node(node->rhs, depth + 2);
    break;
  case ND_RETURN:
    printf("- return\n");
    print_node(node->lhs, depth + 2);
    break;
  case ND_IF:
    printf("- if\n");
    printf(" - cond\n");
    print_node(node->cond, depth + 2);
    printf(" - then\n");
    print_node(node->lhs, depth + 2);
    printf(" - else\n");
    print_node(node->rhs, depth + 2);
    break;
  case ND_WHILE:
    printf("- while\n");
    printf(" - cond\n");
    print_node(node->cond, depth + 2);
    printf(" - body\n");
    print_node(node->lhs, depth + 2);
    break;
  case ND_FOR:
    printf("- for\n");
    printf(" - init\n");
    print_node(node->lhs, depth + 2);
    printf(" - cond\n");
    print_node(node->cond, depth + 2);
    printf(" - incr\n");
    print_node(node->rhs, depth + 2);
    printf(" - body\n");
    print_node(node->body, depth + 2);
    break;
  case ND_BLOCK:
    printf("- block\n");
    Node *cur_stmt = node->next;
    while (cur_stmt != NULL) {
      print_node(cur_stmt, depth + 2);
      cur_stmt = cur_stmt->next;
    }
    break;
  case ND_ADDR:
    printf("- address\n");
    print_node(node->lhs, depth + 2);
    break;
  case ND_STRING:
    printf("- string: %s\n", node->string->str);
    break;
  case ND_DECL:
    printf("- local var decl\n");
    break;
  default:
    printf("- hoge: %d\n", node->kind);
  }
}

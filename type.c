#include "nanocc.h"

int value_size (int kind) {
  switch (kind) {
    case INT:
      // return 4;
      // いったん8バイトにする
      return 8;
    case PTR:
      return 8;
    default:
      return 8;
  }
}
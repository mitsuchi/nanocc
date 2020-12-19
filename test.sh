#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./nanocc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 45 "b = 0; for (a = 0; a < 10; a = a + 1) b = b + a;; return b;"
assert 8 "a = 1; while (a < 5) a = a + a;; return a;"
assert 3 "if (1 < 2) 3;;"; # 最新のテストほど先頭に持ってくるように変更
assert 3 "if (1 < 2) 3; else 4;;";
assert 4 "if (2 < 1) 3; else 4;;";
assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 7 "1+2*3;"
assert 5 "1*2+3;"
assert 2 "1+2*3/4;"
assert 14 "2*(3+4);"
assert 94 "2*(3+(4*(5+6)));"
assert 2 "-3+5;"
assert 2 "-(3+5)+10;"
assert 5 "-3*+5 + 20;"
assert 1 "12==12;"
assert 0 "123==12;"
assert 1 "45!=12;"
assert 0 "45!=45;"
assert 0 "10 < 9;"
assert 0 "10 < 10;"
assert 1 "10 < 11;"
assert 0 "10 <= 9;"
assert 1 "10 <= 10;"
assert 1 "10 <= 11;"
assert 1 "10 > 9;"
assert 0 "10 > 10;"
assert 0 "10 > 11;"
assert 1 "10 >= 9;"
assert 1 "10 >= 10;"
assert 0 "10 >= 11;"
assert 14 "a = 3; b = 5 * 6 - 8; a + b / 2;"
assert 42 "return 42;";
assert 31 "return 5*6+1; 42;";

echo OK
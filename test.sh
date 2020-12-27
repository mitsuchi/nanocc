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

assert 30 "add(x,y){return x+y;} main(){return add(10,20);}"
assert 30 "main(){return triple(10);} triple(x){return x*3;}"
assert 42 "fourtytwo(){return 42;} main(){return fourtytwo();}"
assert 0 "main(){return 0;}"
assert 42 "main(){return 42;}"
assert 3 "main(){return 1+2;}"
assert 7 "main(){1+2; return 3+4;}"
assert 45 "main(){a = 0; b = 0; while (a < 9) {a = a + 1; b = b + a;} return b;}"
assert 45 "main(){b = 0; for (a = 0; a < 10; a = a + 1) b = b + a; return b;}"
assert 8 "main(){a = 1; while (a < 5) a = a + a; return a;}"
assert 3 "main(){if (1 < 2) return 3;}"; # 最新のテストほど先頭に持ってくるように変更
assert 3 "main(){if (1 < 2) return 3; else return 4;}";
assert 4 "main(){if (2 < 1) return 3; else return 4;}";
assert 25 "main(){return 5+20;}"
assert 21 "main(){return 5+20-4;}"
assert 7 "main(){return 1+2*3;}"
assert 5 "main(){return 1*2+3;}"
assert 2 "main(){return 1+2*3/4;}"
assert 14 "main(){return 2*(3+4);}"
assert 94 "main(){return 2*(3+(4*(5+6)));}"
assert 2 "main(){return -3+5;}"
assert 2 "main(){return -(3+5)+10;}"
assert 5 "main(){return -3*+5 + 20;}"
assert 1 "main(){return 12==12;}"
assert 0 "main(){return 123==12;}"
assert 1 "main(){return 45!=12;}"
assert 0 "main(){return 45!=45;}"
assert 0 "main(){return 10 < 9;}"
assert 0 "main(){return 10 < 10;}"
assert 1 "main(){return 10 < 11;}"
assert 0 "main(){return 10 <= 9;}"
assert 1 "main(){return 10 <= 10;}"
assert 1 "main(){return 10 <= 11;}"
assert 1 "main(){return 10 > 9;}"
assert 0 "main(){return 10 > 10;}"
assert 0 "main(){return 10 > 11;}"
assert 1 "main(){return 10 >= 9;}"
assert 1 "main(){return 10 >= 10;}"
assert 0 "main(){return 10 >= 11;}"
assert 14 "main(){a = 3; b = 5 * 6 - 8; return a + b / 2;}"
assert 42 "main(){return 42;}";
assert 31 "main(){return 5*6+1; 42;}";

echo OK
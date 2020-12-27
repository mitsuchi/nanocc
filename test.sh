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

assert 42 "int main(){int x; x = 42; return x;}";
assert 3 "int main(){int x; int y; int z; x = 3; y = 5; z = &y + 8; return *z;}" # y の8バイト上に x がある
assert 3 "int main(){int x; int y; x = 3; y = &x; return *y;}"
assert 55 "int fib(int n){if(n < 2) {return 1;} else {return (fib(n-1) + fib(n-2));}} int main(){return fib(9);}"
assert 30 "int add(int x,int y){return x+y;} int main(){return add(10,20);}"
assert 30 "int main(){return triple(10);} int triple(int x){return x*3;}"
assert 42 "int fourtytwo(){return 42;} int main(){return fourtytwo();}"
assert 0 "int main(){return 0;}"
assert 42 "int main(){return 42;}"
assert 3 "int main(){return 1+2;}"
assert 7 "int main(){1+2; return 3+4;}"
assert 45 "int main(){int a; int b; a = 0; b = 0; while (a < 9) {a = a + 1; b = b + a;} return b;}"
assert 45 "int main(){int a; int b; b = 0; for (a = 0; a < 10; a = a + 1) b = b + a; return b;}"
assert 8 "int main(){int a; a = 1; while (a < 5) a = a + a; return a;}"
assert 3 "int main(){if (1 < 2) return 3;}"; # 最新のテストほど先頭に持ってくるように変更
assert 3 "int main(){if (1 < 2) return 3; else return 4;}";
assert 4 "int main(){if (2 < 1) return 3; else return 4;}";
assert 25 "int main(){return 5+20;}"
assert 21 "int main(){return 5+20-4;}"
assert 7 "int main(){return 1+2*3;}"
assert 5 "int main(){return 1*2+3;}"
assert 2 "int main(){return 1+2*3/4;}"
assert 14 "int main(){return 2*(3+4);}"
assert 94 "int main(){return 2*(3+(4*(5+6)));}"
assert 2 "int main(){return -3+5;}"
assert 2 "int main(){return -(3+5)+10;}"
assert 5 "int main(){return -3*+5 + 20;}"
assert 1 "int main(){return 12==12;}"
assert 0 "int main(){return 123==12;}"
assert 1 "int main(){return 45!=12;}"
assert 0 "int main(){return 45!=45;}"
assert 0 "int main(){return 10 < 9;}"
assert 0 "int main(){return 10 < 10;}"
assert 1 "int main(){return 10 < 11;}"
assert 0 "int main(){return 10 <= 9;}"
assert 1 "int main(){return 10 <= 10;}"
assert 1 "int main(){return 10 <= 11;}"
assert 1 "int main(){return 10 > 9;}"
assert 0 "int main(){return 10 > 10;}"
assert 0 "int main(){return 10 > 11;}"
assert 1 "int main(){return 10 >= 9;}"
assert 1 "int main(){return 10 >= 10;}"
assert 0 "int main(){return 10 >= 11;}"
assert 14 "int main(){int a; int b; a = 3; b = 5 * 6 - 8; return a + b / 2;}"
assert 42 "int main(){return 42;}";
assert 31 "int main(){return 5*6+1; 42;}";

echo OK
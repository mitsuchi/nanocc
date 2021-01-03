#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  echo "$input" > tmp.c
  ./nanocc tmp.c > tmp.s
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

assert 42 'int main(){ /* hoge */ return 42;}'
assert 97 'int main(){ char *x; x = "abc"; return x[0];}'
assert 98 'int main(){ char *x; x = "abc"; return x[1];}'
assert 3 "int main(){ char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y;}"
assert 30 "int x[2]; int main(){ x[0] = 10; x[1] = 20; return x[0] + x[1];}"
assert 42 "int x; int y; int main() {x = 42; return x;}"
assert 30 "int x; int y; int main() {x = 10; y = 20; return x+y;}"
assert 42 "int a; int **b[10]; int main() {return 42;}"
assert 4 "int main(){return sizeof 1;}"
assert 4 "int main(){int x; return sizeof x;}"
assert 8 "int main(){int *y; return sizeof y;}"
assert 4 "int main(){int x; return sizeof (x+3);}"
assert 8 "int main(){int *y; return sizeof (y+3);}"
assert 4 "int main(){int *y; return sizeof *y;}"
assert 4 "int main(){return sizeof sizeof 1;}"
assert 42 "int main(){int a[1]; a[0] = 42; return a[0];}"
assert 2 "int main(){int a[2]; a[0] = 1; a[1] = 2; return a[1];}"
assert 3 "int main(){int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1);}"
assert 3 "int main(){int x; int y; int *z; x = 3; y = 5; z = &y; return *(z+1);}" # y の8バイト上に x がある
assert 3 "int main(){int x; int y; int *z; x = 3; y = 5; z = &y+1; return *z;}" # y の8バイト上に x がある
assert 3 "int main(){int x; int *y; x = 3; y = &x; return *y;}"
assert 3 "int main(){ int x; int *y; y = &x; *y = 3; return x;}"
assert 7 "int main(){int a; int b; a = 3; b = 4; return a + b;}"
assert 42 "int main(){return 42;}"
assert 42 "int main(){int x; x = 42; return x;}"
assert 42 "int id(int x){return x;} int main(){return id(42);}"
assert 42 "int id(int x){int y; return x;} int main(){return id(42);}"
assert 84 "int double(int x){return x*2;} int main(){return double(42);}"
assert 42 "int main() {int a[3]; *a = 42; return *a;}"
assert 43 "int main() {int a[2]; *a = 42; *(a + 1) = 43; return *(a + 1);}"
assert 3 "int main() {int a[2]; *a = 1; *(a + 1) = 2; return *a + *(a + 1);}"
assert 30 "int add(int x,int y){return x+y;} int main(){return add(10,20);}"
assert 55 "int fib(int n){if(n < 2) {return 1;} else {return (fib(n-1) + fib(n-2));}} int main(){return fib(9);}"
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
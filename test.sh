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

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 7 "1+2*3"
assert 5 "1*2+3"
assert 2 "1+2*3/4"
assert 14 "2*(3+4)"
assert 94 "2*(3+(4*(5+6)))"
assert 2 "-3+5"
assert 2 "-(3+5)+10"
assert 5 "-3*+5 + 20"

echo OK
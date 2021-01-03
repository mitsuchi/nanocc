#!/bin/bash

echo "$1" > tmp.c
./nanocc tmp.c > tmp.s
cc -o tmp tmp.s
./tmp
echo $?
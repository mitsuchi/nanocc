#!/bin/bash

./nanocc test.nanoc > tmp.s
cc -o tmp tmp.s
./tmp

if [ $? = 0 ]; then 
  echo OK
else
  echo NG
fi
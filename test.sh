#!/bin/bash

./nanocc test.nanoc > tmp.s
cc -o tmp tmp.s
./tmp

echo OK
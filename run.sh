#!/bin/sh

make kill
make clean
make all
./yalnix -s -t tracefile -lk 3 -lh 3 -lu 1

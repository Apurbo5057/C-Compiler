#!/bin/bash
flex -o 2105057_flex.cpp 2105057.l
g++ 2105057_flex.cpp -lfl -o 2105057.out
./2105057.out input_output/input3.txt

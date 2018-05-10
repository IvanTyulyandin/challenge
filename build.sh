#!/bin/bash

git clean -dfx

cmake -DCMAKE_BUILD_TYPE=Release .

make

./challenge
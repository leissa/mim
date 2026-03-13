#!/bin/sh

set -eu

mkdir -p release_trie
mkdir -p release_immer
mkdir -p release_set

cmake -S . -B release_trie  -DCMAKE_BUILD_TYPE=Release
cmake -S . -B release_immer -DCMAKE_BUILD_TYPE=Release -DMIM_IMMER=ON
cmake -S . -B release_set   -DCMAKE_BUILD_TYPE=Release -DMIM_STD_SET=ON

cmake --build release_trie  -j $(nproc)
cmake --build release_immer -j $(nproc)
cmake --build release_set   -j $(nproc)

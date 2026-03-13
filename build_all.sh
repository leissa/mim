#!/bin/sh

set -eu

mkdir release_trie
mkdir release_immer
mkdir release_set

cmake -S . -B release_trie  -DCMAKE_BUILD_TYPE=Release
cmake -S . -B release_immer -DCMAKE_BUILD_TYPE=Release -DMIM_IMMER
cmake -S . -B release_set   -DCMAKE_BUILD_TYPE=Release -DMIM_STD_SET

cmake --build release_trie  -j $(nproc)
cmake --build release_immer -j $(nproc)
cmake --build release_set   -j $(nproc)

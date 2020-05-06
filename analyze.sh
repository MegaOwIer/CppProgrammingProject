#!/bin/bash

MyPass=/home/metazero/code/VS-Code-CPP/Project/MyPass.so
clang -Xclang -load -Xclang $MyPass -c $1

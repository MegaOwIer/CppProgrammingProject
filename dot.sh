#!/bin/bash

dot -Tpng -o $1.png $1.dot
dot -Tpng -o $1.flat.png $1.flat.dot
dot -Tpng -o $1.transaction.png $1.transaction.dot

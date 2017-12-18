#!/bin/sh
BASE_DIR=`pwd`

cd "$DIR"
DIR=`pwd`

make -f deps.make slash
make -f deps.make pink

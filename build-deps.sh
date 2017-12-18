#!/bin/sh
BASE_DIR=`pwd`

cd "$DIR"
DIR=`pwd`

make -f deps.make slash
make -f deps.make pink
make -f deps.make jemalloc
make -f deps.make zlib
make -f deps.make snappy
make -f deps.make rocksdb
make -f deps.make bzip2

# VCDB

A redis compatible storage with  high capacity KV storage

## Compile

[![Build Status](https://travis-ci.org/TimothyZhang023/vcdb.svg?branch=master)](https://travis-ci.org/TimothyZhang023/vcdb)

### requirements:  
```
CMake >= 3.1
GCC >= 4.8
```

### Get the source code
```
git clone https://github.com/JRHZRD/swapdb.git --recursive
```

## Build

(you can skip this step if you add '--recursive' option when 'git clone'.) for submodules update process.
```
git submodule update --init --recursive
```

```
sh build-deps.sh && mkdir build && cd build && cmake .. && make -j8
```

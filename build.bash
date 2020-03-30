#!/bin/bash -eux

source_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "$source_path"
mkdir -p build
cd build

if [ "$#" -ne 1 ] ; then
  echo "Building zlib Debug configuration"
  cmake -DCMAKE_BUILD_TYPE=Debug ..
  make
else 
  shopt -s nocasematch
  if [[ "$1" = "debug" ]] ; then
    echo "Building zlib Debug configuration"
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make 
  elif [[ "$1" = "release" ]] ; then
    echo "Building zlib Release configuration"
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
  elif [[ "$1" = "coverage" ]] ; then
    echo "Building zlib Coverage configuration"
    cmake -DCMAKE_BUILD_TYPE=Coverage ..
    make
    make coverage
  elif [[ "$1" = "sanitizer" ]] ; then
    echo "Building zlib Sanitizer configuration"
    cmake -DCMAKE_BUILD_TYPE=Sanitizer ..
    make
    make test ARGS="-V"
  elif [[ "$1" = "valgrind" ]] ; then
    echo "Running Valgrind on zlib"
    cmake -DCMAKE_BUILD_TYPE=Valgrind ..
    make
    ctest -T memcheck
  elif [[ "$1" = "test" ]] ; then
    echo "Running zlib tests"
    cmake -DCMAKE_BUILD_TYPE=Test ..
    make
    make test ARGS="-V"
  elif [[ "$1" = "clean" ]] ; then
    echo "Cleaning zlib build"
    cd "$source_path"
    rm -rf build
  else
    echo "Unrecognized argument $1"
  fi
fi

cd "$source_path"

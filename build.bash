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
  if [[ "$1" = "coverage" ]] ; then
    echo "Building zlib Coverage configuration"
    cmake -DCMAKE_BUILD_TYPE=Coverage ..
    make
    make coverage
  elif [[ "$1" = "valgrind" ]] ; then
    echo "Running Valgrind on zlib"
    cmake -DCMAKE_BUILD_TYPE=Valgrind ..
    make
    valgrind --leak-check=full -v ./zlib_gtest
  elif [[ "$1" = "save" ]] ; then
    echo "Saving testing results"
    if [[ -e ./coverage ]] ; then
    	cp -r ./coverage/* $source_path/test/output/coverage/
    else
    	echo "No coverage results found"
    fi
    if [[ -e ./Testing/Temporary/LastTest.log ]] ; then
        cp ./Testing/Temporary/LastTest.log $source_path/test/output/Test.log
    else
    	echo "No test results found"
    fi
  elif [[ "$1" = "cfstest" ]] ; then
    echo "Configuring for cFS and running zlib tests"
    cmake -DCMAKE_BUILD_TYPE=CfsTest ..
    make
    make test ARGS="-V"
  elif [[ "$1" = "test" ]] ; then
    echo "Running zlib tests"
    cmake -DCMAKE_BUILD_TYPE=Test ..
    make
    make test ARGS="-V"
  elif [[ "$1" = "cobra" ]] ; then
    echo "Running all cobra tests (assumes cobra is configured)"
    cobra -f basic -I$source_path/include -I$source_path/build $source_path/src/*.c
    cobra -f misra2012 -I$source_path/include -I$source_path/build $source_path/src/*.c
    cobra -f p10 -I$source_path/include -I$source_path/build $source_path/src/*.c
    cobra -f jpl -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "cobra-basic" ]] ; then
    echo "Running basic cobra tests (assumes cobra is configured)"
    cobra -f basic -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "cobra-misra" ]] ; then
    echo "Running misra cobra tests (assumes cobra is configured)"
    cobra -f misra2012 -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "cobra-p10" ]] ; then
    echo "Running p10 cobra tests (assumes cobra is configured)"
    cobra -f p10 -I$source_path/include -I$source_path/build $source_path/src/*.c  
  elif [[ "$1" = "cobra-jpl" ]] ; then
    echo "Running jpl cobra tests (assumes cobra is configured)"
    cobra -f jpl -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "clean" ]] ; then
    echo "Cleaning zlib build"
    cd "$source_path"
    rm -rf build
  else
    echo "Unrecognized argument $1"
  fi
fi

cd "$source_path"

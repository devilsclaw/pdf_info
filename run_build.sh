#!/bin/bash

if ! [ -d build ] ; then
  mkdir build
fi

cd build

cmake ..

make && ./pdf_info ../sample-forms/OoPdfFormExample.pdf

cd -

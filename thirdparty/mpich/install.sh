#!/bin/bash

tar -xzf mpich2-1.4.1p1.tar.gz
cd mpich2-1.4.1p1/

echo "Configuring MPICH2"
export CC="gcc -fPIC"
export CXX="g++ -fPIC"
./configure --enable-shared

echo "Building MPICH2"
make

echo "Installing MPICH2... You will likely be asked for your password"
sudo make install

#!/bin/bash

tar -xzf 3.2.2.tar.gz
cd eigen*

mkdir bin
cd bin/

cmake ../
sudo make install

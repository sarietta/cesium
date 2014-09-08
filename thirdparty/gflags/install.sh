#!/bin/bash

# Since we know that we have both Google Flags and Google Logging, and
# we need to install gflags first to get our logging flags, we install
# both here and mimic this behavior in the glog install.sh.

echo "Installing Google Flags"

cd ../gflags
tar -xzf gflags-2.0.tar.gz
cwd=`pwd`
cd gflags-2.0
./configure && make && sudo make install
cd ${cwd}

echo "Installing Google Logging"

cd ../glog
tar -xzf glog-0.3.2.tar.gz
cd glog-0.3.2

./configure && make && sudo make install

#!/bin/bash

# Determine arch
arch=`uname -m`;

file='';
if [ "$arch" == "x86_64" ]; then
    file='MCR_R2012a_glnxa64_installer.zip';
elif [ "$arch" == "i686" ]; then
    file='MCR_R2012a_glnx86_installer.zip';
else
    echo "Could not determine architecture of system. Is this a 32bit or 64bit machine?";
    exit 1;
fi

wget "http://ravi-server1.cs.berkeley.edu/store/${file}";
mkdir ./MCR_2012a
unzip -d ./MCR_2012a $file;

cd ./MCR_2012a

read -p "Where you like to install MATLAB runtime? [/usr/local/MATLAB/R2012a]" path
if [ "$path" == "" ]; then
    path='/usr/local/MATLAB/R2012a';
fi

echo "Installing MATLAB runtime to: $path";

sudo mkdir -p $path
sudo ./install -mode silent -agreeToLicense yes -destinationFolder $path

# MATLAB is dumb and puts things in a subfolder of the path specified.
if [ ! -f ${path}/extern/include/mat.h ]; then
    folder=`ls ${path}`;
    sudo mv ${path}/${folder} /tmp/slib_install_temp_folder
    sudo mv /tmp/slib_install_temp_folder/* ${path}
fi

# MATLAB also has crappy old version of c++ lib.
sudo mv ${path}/sys/os/glnxa64/libstdc++.so.6 ${path}/sys/os/glnxa64/libstdc++.so.6.old

setvars=0;
read -p "Do you want me to set your environment variables for MATLAB?" yn
case $yn in
    [Yy]* ) setvars=1;;
    [Nn]* ) setvars=0;;
    * ) echo "Please answer yes or no.";;
esac

if [ $setvars -eq 1 ]; then
    read -p "What file do you use to save environment variables? [${HOME}/.bash_profile]" rcfile
    if [ "$rcfile" == "" ]; then
	rcfile="${HOME}/.bash_profile";
    fi

    echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${path}/bin/glnxa64/" >> $rcfile
    echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${path}/sys/os/glnxa64/" >> $rcfile
    
    echo "export MATLAB_INCLUDE_DIR=${path}/extern/include" >> $rcfile
    echo "export MATLAB_LIB_DIR=${path}/bin/glnxa64" >> $rcfile

    source $rcfile
fi

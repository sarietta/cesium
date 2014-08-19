#!/bin/bash

# Let's start out by trying to determine the package manager on this system.
pkgmanager='';
command -v apt-get > /dev/null 2>&1 && pkgmanager='apt-get';
command -v yum > /dev/null 2>&1 && pkgmanager='yum';

if [ "$pkgmanager" == "" ]; then
    echo "I couldn't determine what program to use for installing libraries, etc on your system (I tried apt-get and yum).";    
    read -p "What is the name of your package manager? " pkgmanager

    while [ "$(command -v $pkgmanager 2>&1)" == "" ]; do
	echo "Couldn't find command: $pkgmanager";
	read -p "What is the name of your package manager? " pkgmanager
    done
fi

echo -e "*** Found system package manager: $pkgmanager";

# First we need to install cmake if it does not exist.
command -v cmake > /dev/null 2>&1 || ( \
    echo -e 'IMPORTANT: cmake not found on this system\nYou need to install cmake via "sudo apt-get install cmake", "sudo yum install cmake", etc depending on your system.' \
    && exit );

# Let's see if cmake can successfully compile slib.
echo -e "*** Attempting to run cmake (output is disabled)...";
cmake . -DSKIP_TESTS > /dev/null 2>&1;
code=$?;

autoresolve=0;
if [ $code -ne 0 ]; then
    echo -e "\nIt looks like cmake failed to create a Makefile for slib. This is almost certainly due to some missing libraries on your system.";

    read -p "Would you like me to try to automatically determine which libraries are missing? [y/n]" yn
    case $yn in
        [Yy]* ) autoresolve=1;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
fi

if [ $autoresolve -eq 1 ]; then
    echo -e "\n*** Attempting to automatically determine missing libraries...";
    
    # Find all of the libraries used.
    libraries=();
    while read line; do
	library=`echo $line | awk '{print $3}'`;
	libraries=("${libraries[@]}" "${library}")
    done < <(cat CMakeLists.txt | grep "SLIB_REQUIRED")

    unique_libraries=$(echo "${libraries[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' ')
    libraries=("${unique_libraries[@]}");

    # Try to determine which ones are missing.
    for library in ${libraries[@]}; do

	# See if there is a customer check routine.
	check=`cat CMakeLists.txt | grep "SLIB_CHECK" | grep "$library"`;
	success=0;
	if [ "$check" == "" ]; then
	    # Default to cmake check routine.
	    cmake --find-package -DNAME=${library} -DCMAKE_MODULE_PATH=./cmake \
		-DLANGUAGE=C -DCOMPILER_ID=GNU -DMODE=COMPILE > /dev/null 2>&1;
	    if [ $? -eq 0 ]; then
		success=1;
	    fi
	else
	    routine=`echo $check | awk -F\" '{print $2}'`;
	    success=`eval $routine`;
	    if [ "$success" == "" ]; then
		success=0;
	    else
		success=1;
	    fi
	fi

	if [ $success -eq 0 ]; then
	    echo "Could not find $library, but it is required.";

	    install=0;
	    read -p "Would you like me to try to install it for you? [y/n]" yn	    
	    case $yn in
		[Yy]* ) install=1;;
		[Nn]* ) install=0;;
		* ) echo "Please answer yes or no.";;
	    esac

	    if [ $install -eq 1 ]; then
		type=`cat CMakeLists.txt | grep "SLIB_REQUIRED" | grep "$library" | awk '{print $4}'`;

		if [ "$type" == "bundle" ]; then
		    cwd=`pwd`;
		    path=`cat CMakeLists.txt | grep "SLIB_REQUIRED" | grep "$library" | awk '{print $5}'`;
		    cd $path;
		    ./install.sh
		    if [ $? -ne 0 ]; then
			echo "Installation of ${library} failed. I tried my best to install it for you, but you might need to Google how to install it and do it yourself. Sorry about that :(.";
			read -p "Press enter to continue...";
		    fi
		    cd $cwd;
		elif [ "$type" == "pkg" ]; then
		    pkg=`cat CMakeLists.txt | grep "SLIB_REQUIRED" | grep "$library" | awk '{print $5}'`;
		    sudo ${pkgmanager} install ${pkg};
		    if [ $? -ne 0 ]; then
			echo "Installation of ${library} failed. I tried my best to install it for you, but you might need to Google how to install it and do it yourself. Sorry about that. :(";
			read -p "Press enter to continue...";
		    fi
		elif [ "$type" == "manual" ]; then
		    echo "Unfortunately this library cannot be bundled with slib and is not available via your package manager. You will need to install this on your own.";
		    echo "This typically happens when the library in question costs money or requires you to register to download it. :(";
		    read -p "Press enter to continue...";
		    continue;
		else
		    echo "Well this is embarrassing... I don't know how to install ${library}. This shouldn't happen though, so maybe you edited CMakeLists.txt?".
		    read -p "Press enter to continue...";
		    continue;
		fi
	    fi
	fi
    done

fi  # if [ $autoresolve -eq 1 ];

# Try to run cmake again.
echo -e "*** Attempting to run cmake (output is disabled)...";
cmake . -DSKIP_TESTS > /dev/null 2>&1;

if [ $? -ne 0 ]; then
    echo "Still can't run cmake. Are you missing libraries still?";
    exit;
fi

make
if [ $? -ne 0 ]; then
    echo "Make failed. Are you missing libraries still?";
    exit;
fi


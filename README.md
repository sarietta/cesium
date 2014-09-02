![](assets/logo.100.png) 

# Cesium

Cesium (roughly speaking) is a distributed visual processing framework. More informally, it's a set of C++ classes that make it relatively easy to run visual processing operations on a set of images in a distributed environment. Actually, the "operations" can be anything you want, but the library includes some nice classes / functions that make it particularly amenable to those types of operations.

### Installation

In theory you should be able to execute the install script in the root directory:

```$> ./install.sh```

We wrote this script to make the install process as easy as possible. It tries very hard to automatically correct any compilation issues. That said, this is the land of C++ which means that it's nearly impossible to guarantee this will always work. If you find a case the script doesn't handle, please open an issue and we will try to provide a workaround.

If the install script fails, it's likely that the library you need is include in the `thirdparty` directory. There is an `install.sh` script in each of those that will hopefully take care of installation for you.

##### Prerequisites
Here are the prerequisites in case you care/need to know them if/when the install scripts fail (most of these are included in `thirdparty`):

* Boost >= 1.46
* Eigen3
* MATLAB Compiler Runtime (it's free!)
* Google Flags
* Google Logging
* MPICH2

These libraries may be useful but are not strictly required:

* CUDA
 * Only needed when using those badass [Caffe](http://caffe.berkeleyvision.org/) features
* CBLAS
* FFTW3
 * You **must** compile with single-precision float support
* X11 Development Libraries
* ARPACK 
 * Only needed when you want super fast eigenvalue solver (`code/matrix/eigs.cc`)

### Quick Tutorial

This source code is also available in `code/cesium/test_cesium_tutorial.cc`.

```

```

## A Brief FAQ

#### Is it Cross-Platform?
First off, Cesium was developed with an explicit intention that it could be used in a variety of different environments. Wherever possible we tried to avoid using non-standard libraries / functions / etc. Of course it does have some dependencies, but any quasi-current Linux distribution should have no problem getting Cesium to run.

It does rely on a distributed filesystem, but it doesn't have to be anything special; NFS is perfectly fine and is what we use on our local cluster. 

#### Is it Faster than X?
Cesium is pretty good at processing a static set of images using a large heterogeneous cluster. It can send data to processing nodes pretty efficiently, but it's not super optimized at the moment so there may be some latency / bandwidth imperfections. One advantage it has over MapReduce-like frameworks is that it doesn't perform the expensive Shuffle and Reduce operations, which are often unnecessary and time-consuming.

#### Do I Have to Use C++?

Strictly speaking, yes. Python bindings will be available in the future. 

One thing worth noting is that Cesium was designed to interface with MATLAB at a very low level. In fact, all data passed between nodes (currently) is encapsulated in an object called `MatlabMatrix` which mimics actual matrices in MATLAB but with fast serialization and a friendly interface.

#### Why Are There So Many Classes in This Library?

This is partly to address FAQ #1. More generally, we find that a lot of operations that should be simple just aren't. One could make the argument that these extra features should be in a separate library, and that's fine, but what's the harm in including them? The pros seem to outweigh the cons in this instance.

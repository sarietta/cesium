![](assets/logo.100.png) # Cesium

Cesium is a distributed visual processing framework.

Distributed :: Cesium takes an arbitrarily large/small set of machines
that have MPI installed and turns them into one abstracted computation
entity

Visual Processing :: Cesium is meant to operate on a large number of
images, each of which is usually very small in terms of storage. Our
framework includes many common image processing operations already
built-in

Framework :: Cesium is designed to be easy-to-use. Starting jobs,
handling data, and interfacing with common external programs
(i.e. MATLAB) is all easily handled with simple routines and API
calls.

Some features of Cesium include:

- Smart scheduling of individual visual processing tasks
- Automatic checkpointing
- Automatic caching both in-memory and on disk
- A mechanism for processing variables/datasets that don’t fit in memory directly
- A fast low-level communication layer implemented in C++ and MPI
- Seamless integration with MATLAB


=======================================================================

NOTE - Cesium was designed in a research setting. It was designed to
be stable and easy-to-use but has not been rigoroursly tested to be
used in production environments.

[![Build Status](https://travis-ci.org/catsop/python-sopnet.svg?branch=master)](https://travis-ci.org/catsop/python-sopnet)
===========

Submodules
----------

If you haven't done so already, make sure that all submodules are up-to-date:

```sh
$ git submodule update --init
```

Dependencies
------------

The build process is managed by cmake of which you need at least version 3.

All packages except for the vigra library and the Gurobi solver can be
installed using the default Ubuntu repositories for your release:

* [12.04 Precise](packagelist-ubuntu-12.04-apt.txt)
* [14.04 Trusty](packagelist-ubuntu-14.04-apt.txt)

Make sure your version of libboost includes libboost-timer-dev. Optional HDF5
support is enabled if you have libhdf5-serial-dev installed.

A recent version of vigra will be downloaded and compiled automatically.

To compile the python wrappers (pysopnet), you need additionally:

* libboost-python-dev
* libpython-dev

Instructions are available for installing dependencies for
[Scientific Linux 6.5](https://github.com/catsop/python-sopnet/wiki/Compiling-in-Scientific-Linux-6.5).
Python-sopnet also compiles in MacOS X, but dependencies must be found through
Homebrew or Macports.

Gurobi Solver
-------------

Download and unpack the Gurobi solver, request a licence (academic licences are
free). Run

```sh
$ ./grbgetkey <you-licence-id>
```

in the gurobi bin directory from an academic domain to download the license
file (gurobi.lic). Make sure the environment variable `GRB_LICENCE_FILE` points
to it. Set the cmake variable `Gurobi_ROOT_DIR` to the path containing the lib
and bin directory or set the environment variable `GUROBI_ROOT_DIR` accordingly
before calling cmake.

Compiling
---------

If you are just interested in the python wrappers, it is enough to call

```sh
$ python setup.py install
```

from the repository's main directory. This will launch cmake and install the
wrappers, given that all dependencies are fulfilled. You might have to run the
above command as root to install the wrappers in the python system directory.

For a standard build, create a build directory (e.g., ./build), change into it
and type

```sh
$ cmake [path_to_sopnet_directory (e.g. '..')]
```

Cmake will try to find the required packages and tell you which ones are
missing. After cmake finishes without errors, run

```sh
$ make
```
Python-sopnet compiles in gcc, clang, and icc.

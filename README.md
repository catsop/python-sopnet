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

All packages except for the vigra library and the Gurobi solver can be
installed using the default Ubuntu repositories:

* libboost-all-dev (make sure libboost-timer-dev is included)
* liblapack-dev
* libx11-dev
* libx11-xcb-dev
* libxcb1-dev
* libxrandr-dev
* libxi-dev
* freeglut3-dev
* libglew1.6-dev
* libcairo2-dev
* libpng12-dev
* libmagick++-dev
* libcurl4-openssl-dev
* libtiff4-dev
* libhdf5-serial-dev (optional)

A recent version of vigra will be downloaded and compiled automatically.

To compile the python wrappers (pysopnet), you need additionally:

* libboost-python-dev
* libpython-dev

The build process is managed by cmake of which you need at least version 2.8.8.

Gurobi Solver
-------------

Download and unpack the Gurobi solver, request a licence (academic licences are
free). Run

```sh
$ ./grbgetkey <you-licence-id>
```

in the gurobi bin directory from an academic domain to download the licence
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
missing. After cmake finished without errors, run

```sh
$ make
```


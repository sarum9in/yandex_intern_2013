Building
========

How to build
------------

$ ./configure.sh {optional build directory, if not specified, $PWD/build will be used}
$ make -C {build directory}

After these steps "${build directory}/sort" may be used to sort binary files consists of uint32_t (native binary encoded).

Requirements
------------

Build
~~~~~
This project requires bash, git, cmake>=2.8, make, g++>=4.7 and boost>=1.50.

Run
~~~
This project requires boost.

Boost
~~~~~
This project requires the following boost libraries: system, filesystem, serialization, unit_test_framework and thread.

Submodules
~~~~~~~~~~
bunsan_common, yandex_contest_common and yandex_contest_system ("no_lxc" branch) projects are used as submodules.
They will be fetched by "configure.sh".

Archlinux
---------

Project was tested under Archlinux amd64 (updated 10.04.2013).
Required packages:

    git cmake boost base-devel.

Ubuntu
------

Project was tested under Ubuntu 12.10 amd64.
Required packages:

    git cmake
    g++
    g++-4.7
    libboost-system1.50-dev libboost-system1.50.0
    libboost-filesystem1.50-dev libboost-filesystem1.50.0
    libboost-serialization1.50-dev libboost-serialization1.50.0
    libboost-test1.50-dev libboost-test1.50.0
    libboost-thread1.50-dev libboost-thread1.50.0

Usage
=====

sort
----

$ sort {source file} {destination file}

Make sure that directory with {destination file} is writable.
Directory with unspecified name will be created for temporary files (will be removed after termination).

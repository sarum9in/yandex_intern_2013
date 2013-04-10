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
This project requires bash, git, cmake, make, g++>=4.7 and boost.

Run
~~~
This project requires boost.

Boost
~~~~~
This project required the following boost libraries: system, filesystem, serialization, unit_test_framework and thread.

Submodules
~~~~~~~~~~
bunsan_common, yandex_contest_common and yandex_contest_system ("no_lxc" branch) projects are used as submodules.
They will be fetched by "configure.sh".

Sort usage
==========

$ sort {source file} {destination file}

Make sure that directory with {destination file} is writable.
Directory with unspecified name will be created for temporary files (will be removed after termination).

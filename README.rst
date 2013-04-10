Building
========

$ ./configure {optional build directory, if not specified, $PWD/build will be used}
$ make -C {build directory}

After these steps "${build directory}/sort" may be used to sort binary files consists of uint32_t (native binary encoded).

Sort usage
==========

$ sort {source file} {destination file}

Make sure that directory with {destination file} is writable.
Directory with unspecified name will be created for temporary files (will be removed after termination).

# Prerequisites

These are the base requirements to build and use qsfs from a source package (as described below): 
- GNU Compiler Collection (GCC) 4.1.2 or later
- CMake v3.0 or later

### Additional Requirements
**qsfs** is integrated with QingStor via the [QingStor SDK for C++][qs-sdk-cpp link]. qsfs uses glog for logging and gtest for unit testing, qsfs will download and install them under the project's build directory as part of the CMake build's configure step, so you can just leave them alone.

qsfs is a fuse based filesystem, so you must have libfuse installed.

Typically, you'll find these packages in your system's package manager.

To install these packages on Ubuntu:
```sh
 $ [sudo] apt-get install g++ fuse libfuse-dev
```

To install these packages on CentOS:
```sh
 $ [sudo] yum install gcc-c++ fuse fuse-devel
```

You may also need to install [git][git install link] in order to clone the source code from GitHub. You also need to install [CMAKE][cmake install link] in order to build from source.

# Build from Source using CMake

Clone the qsfs source from [yunify/qsfs][qsfs github link] on GitHub:
```sh
 $ git clone https://github.com/jimhuaang/qsfs.git
```

Enter the project directory containing the package's source code:
```sh
 $ cd qsfs
```

It is always a good idea to not pollute the source with build files,
so create a directory in which to create your build files:
```sh
 $ mkdir build
 $ cd build
```

Run cmake to configure and install dependencies, NOTICE as we install
dependencies in cmake configure step, the terminal will wait for you
to type password in order to get root privileges:
```sh
 $ cmake ..
 $ make
```

Notice, if you want to enable unit test, specfiy -DBUILD_TESTS=ON in cmake configure step; you can specify build type such as -DCMAKE_BUILD_TYPE=Debug; you can specify -DINSTALL_HEADERS to request installations of headers and other development files:
```sh
 $ cmake -DBUILD_TESTS=ON ..
 $ cmake -DCMAKE_BUILD_TYPE=Debug ..
 $ cmake -DINSTALL_HEADERS ..
```

Run make to build:
```sh
 $ make
```

Run unit tests:
```sh
 $ make test
```
  or
```sh
 $ ctest -R qsfs -V
```

Install the programs and any data files and documentation:
```sh
 $ [sudo] make install
```

To clean the generated build files, just remove folder build:
```sh
 $ rm -rf build
```

To clean program binaries, juse remove folder bin:
```sh
 $ rm -rf bin
```

To remove all installed files:
```sh
 $ [sudo] make uninstall
```

To only remove installed qsfs files:
```sh
 $ [sudo] make uninstall_qsfs
```

To remove installed depended project's files, such as googletest, googleflags, goolgelog and sdk:
```sh
 $ [sudo] make uninstall_googletest
 $ [sudo] make uninstall_googlelog
 $ [sudo] make uninstall_googleflags
 $ [sudo] make uninstall_qingstorsdk
```


[qsfs github link]: https://github.com/yunify/qsfs
[qs-sdk-cpp link]: https://github.com/yunify/qingstor-sdk-cpp
[git install link]: https://git-scm.com/book/en/v2/Getting-Started-Installing-Git
[cmake install link]: https://cmake.org/install/
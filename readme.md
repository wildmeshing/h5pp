[![Build Status](https://travis-ci.org/DavidAce/h5pp.svg?branch=master)](https://travis-ci.org/DavidAce/h5pp)

# h5pp
`h5pp` is a C++17 header-only wrapper for HDF5 with focus on simplicity.


In just a few lines of code, `h5pp` lets users read and write to disk in binary format. It supports complex data types in possibly multidimensional containers that are common in scientific computing.
In particular, `h5pp` makes it easy to store [**Eigen**](http://eigen.tuxfamily.org) matrices and tensors.


## Features
* Standard CMake build, install and linking using targets.
* Supports Clang and GNU GCC.
* Automated install and linking of dependencies, if desired.
* Support for common data types:
    - `char`,`int`, `float`, `double` in unsigned and long versions.
    - any of the above in std::complex<> form.
    - any of the above in C-style arrays.
    - `std::vector`
    - `std::string`
    - `Eigen` types such as `Matrix`, `Array` and `Tensor` (from the unsupported module), with automatic conversion to/from row major storage layout.
    - Other containers with a contiguous buffer (without conversion to/from row major).



## Requirements
* C++17 capable compiler (tested with GCC version >= 8 and Clang version >= 7.0)
* CMake (tested with version >= 3.10)
* Dependencies (optional automated install available through CMake):
    - [**HDF5**](https://support.hdfgroup.org/HDF5/) (tested with version >= 1.10).
    - [**Eigen**](http://eigen.tuxfamily.org) (tested with version >= 3.3.4).
    - [**spdlog**](https://github.com/gabime/spdlog) (tested with version >= 1.3.1)

The build process will attempt to find the libraries above in the usual system install paths.
By default, CMake will warn if it can't find the dependencies, and the installation step will simply copy the headers to `install-dir` and generate a target `h5pp::h5pp` for linking.

For convenience, `h5pp` is also able to download and install the missing dependencies for you into the given install-directory (default: `install-dir/third-party`),
and add these dependencies to the exported target `h5pp::deps`. To enable this automated behavior read more about [build options](#build-options) and [linking](#linking) targets below.



## Usage

To write a file simply pass any supported object and a dataset name to `writeDataset`.

Reading works similarly with `readDataset`, with the only exception that you need to give a container of the correct type beforehand. You can query the *size* in advance, but there is no support (yet?)
for querying the *type* in advance. The container will be resized appropriately by `h5pp`.


```c++

#include <iostream>
#include <h5pp/h5pp.h>
using namespace std::complex_literals;

int main() {
    
    // Initialize a file
    h5pp::File file("myDir/someFile.h5");


    // Write a vector with doubles
    std::vector<double> testVector (5, 10.0);
    file.writeDataset(testVector, "testVector");

    // Or write it using C-style array pointers together with its size
    file.writeDataset(testVector.data(),testVector.size(),"testVector" )


    // Write an Eigen matrix with std::complex<double>
    Eigen::MatrixXcd testMatrix (2, 2);
    testMatrix << 1.0 + 2.0i,  3.0 + 4.0i, 5.0 + 6.0i , 7.0 + 8.0i;
    file.writeDataset(testMatrix, "someGroup/testMatrix");



    // Read a vector with doubles
    std::vector<double> readVector;
    file.readDataset(readVector, "testVector");

    // Or read it by assignment in one line
    readVector = file.readDataset<std::vector<double>> ("testVector");


    // Read an Eigen matrix with std::complex<double>
    Eigen::MatrixXcd readMatrix;
    file.readDataset(readMatrix, "someGroup/testMatrix");



    return 0;
}

```

Writing attributes of any type to a group or dataset (in general "links") works similarly with the method `writeAttributeToLink(someObject,attributeName,targetLink)`.


###


### File permissions
To define permissions use the settings `AccessMode::<mode>` and/or `CreateMode::<mode>` as arguments when initializing the file.
The possible modes are
* `AccessMode::`
    - `READONLY`  Read permission to the file.
    - `READWRITE` **(default)** Read and write permission to the file.
* `CreateMode::`
    - `OPEN` Open the file with the given name. Throws an error when file does not exist.
    - `RENAME` **(default)** File is created with an available file name. If `myFile.h5` already exists, `myFile-1.h5` is created instead. The appended integer is increased until an available name is found
    - `TRUNCATE` File is created with the given name and erases any pre-existing file. 

The defaults are chosen to avoid loss of data.
To give a concrete example, the syntax works as follows
```c++
    h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE);
```

### Extendable and non-extendable datasets (new)
By default, datasets in h5pp are created as non-extendable. This means that a dataset has a fixed size and can only be overwritten if the new data has the same size and shape.
In contrast, extendable datasets have dynamic size and can be overwritten by a larger dataset. Keep in mind that overwriting with a smaller dataset does not shrink the file size.

To swap the default behavior, use one of the methods below
```c++
    file.enableDefaultExtendable();
    file.disableDefaultExtendable();
```

You can also optionally pass a true/false argument when writing a new dataset to explicitly create it as extendable or non-extendable

```c++
    file.writeDataset(testvector, "testvector", true);      // Creates an extendable dataset
    file.writeDataset(testvector, "testvector", false);     // Creates a non-extendable dataset    
```

**Technical details:** 
- Extendability only applies for datasets with one or more dimensions. Zero-dimensional or "scalar" datasets are always as non-extendable (as `H5S_SCALAR`).
- Extendable datasets are "chunked" (as in `H5D_CHUNKED`), which means they can be read into memory in smaller chunks. This makes sense for large enough datasets.
- A dataset with one or more dimensions is **non-extendable by default, unless it is very large**.
- A non-extendable dataset smaller than 32 KB will be created as `H5D_COMPACT`, meaning it can fit in the metadata header.
- A non-extendable dataset between 32 KB and 512 KB will be created as `H5D_CONTIGUOUS`, meaning it is not "chunked" and can be read entirely at once.
- A non-extendable dataset larger than 512 KB will be **made into an extendable dataset** unless explicitly specified, because it makes sense to read large datasets in chunks.



### Pro-tip: load into Python using h5py
HDF5 data is easy to load into Python. Loading integer and floating point data is straightforward. Complex data is almost as simple to use.
HDF5 does not support complex types specifically, but `h5pp`enables this through compound HDF5 types. Here is a python example which uses `h5py`
to load 1D arrays from an HDF5 file generated with `h5pp`:


```python
import h5py
import numpy as np
file  = h5py.File('myFile.h5', 'r')

# Originally written as std::vector<double> in h5pp
myDoubleArray = np.asarray(file['double-array-dataset'])                                     

# Originally written as std::vector<std::complex<double>> in h5pp
myComplexArray = np.asarray(file['complex-double-array-dataset'].value.view(dtype=np.complex128)) 
```

Pay attention to the cast to `dtype=np.complex128` which interprets each element of the array as two `doubles`, i.e. the real and imaginary parts are `2 * 64 = 128` bits.  



## Installation
Build the library just as any CMake project:

```bash
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=<install-dir> -DDOWNLOAD_MISSING=ON ../
    make
    make install
    make examples
```

By passing the variable `DOWNLOAD_MISSING=ON` CMake will download all the dependencies and install them under `install-dir/third-party` if not found in the system. 
By default `ìnstall-dir` will be `project-dir/install`, where `project-dir` is the directory containing the main `CMakeLists.txt` file. And of course, making the examples is optional.

### Build options

The `cmake` step above takes several options, `cmake [-DOPTIONS=var] ../ `:
* `-DCMAKE_INSTALL_PREFIX:PATH=<install-dir>` to specify install directory (default: `project-dir/install/`).
* `-DBUILD_SHARED_LIBS:BOOL=<ON/OFF>` to link dependencies with static or shared libraries (default: `OFF`)
* `-DCMAKE_BUILD_TYPE=Release/Debug` to specify build type of tests and examples (default: `Release`)
* `-DENABLE_TESTS:BOOL=<ON/OFF>` to run ctests after build (recommended!) (default: `OFF`).
* `-DBUILD_EXAMPLES:BOOL=<ON/OFF>` to build example programs (default: `OFF`)
* `-DDOWNLOAD_MISSING:BOOL=<ON/OFF>` to toggle automatic installation of all third-party dependencies (default: `OFF`).


In addition, the following variables can be set to help guide CMake's `find_package()` to your preinstalled software (no defaults):

* `-DEigen3_DIR:PATH=<path to Eigen3Config.cmake>` 
* `-DEigen3_ROOT_DIR:PATH=<path to Eigen3 install-dir>` 
* `-DEIGEN3_INCLUDE_DIR:PATH=<path to Eigen3 include-dir>`
* `-DHDF5_DIR:PATH=<path to HDF5Config.cmake>` 
* `-DHDF5_ROOT:PATH=<path to HDF5 install-dir>` 
* `-Dspdlog_DIR:PATH=<path to spdlogConfig.cmake>` 



### Linking to your CMake project

#### By using CMake targets (easy)
`h5pp` is easily imported into your project using CMake's `find_package()`. Just point it to the `h5pp` install directory.
When found, targets are made available to compile and link to dependencies correctly.
A minimal `CMakeLists.txt` to use `h5pp` would look like:


```cmake
    cmake_minimum_required(VERSION 3.10)
    project(myProject)
    add_executable(myExecutable main.cpp)
    find_package(h5pp PATHS <path-to-h5pp-install-dir> REQUIRED)
    target_link_libraries(myExecutable PRIVATE h5pp::h5pp h5pp::deps h5pp::flags)

```
**Targets**

-  `h5pp::h5pp` includes the `h5pp` headers.
-  `h5pp::deps` includes dependencies and link corresponding libraries. This target is an alias for the set of libraries that were found/downloaded during the install
    of `h5pp`. These are `Eigen3::Eigen`, `spdlog::spdlog` and `hdf5::hdf5`, which can of course be used independently.
-  `h5pp::flags` sets compile flags that you need to compile with `h5pp`. These flags enable C++17 and filesystem headers, i.e. `-std=c++17 -lstdc++fs`.


#### By copying the headers manually (not as easy)
Copy the headers in the folder `h5pp/source/include/h5pp` somewhere, and link your project to the dependencies `hdf5`, `Eigen3` and `spdlog` in the way you prefer. 
For instance you can use CMake's `find_package(...)` mechanism to find relevant paths.
A minimal `CMakeLists.txt` could be:

```cmake
    cmake_minimum_required(VERSION 3.10)
    project(myProject)
    
    add_executable(myExecutable main.cpp)
    target_include_directories(myExecutable PRIVATE <path-to-h5pp-headers>)
    # Setup h5pp
    target_compile_options(myExecutable PRIVATE -lstdc++fs -std=c++17 )
    target_link_libraries(myExecutable PRIVATE  -lstdc++fs)
    
    # Possibly use find_package() here

    # Link dependencies (this is the tricky part)
    target_include_directories(myExecutable PRIVATE <path-to-Eigen3-include-dir>) 
    target_include_directories(myExecutable PRIVATE <path-to-spdlog-include-dir>) 
    target_include_directories(myExecutable PRIVATE <path-to-hdf5-include-dir>) 
    # Link dependencies (this is the trickiest part). Note that you only need the C libs.
    target_link_libraries(myExecutable PRIVATE  <path-to-libhdf5_hl> <path-to-libhdf5> -ldl -lm -lz -lpthread) # Possibly more -l libs

```

The trickiest part is linking to HDF5 libraries. 
When installing `h5pp` this is handled with a helper function defined in `cmake-modules/FindPackageHDF5.cmake` which finds HDF5 installed
somewhere on your system (e.g. installed via `conda`,`apt`, `Easybuild`,etc) and defines a CMake target `hdf5::hdf5` with everything you need to link correctly.
You can use it too! If you copy `cmake-modules/FindPackageHDF5.cmake` to your project, find HDF5 by simply including it:

```cmake
include(FindPackageHDF5.cmake)

if(HDF5_FOUND AND TARGET hdf5::hdf5)
        target_link_libraries(myExecutable PRIVATE hdf5::hdf5)
endif()

```

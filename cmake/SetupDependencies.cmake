

# Fix issue with linking -pthreads or -lpthreads
if(NOT THREADS_PREFER_PTHREAD_FLAG)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads)


# h5pp requires the filesystem header (and possibly stdc++fs library)
find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
target_link_libraries(deps INTERFACE std::filesystem)


# Detect MPI if requested (we expect the HDF5 library to also be built with support for MPI)
if (H5PP_ENABLE_MPI AND NOT WIN32)
    find_package(MPI COMPONENTS C CXX REQUIRED)
    target_link_libraries(deps INTERFACE MPI::MPI_CXX MPI::MPI_C)
endif ()

# Detect quadmath/__float128 if requested
if(H5PP_USE_QUADMATH)
    find_package(quadmath REQUIRED)
    target_link_libraries(flags INTERFACE quadmath)
    target_compile_definitions(flags INTERFACE H5PP_USE_QUADMATH)
    target_compile_definitions(flags INTERFACE H5PP_USE_FLOAT128)
elseif(H5PP_USE_FLOAT128)
    include(CheckTypeSize)
    check_type_size(__float128 H5PP_FLOAT128_EXISTS BUILTIN_TYPES_ONLY LANGUAGE CXX)
    mark_as_advanced(H5PP_FLOAT128_EXISTS)
    if(H5PP_FLOAT128_EXISTS)
        target_compile_definitions(flags INTERFACE H5PP_USE_FLOAT128)
    else()
        message(FATAL_ERROR "CMake option H5PP_USE_FLOAT128 is ON, but type __float128 could not be detected")
    endif()
endif()



# Start finding the dependencies
if(H5PP_ENABLE_EIGEN3)
    #find_package(Eigen3 REQUIRED)
    include(eigen)
    if(EIGEN3_VERSION AND EIGEN3_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "H5PP_ENABLE_EIGEN3: Found version ${EIGEN3_VERSION} < 3.3 required by h5pp")
    endif()
    target_link_libraries(deps INTERFACE Eigen3::Eigen)
    target_compile_definitions(deps INTERFACE H5PP_USE_EIGEN3)
endif()
if(H5PP_ENABLE_FMT)
    include(fmt)
    # find_package(fmt REQUIRED)
    if(FMT_VERSION AND FMT_VERSION VERSION_LESS 6.1.2)
        message(FATAL_ERROR "H5PP_ENABLE_FMT: Found version ${FMT_VERSION} < 6.1.2 required by h5pp")
    endif()
    target_link_libraries(deps INTERFACE fmt::fmt)
    target_compile_definitions(deps INTERFACE H5PP_USE_FMT)
endif()
if(H5PP_ENABLE_SPDLOG)
    include(spdlog)
    #find_package(spdlog REQUIRED)
    if(SPDLOG_VERSION AND SPDLOG_VERSION VERSION_LESS 1.5.0)
        message(FATAL_ERROR "H5PP_ENABLE_SPDLOG: Found version ${SPDLOG_VERSION} < 1.5.0 required by h5pp")
    endif()
    target_link_libraries(deps INTERFACE spdlog::spdlog)
    target_compile_definitions(deps INTERFACE H5PP_USE_SPDLOG)
endif()

# Note that the call below defaults to FindHDF5.cmake bundled with h5pp,
# because cmake/modules has been added to CMAKE_MODULE_PATH in cmake/SetupPaths.cmake
# Also, we don't impose any version requirement here: h5pp is compatible with 1.8 to 1.14.
find_package(ZLIB QUIET)
find_package(SZIP QUIET)
#find_package(HDF5 COMPONENTS C HL REQUIRED)
# if(HDF5_VERSION AND HDF5_VERSION VERSION_LESS 1.8)
#     message(FATAL_ERROR "Found version ${HDF5_VERSION} < 1.8 required by h5pp")
# endif()
include(hdf5)
#include(cmake/HDF5TargetUtils.cmake)
#h5pp_get_modern_hdf5_target_name()
#target_link_libraries(deps INTERFACE HDF5::HDF5)

#include(h5pp_target_link_system_libraries)
# h5pp_target_link_system_libraries(deps INTERFACE hdf5::hdf5)
target_link_libraries(deps INTERFACE hdf5::hdf5)

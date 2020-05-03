if(H5PP_DOWNLOAD_METHOD MATCHES "find")
    if(NOT H5PP_DOWNLOAD_METHOD MATCHES "fetch")
        set(REQUIRED ${REQUIRED})
    endif()
    # Start finding the dependencies
    if(H5PP_ENABLE_EIGEN3 AND NOT TARGET Eigen3::Eigen )
        find_package(Eigen3 3.3.4 ${REQUIRED})
        if(TARGET Eigen3::Eigen)
            list(APPEND H5PP_TARGETS Eigen3::Eigen)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        endif()
    endif()

    if(H5PP_ENABLE_SPDLOG AND NOT TARGET spdlog::spdlog)
        find_package(spdlog 1.3.1 ${REQUIRED})
        if(TARGET spdlog)
            add_library(spdlog::spdlog ALIAS spdlog)
        endif()
        if(TARGET spdlog::spdlog)
            list(APPEND H5PP_TARGETS spdlog::spdlog)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        endif()
    endif()

    if (NOT TARGET hdf5::hdf5)
        find_package(HDF5 1.8 COMPONENTS C HL ${REQUIRED})
        if(TARGET hdf5::hdf5)
            list(APPEND H5PP_TARGETS hdf5::hdf5)
            target_link_libraries(deps INTERFACE hdf5::hdf5)
        endif()
    endif()
endif()
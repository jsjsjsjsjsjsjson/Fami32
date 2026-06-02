# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(COMPILE_OPTIONS -Wno-error APPEND)
project(Fami32)

set(PROJECT_COMPONENTS_DIR "${CMAKE_SOURCE_DIR}/components")
set(PROJECT_MAIN_DIR "${CMAKE_SOURCE_DIR}/main")

idf_build_get_property(component_targets __BUILD_COMPONENT_TARGETS)
foreach(component_target ${component_targets})
    get_property(component_dir TARGET ${component_target} PROPERTY COMPONENT_DIR)
    get_property(component_lib TARGET ${component_target} PROPERTY COMPONENT_LIB)

    if(NOT TARGET ${component_lib})
        continue()
    endif()

    string(FIND "${component_dir}" "${PROJECT_COMPONENTS_DIR}/" components_pos)
    if(component_dir STREQUAL PROJECT_MAIN_DIR OR components_pos EQUAL 0)
        get_target_property(component_type ${component_lib} TYPE)
        if(component_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${component_lib} INTERFACE -Ofast)
        else()
            target_compile_options(${component_lib} PRIVATE -Ofast)
        endif()
    endif()
endforeach()

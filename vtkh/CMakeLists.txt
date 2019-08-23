cmake_minimum_required(VERSION 3.0)

project(gray-scott-vtkh)

include("FindVTKh.cmake")
# setup vtkh include paths
include_directories(${VTKH_INCLUDE_DIRS})

find_package(ADIOS2 REQUIRED)
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)


#find_library(PAPI papi)
#include_directories(${PAPI_DIR}/include)


# create our example 
add_executable(gray-scott-vtkh vtkh-render.cxx)

# link to vtkh
target_link_libraries(gray-scott-vtkh vtkh_mpi adios2::adios2)

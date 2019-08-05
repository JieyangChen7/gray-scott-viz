cmake_minimum_required(VERSION 3.0)

project(decompress_tool)


find_package(ADIOS2 REQUIRED)
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)


# create our example 
add_executable(decompress_adios decompress_adios.cxx)

# link to vtkh
target_link_libraries(decompress_adios adios2::adios2)

cmake_minimum_required(VERSION 3.0)

project(decompress_tool)

set(ADIOS2_DIR /home/4yc/opt/adios/lib/cmake/adios2)
find_package(ADIOS2 REQUIRED)
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)


LINK_DIRECTORIES(/home/4yc/opt/zc/lib)
INCLUDE_DIRECTORIES(/home/4yc/opt/zc/include)

# create our example 
#add_executable(decompress_adios decompress_adios.cxx)
add_executable(profile_nocompression_adios profile_nocompression_adios.cxx)
add_executable(profile_compress_adios profile_compress_adios.cxx)
add_executable(profile_decompress_adios profile_decompress_adios.cxx)
add_executable(profile_zc profile_zc.cxx)

# link to vtkh
#target_link_libraries(decompress_adios adios2::adios2)
target_link_libraries(profile_nocompression_adios adios2::adios2)
target_link_libraries(profile_compress_adios adios2::adios2)
target_link_libraries(profile_decompress_adios adios2::adios2)
target_link_libraries(profile_zc adios2::adios2 zc)

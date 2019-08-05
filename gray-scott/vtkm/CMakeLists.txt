cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(VTKmDemo CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

find_package(ADIOS2 REQUIRED)
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)


set(srcs vtkm-render.cxx)

if(TARGET vtkm_rendering)
  set(backend "VTKM_DEVICE_ADAPTER_SERIAL")
  if(TARGET vtkm::cuda)
    vtkm_compile_as_cuda(cuda_srcs ${srcs})
    set(srcs ${cuda_srcs})
    set(backend "VTKM_DEVICE_ADAPTER_CUDA")
  elseif(TARGET vtkm::tbb)
    set(backend "VTKM_DEVICE_ADAPTER_TBB")
  endif()

  add_executable(gray-scott-vtkm ${srcs})
  target_link_libraries(gray-scott-vtkm PRIVATE vtkm_rendering adios2::adios2)
  target_compile_definitions(gray-scott-vtkm PRIVATE "VTKM_DEVICE_ADAPTER=${backend}")
endif()


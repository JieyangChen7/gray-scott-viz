#include <vtkh/vtkh.hpp>
#include <vtkh/DataSet.hpp>
#include <vtkh/filters/MarchingCubes.hpp>
#include <vtkh/rendering/RayTracer.hpp>
#include <vtkh/rendering/Scene.hpp>


#include <adios2.h>

#include <vtkm/Types.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <iostream>
#include <iomanip>
#include <mpi.h>
#include <string>

#include <stdio.h>
#include <vector>

//#include "papi.h"
#include <chrono>

int main(int argc, char *argv[]) {

  MPI_Init(NULL, NULL);
  int comm_size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  unsigned int n, b, nb, d, start_iter, end_iter;
  std::chrono::high_resolution_clock::time_point s, e;
  int retval;
  int existU, existV;
  int gap;
  double iso_value;

  double initialization_time = .0f;
  double step_setup_time = .0f;
  double load_decompress_time = .0f;
  double build_dataset_time = .0f;
  double mc_setup_time = .0f;
  double mc_time = .0f;
  double rendering_setup_time = .0f;
  double rendering_time = .0f;

  double max_initialization_time = .0f;
  double max_step_setup_time = .0f;
  double max_load_decompress_time = .0f; 
  double max_build_dataset_time = .0f;
  double max_mc_setup_time = .0f;
  double max_mc_time = .0f;
  double max_rendering_setup_time = .0f;
  double max_rendering_time = .0f;


  if (rank == 0 && argc < 5) {
    std::cout << "No enough arguments provided!" << std::endl;
    std::cout << "Usage: <executable> <bp file> <xml file> <simulation size> <data partition block size>" << std::endl;
    MPI_Finalize();
  }

  std::string pb_filename = argv[1];
  std::string xml_filename = argv[2];
  n = atoi(argv[3]);
  b = atoi(argv[4]);
  d = atoi(argv[5]);
  char* output_file = argv[6];
  start_iter = atoi(argv[7]);
  end_iter   = atoi(argv[8]);
  existU  = atoi(argv[9]);
  existV  = atoi(argv[10]);
  gap = atoi(argv[11]);
  iso_value = atof(argv[12]);

  //std::cout << "test" << std::endl;


  if (rank == 0 && n % b != 0) {
    std::cout << "<data partition block size> should be able to divide <simulation size>" << std::endl;
    return 0;
  }

  //if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT )
  //{
  //    printf("PAPI initialization error! \n");
  //    MPI_Finalize();
  //}

  if (d == 1) {
    vtkh::ForceSerial();
  } else if (d == 2) {
    vtkh::ForceCUDA();
    vtkh::SelectCUDADevice(0);
  } else if (d == 3) {
    vtkh::ForceOpenMP();
  }
  //std::cout << "IsSerialEnabled(): " << vtkh::IsSerialEnabled() << std::endl;
  //std::cout << "IsOpenMPEnabled(): " << vtkh::IsOpenMPEnabled() << std::endl;
  //std::cout << "IsCUDAEnabled(): " << vtkh::IsCUDAEnabled() << std::endl;
  


  //MPI_Barrier(MPI_COMM_WORLD);
  s = std::chrono::high_resolution_clock::now();

  vtkh::SetMPICommHandle(MPI_Comm_c2f(MPI_COMM_WORLD));
  // Data partition
  std::vector<adios2::Box<adios2::Dims>> selections;
  std::vector<int> domain_ids;
  nb = n / b;

  for (unsigned int i = rank*nb*nb*nb/comm_size; i < (rank+1)*nb*nb*nb/comm_size; i++){
    unsigned int x = (i%nb)*b;
    unsigned int y = ((i/nb)%nb)*b;
    unsigned int z = (((i/nb)/nb)%nb)*b;
    adios2::Box<adios2::Dims> sel({x, y, z}, {b, b, b});
    selections.push_back(sel);
    domain_ids.push_back(i);
    //std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 
  }

  adios2::ADIOS adios(xml_filename, MPI_COMM_WORLD, adios2::DebugON);
  //const std::string input_fname = "gs.bp";
  adios2::IO inIO = adios.DeclareIO("DecompressedSimulationOutput");
  //inIO.SetEngine("SST");

  adios2::Engine reader = inIO.Open(pb_filename, adios2::Mode::Read);

  //MPI_Barrier(MPI_COMM_WORLD);
  e = std::chrono::high_resolution_clock::now();
  initialization_time = std::chrono::duration<double>(e - s).count();
 
  int iter = 0; 
  while (iter*gap < end_iter) {
      //std::cout << iter*gap << std::endl; 
      s = std::chrono::high_resolution_clock::now();

      adios2::StepStatus status = reader.BeginStep(); 
      //status = reader.BeginStep(); 
      if (status != adios2::StepStatus::OK) {
          std::cout << "Step error" << std::endl;
          break;
	  //reader.Close();
  	  //MPI_Finalize();
  	  //return 0;

      }
      const adios2::Variable<int> varStep = inIO.InquireVariable<int>("step");
      std::vector<int> step;
      reader.Get(varStep, step, adios2::Mode::Sync);
 
      //std::cout << "Step =" << step[0] << std::endl;
     
      
      if (iter*gap < start_iter) {
          // std::cout << "continue" << std::endl;
          reader.EndStep();
      	  iter++;
	  continue;
      }
      
      adios2::Variable<double> varU;
      adios2::Variable<double> varV;

      if (existU > -1) {
        varU = inIO.InquireVariable<double>("U");
      }
      if (existV > -1) {
        varV = inIO.InquireVariable<double>("V");
      }
      //std::cout <<"Process "<<rank <<" is working on step: " << step[0] << std::endl;
      e = std::chrono::high_resolution_clock::now();
      step_setup_time = std::chrono::duration<double>(e - s).count();

      //adios2::Box<adios2::Dims> sel({x * 24, y * 24, z * 24}, {24, 24, 24});
      //varU.SetSelection(sel);
      adios2::Dims shapeU;
      adios2::Dims shapeV;
      
      if (existU > -1) {
        shapeU = varU.Shape();
      }

      if (existV > -1) {
        shapeV = varV.Shape();
      }
      //std::cout << "Shape: " << shape[0] << ", " << shape[1] << ", " << shape[2] << std::endl;

      //std::vector<double> pointvar_u;
      //reader.Get(varU, pointvar_u, adios2::Mode::Sync);
      //std::cout << "Size of pointvar_u: " << pointvar_u.size() << std::endl;

      //varV.SetSelection(sel);
      //std::vector<double> pointvar_v;
      //reader.Get(varV, pointvar_v, adios2::Mode::Sync);

      s = std::chrono::high_resolution_clock::now();
      std::vector<std::vector<double>*> u_pointers;
      std::vector<std::vector<double>*> v_pointers;
      
      
      for (int idx =0; idx < domain_ids.size(); idx++) {
        if (existU > -1) {
          varU.SetSelection(selections[idx]);
          std::vector<double> * u = new std::vector<double>();
          reader.Get(varU, *u, adios2::Mode::Sync);
          u_pointers.push_back(u);
        }

        if (existV > -1) {
          varV.SetSelection(selections[idx]);
          std::vector<double> * v = new std::vector<double>();
          reader.Get(varV, *v, adios2::Mode::Sync);
          v_pointers.push_back(v);
        }
      }
      e = std::chrono::high_resolution_clock::now();
      load_decompress_time = std::chrono::duration<double>(e - s).count();


      s = std::chrono::high_resolution_clock::now();
      vtkh::DataSet data_set;
      std::string dfname_u = "pointvar_u";
      std::string dfname_v = "pointvar_v";
      
      for (int idx =0; idx < domain_ids.size(); idx++) {
        vtkm::Id3 dims(selections[idx].second[2], selections[idx].second[1], selections[idx].second[0]);
        vtkm::Id3 org(selections[idx].first[2], selections[idx].first[1], selections[idx].first[0]);
        vtkm::Id3 spc(1, 1, 1);
        vtkm::cont::DataSetBuilderUniform dataSetBuilder;
        vtkm::cont::DataSet dataSet;
        dataSet = dataSetBuilder.Create(dims, org, spc);

        vtkm::cont::DataSetFieldAdd dsf;
        if (existU > -1) {
          dsf.AddPointField(dataSet, dfname_u, *(u_pointers[idx]));
        }
        if (existV > -1) {
          dsf.AddPointField(dataSet, dfname_v, *(v_pointers[idx]));
        }
        data_set.AddDomain(dataSet, domain_ids[idx]);
      }
      e = std::chrono::high_resolution_clock::now();
      build_dataset_time = std::chrono::duration<double>(e - s).count();
      // vtkm::Id3 dims(48, 48, 48);
      // vtkm::Id3 org(0, 0, 0);
      // vtkm::Id3 spc(1, 1, 1);
      // vtkm::cont::DataSetBuilderUniform dataSetBuilder;
      // vtkm::cont::DataSet dataSet;
      // dataSet = dataSetBuilder.Create(dims, org, spc);
      // data_set.AddDomain(dataSet, rank);


      // vtkm::Id3 dims(24, 24, 24);
      // vtkm::Id3 org(x * 24, y * 24, z * 24);
      // vtkm::Id3 spc(1, 1, 1);

      // vtkm::cont::DataSetBuilderUniform dataSetBuilder;
      // vtkm::cont::DataSet dataSet;
      // dataSet = dataSetBuilder.Create(dims, org, spc);

      // std::string dfname_u = "pointvar_u";
      // std::string dfname_v = "pointvar_v";
      // vtkm::cont::DataSetFieldAdd dsf;
      // dsf.AddPointField(dataSet, dfname_u, pointvar_u);
      // dsf.AddPointField(dataSet, dfname_v, pointvar_v);


      // vtkh::DataSet data_set;
      // data_set.AddDomain(dataSet, rank);

      s = std::chrono::high_resolution_clock::now();
      vtkh::MarchingCubes marcher;
      marcher.SetInput(&data_set);

      marcher.SetField(dfname_v); 

      const int num_vals = 1;
      double iso_vals [num_vals];
      iso_vals[0] = iso_value;

      marcher.SetIsoValues(iso_vals, num_vals);
      //marcher.AddMapField(dfname_u);
      marcher.AddMapField(dfname_v);
      e = std::chrono::high_resolution_clock::now();
      mc_setup_time = std::chrono::duration<double>(e - s).count();

      s = std::chrono::high_resolution_clock::now();
      marcher.Update();
      e = std::chrono::high_resolution_clock::now();
      mc_time = std::chrono::duration<double>(e - s).count();


      s = std::chrono::high_resolution_clock::now();
      vtkh::DataSet *iso_output = marcher.GetOutput();
      // vtkm::Bounds bounds = iso_output->GetGlobalBounds();
      // std::cout << "Bound: X(" << bounds.X.Min << " - " <<  bounds.X.Max << ")" <<std::endl;
      // std::cout << "Bound: Y(" << bounds.Y.Min << " - " <<  bounds.Y.Max << ")" <<std::endl;
      // std::cout << "Bound: Z(" << bounds.Z.Min << " - " <<  bounds.Z.Max << ")" <<std::endl;
      vtkm::Bounds bounds(vtkm::Range(0, shapeV[2]-1), vtkm::Range(0, shapeV[1]-1), vtkm::Range(0, shapeV[0]-1));
      vtkm::rendering::Camera camera;
      camera.ResetToBounds(bounds);


      // vtkm::Vec<vtkm::Float32, 3> totalExtent;
      // totalExtent[0] = vtkm::Float32(48);
      // totalExtent[1] = vtkm::Float32(48);
      // totalExtent[2] = vtkm::Float32(48);
      // vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
      // vtkm::Normalize(totalExtent);
      // camera.SetLookAt(totalExtent * (mag * .5f));
      // camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
      // camera.SetClippingRange(1.f, 100.f);
      // camera.SetFieldOfView(60.f);
      //camera.SetPosition(totalExtent * (mag * 1.f));

      float bg_color[4] = { 0.f, 0.f, 0.f, 1.f};
      vtkh::Render render = vtkh::MakeRender(1024, 
                                             1024, 
                                             camera, 
                                             *iso_output, 
                                             std::string(output_file) +"_"+ std::to_string(step[0]),
                                             bg_color); 
      
      vtkh::Scene scene;
      scene.AddRender(render);

      vtkh::RayTracer tracer;
      tracer.SetInput(iso_output);
      tracer.SetField(dfname_v); 

      vtkm::cont::ColorTable color_map("Rainbow Uniform");
      tracer.SetColorTable(color_map);
      tracer.SetRange(vtkm::Range(0, 0.5));
      scene.AddRenderer(&tracer);  
      e = std::chrono::high_resolution_clock::now();
      rendering_setup_time = std::chrono::duration<double>(e - s).count();

      s = std::chrono::high_resolution_clock::now();
      scene.Render();
      e = std::chrono::high_resolution_clock::now();
      rendering_time = std::chrono::duration<double>(e - s).count();

      //vtkm::Range r = color_map.GetRange();
      //std::cout << "Range is: " << r.Min << " - " << r.Max << std::endl;

    
      for (int idx =0; idx < domain_ids.size(); idx++) {
        if (existU > -1) {
          delete u_pointers[idx];
        }
        if (existV > -1) {
          delete v_pointers[idx];
        }
      }

      MPI_Reduce(&initialization_time, &max_initialization_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&step_setup_time, &max_step_setup_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&load_decompress_time, &max_load_decompress_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&build_dataset_time, &max_build_dataset_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&mc_setup_time, &max_mc_setup_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&mc_time, &max_mc_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&rendering_setup_time, &max_rendering_setup_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      MPI_Reduce(&rendering_time, &max_rendering_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if (rank == 0) {
        /*std::cout << std::setw(25) << std::left << "initialization_time: " << max_initialization_time  <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"step_setup_time: "      << max_step_setup_time      <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"load_decompress_time: " << max_load_decompress_time <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"build_dataset_time: "   << max_build_dataset_time   <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"mc_setup_time: "        << max_mc_setup_time        <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"mc_time: "              << max_mc_time              <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"rendering_setup_time: " << max_rendering_setup_time <<" s" << std::endl;
        std::cout << std::setw(25) << std::left <<"rendering_time: "       << max_rendering_time       <<" s" << std::endl;
        */

      	std::cout << step[0] << "," << max_load_decompress_time << "," << max_mc_time << "," << max_rendering_time << std::endl;
      }
     
      reader.EndStep();
      iter++;
  }
  reader.Close();
  MPI_Finalize();
  return 0;
}

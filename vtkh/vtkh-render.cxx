#include <vtkh/vtkh.hpp>
#include <vtkh/DataSet.hpp>
#include <vtkh/filters/MarchingCubes.hpp>
#include <vtkh/rendering/RayTracer.hpp>
#include <vtkh/rendering/Scene.hpp>

#include <adios2.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <iostream>
#include <mpi.h>
#include <string>

#include <stdio.h>
#include <vector>

#include "papi.h"


int main(int argc, char *argv[]) {

  MPI_Init(NULL, NULL);
  int comm_size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  vtkh::SetMPICommHandle(MPI_Comm_c2f(MPI_COMM_WORLD));

  int n, b, nb;
  long long s, e;
  int retval;
  float load_decompress_time = .0f, mc_time = .0f, rendering_time = .0f;


  if (rank == 0 && argc < 5) {
    std::cout << "No enough arguments provided!" << std::endl;
    std::cout << "Usage: <executable> <bp file> <xml file> <simulation size> <data partition block size>" << std::endl;
    MPI_Finalize();
  }

  std::string pb_filename = argv[1];
  std::string xml_filename = argv[2];
  n = atoi(argv[3]);
  b = atoi(argv[4]);

  if (rank == 0 && n % b != 0) {
    std::cout << "<data partition block size> should be able to divide <simulation size>" << std::endl;
    return 0;
  }



  if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT )
  {
      printf("PAPI initialization error! \n");
      MPI_Finalize();
  }


  // Data partition
  std::vector<adios2::Box<adios2::Dims>> selections;
  std::vector<int> domain_ids;
  nb = n / b;


  for (int i = rank*nb*nb*nb/comm_size; i < (rank+1)*nb*nb*nb/comm_size; i++){
    int x = (i%nb)*b;
    int y = ((i/nb)%nb)*b;
    int z = (((i/nb)/nb)%nb)*b;
    adios2::Box<adios2::Dims> sel({x, y, z}, {b, b, b});
    selections.push_back(sel);
    domain_ids.push_back(i);
    std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 
  }



  // int x = (n/comm_size) * rank;
  // int y = 0;
  // int z = 0;
  // adios2::Box<adios2::Dims> sel({x, y, z}, {n/comm_size, n, n});
  // selections.push_back(sel);
  // domain_ids.push_back(rank);
  // std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 


  // int x = 0;
  // int y = 0;
  // int z = 0;
  // adios2::Box<adios2::Dims> sel({x, y, z}, {b, b, b});
  // selections.push_back(sel);
  // domain_ids.push_back(rank);
  // std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 


  // x = 24;
  // y = 0;
  // z = 0;
  // adios2::Box<adios2::Dims> sel2({x, y, z}, {b, b, b});
  // selections.push_back(sel2);
  // domain_ids.push_back(rank);
  // std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 

  // x = 24;
  // y = 24;
  // z = 24;
  // adios2::Box<adios2::Dims> sel3({x, y, z}, {b, b, b});
  // selections.push_back(sel3);
  // domain_ids.push_back(rank);
  // std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 

  // x = 24;
  // y = 24;
  // z = 0;
  // adios2::Box<adios2::Dims> sel4({x, y, z}, {b, b, b});
  // selections.push_back(sel4);
  // domain_ids.push_back(rank);
  // std::cout << "p" << rank << " get: (" << x << ", " << y << ", " << z << ")" << std::endl; 


  s = PAPI_get_real_usec();


  //int x = rank >> 0 & 1;
  //int y = rank >> 1 & 1;
  //int z = rank >> 2 & 1;

  //std::cout << "process " << rank << " has index (" << x << ", " << y << ", " << z << ")" << std::endl;

  adios2::ADIOS adios(xml_filename, MPI_COMM_WORLD, adios2::DebugON);
  //const std::string input_fname = "gs.bp";
  adios2::IO inIO = adios.DeclareIO("SimulationOutput");
  adios2::Engine reader = inIO.Open(pb_filename, adios2::Mode::Read);
  
  int total_iter = 12;
  int iter = 0;
  while (iter < total_iter) {

      adios2::StepStatus status = reader.BeginStep(); 
      if (status != adios2::StepStatus::OK) {
          std::cout << "Step error!";
      }


      adios2::Variable<double> varU = inIO.InquireVariable<double>("U");
      adios2::Variable<double> varV = inIO.InquireVariable<double>("V");
      const adios2::Variable<int> varStep = inIO.InquireVariable<int>("step");


      std::vector<int> step;
      reader.Get(varStep, step, adios2::Mode::Sync);
      std::cout << "Step: " << step[0] << std::endl;

      //adios2::Box<adios2::Dims> sel({x * 24, y * 24, z * 24}, {24, 24, 24});
      //varU.SetSelection(sel);

      //adios2::Dims shape = varU.Shape();
      //std::cout << "Shape: " << shape[0] << ", " << shape[1] << ", " << shape[2] << std::endl;

      //std::vector<double> pointvar_u;
      //reader.Get(varU, pointvar_u, adios2::Mode::Sync);
      //std::cout << "Size of pointvar_u: " << pointvar_u.size() << std::endl;

      //varV.SetSelection(sel);
      //std::vector<double> pointvar_v;
      //reader.Get(varV, pointvar_v, adios2::Mode::Sync);

      std::vector<std::vector<double>*> u_pointers;
      std::vector<std::vector<double>*> v_pointers;
      
      s = PAPI_get_real_usec();
      for (int idx =0; idx < domain_ids.size(); idx++) {
        varU.SetSelection(selections[idx]);
        std::vector<double> * u = new std::vector<double>();
        reader.Get(varU, *u, adios2::Mode::Sync);
        u_pointers.push_back(u);

        varV.SetSelection(selections[idx]);
        std::vector<double> * v = new std::vector<double>();
        reader.Get(varV, *v, adios2::Mode::Sync);
        v_pointers.push_back(v);
      }
      s = PAPI_get_real_usec() - s;
      load_decompress_time += s;


      vtkh::DataSet data_set;
      std::string dfname_u = "pointvar_u";
      std::string dfname_v = "pointvar_v";
      
      for (int idx =0; idx < domain_ids.size(); idx++) {
        vtkm::Id3 dims(selections[idx].second[0], selections[idx].second[1], selections[idx].second[2]);
        vtkm::Id3 org(selections[idx].first[2], selections[idx].first[1], selections[idx].first[0]);
        std::cout << "pp" << rank << " get: (" << selections[idx].first[0] << ", " << selections[idx].first[1] << ", " << selections[idx].first[2] << ")" << std::endl; 
        vtkm::Id3 spc(1, 1, 1);
        vtkm::cont::DataSetBuilderUniform dataSetBuilder;
        vtkm::cont::DataSet dataSet;
        dataSet = dataSetBuilder.Create(dims, org, spc);

        vtkm::cont::DataSetFieldAdd dsf;
        dsf.AddPointField(dataSet, dfname_u, *(u_pointers[idx]));
        dsf.AddPointField(dataSet, dfname_v, *(v_pointers[idx]));
        data_set.AddDomain(dataSet, domain_ids[idx]);
      }
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

      s = PAPI_get_real_usec();
      vtkh::MarchingCubes marcher;
      marcher.SetInput(&data_set);
      marcher.SetField(dfname_u); 

      const int num_vals = 2;
      double iso_vals [num_vals];
      iso_vals[0] = -1;
      iso_vals[1] = 0.99;

      marcher.SetIsoValues(iso_vals, num_vals);
      marcher.AddMapField(dfname_u);
      marcher.AddMapField(dfname_v);
      marcher.Update();
      vtkh::DataSet *iso_output = marcher.GetOutput();
      s = PAPI_get_real_usec() - s;
      mc_time += s;


      vtkm::Bounds bounds = iso_output->GetGlobalBounds();

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
      // camera.SetPosition(totalExtent * (mag * 2.f));

      float bg_color[4] = { 0.f, 0.f, 0.f, 1.f};
      vtkh::Render render = vtkh::MakeRender(512, 
                                             512, 
                                             camera, 
                                             *iso_output, 
                                             "step_" + std::to_string(step[0]),
                                             bg_color);  

      vtkh::Scene scene;
      scene.AddRender(render);

      vtkh::RayTracer tracer;
      tracer.SetInput(iso_output);
      tracer.SetField(dfname_v); 
      scene.AddRenderer(&tracer);  

      scene.Render();

      // std::vector<double> vars;
      // for (int i =0; i < 10; i++) {
      //     cout << pointvar_u[i] << endl;  
      // }

      






        iter++;
    }
    e = PAPI_get_real_usec();
    std::cout << "Time (s): " << double(e-s)/1e6 << std::endl;
  

  // vtkh::DataSet data_set;
 
  // const int base_size = 32;
  // const int blocks_per_rank = 2;
  // const int num_blocks = comm_size * blocks_per_rank; 
  
  // for(int i = 0; i < blocks_per_rank; ++i)
  // {
  //   int domain_id = rank * blocks_per_rank + i;
  //   data_set.AddDomain(CreateTestData(domain_id, num_blocks, base_size), domain_id);
  // }
  
  // vtkh::MarchingCubes marcher;
  // marcher.SetInput(&data_set);
  // marcher.SetField("point_data"); 

  // const int num_vals = 2;
  // double iso_vals [num_vals];
  // iso_vals[0] = -1; // ask for something that does not exist
  // iso_vals[1] = (float)base_size * (float)num_blocks * 0.5f;

  // marcher.SetIsoValues(iso_vals, num_vals);
  // marcher.AddMapField("point_data");
  // marcher.AddMapField("cell_data");
  // marcher.Update();

  // vtkh::DataSet *iso_output = marcher.GetOutput();
  // vtkm::Bounds bounds = iso_output->GetGlobalBounds();

  // vtkm::rendering::Camera camera;
  // camera.ResetToBounds(bounds);

  // float bg_color[4] = { 0.f, 0.f, 0.f, 1.f};
  // vtkh::Render render = vtkh::MakeRender(512, 
  //                                        512, 
  //                                        camera, 
  //                                        *iso_output, 
  //                                        "iso_par",
  //                                        bg_color);  

  // vtkh::Scene scene;
  // scene.AddRender(render);

  // vtkh::RayTracer tracer;
  // tracer.SetInput(iso_output);
  // tracer.SetField("cell_data"); 
  // scene.AddRenderer(&tracer);  

  // scene.Render();

  // delete iso_output; 
  MPI_Finalize();
}

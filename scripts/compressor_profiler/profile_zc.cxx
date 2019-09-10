#include <adios2.h>
#include <mpi.h>
#include <string>
#include <chrono>
#include <iostream>
#include <cmath>
#include "zc.h"

double L_infinity_error(int d1, int d2, int d3, double * org, double * dec) {
  double norm0 = 0.0;
  double norm  = 0.0;
  for (int i = 0; i < d1*d2*d3; i++) { 
    if (norm0 < fabs(org[i])) norm0 = fabs(org[i]);
    double abs_diff = fabs(org[i] - dec[i]);
    if (norm < abs_diff) norm = abs_diff;
  }
  return norm/norm0;
}


int main(int argc, char *argv[]) {

  MPI_Init(NULL, NULL);
  int comm_size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  std::string org_pb_filename = argv[1];
  std::string dec_pb_filename = argv[2];

  //std::string xml_filename = argv[3];

  int compressU = std::atoi(argv[3]); // -1: have not stored U; 0: have stored U
  int compressV = std::atoi(argv[4]); // -1: have not stored V; 0: have stored V;
  
  // Z-Checker configuration file
  char *cfgFile = argv[5];

  //std::cout << "zc config = " << cfgFile << std::endl;
  //std::cout << "compressU = " << compressU << std::endl;
  //std::cout << "compressV = " << compressV << std::endl;       

  adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
 
  adios2::IO org_inIO = adios.DeclareIO("NocompressedSimulationOutput");
  adios2::Engine org_reader = org_inIO.Open(org_pb_filename, adios2::Mode::Read);

  adios2::IO dec_inIO = adios.DeclareIO("DecompressedSimulationOutput");
  adios2::Engine dec_reader = dec_inIO.Open(dec_pb_filename, adios2::Mode::Read);

  const adios2::Variable<int> org_inVarStep = org_inIO.InquireVariable<int>("step");
  adios2::Variable<double> org_inVarU;
  adios2::Variable<double> org_inVarV;
  adios2::Dims org_shapeU;
  adios2::Dims org_shapeV;

  const adios2::Variable<int> dec_inVarStep = dec_inIO.InquireVariable<int>("step");
  adios2::Variable<double> dec_inVarU;
  adios2::Variable<double> dec_inVarV;
  adios2::Dims dec_shapeU;
  adios2::Dims dec_shapeV;

  if (compressU > -1) {
    //std::cout << "compressU" << std::endl;
    org_inVarU = org_inIO.InquireVariable<double>("U");
    org_shapeU = org_inVarU.Shape();
    dec_inVarU = dec_inIO.InquireVariable<double>("U");
    dec_shapeU = dec_inVarU.Shape();
  }
  if (compressV > -1) {
    //std::cout << "compressV" << std::endl;
    org_inVarV = org_inIO.InquireVariable<double>("V");
    org_shapeV = org_inVarV.Shape();
    dec_inVarV = dec_inIO.InquireVariable<double>("V");
    dec_shapeV = dec_inVarV.Shape();
  }
  

  //ZC_Init(cfgFile);
  //ZC_CompareData* compareResult;
//  ZC_DataProperty * org_dataProperty;
//  ZC_DataProperty * dec_dataProperty;
  int iter = 0;
  while (true) {
    adios2::StepStatus status1 = org_reader.BeginStep(); 
    adios2::StepStatus status2 = dec_reader.BeginStep();
    //status = reader.BeginStep(); 
    if (status1 != adios2::StepStatus::OK || status2 != adios2::StepStatus::OK) {
        break;
    }
    //std::cout << "iter = " << iter << std::endl;

    
    
    std::vector<int> org_inStep;
    org_reader.Get(org_inVarStep, org_inStep, adios2::Mode::Sync);
      

    if (compressU > -1) {
      std::vector<double> org_u;
      std::vector<double> dec_u;
      org_reader.Get(org_inVarU, org_u, adios2::Mode::Sync);
      dec_reader.Get(dec_inVarU, dec_u, adios2::Mode::Sync);
      char varName[] = "U";
      ZC_Init(cfgFile);
      ZC_CompareData* compareResult = ZC_compareData(varName, ZC_DOUBLE, org_u.data(), dec_u.data(),
                1, 1, org_shapeU[0], org_shapeU[1], org_shapeU[2]);
      double l_inf_error = L_infinity_error(org_shapeV[0], org_shapeV[1], org_shapeV[2], org_u.data(), dec_u.data());
      std::cout<< org_inStep[0] <<"," << compareResult->psnr << "," << l_inf_error <<std::endl;
      freeDataProperty(compareResult->property);
      freeCompareResult(compareResult);   
    }
    if (compressV > -1) {
      std::vector<double> org_v;
      std::vector<double> dec_v;
      ZC_Init(cfgFile); 
      org_reader.Get(org_inVarV, org_v, adios2::Mode::Sync);
      dec_reader.Get(dec_inVarV, dec_v, adios2::Mode::Sync);
      char varName[] = "V";
      ZC_CompareData* compareResult = ZC_compareData(varName, ZC_DOUBLE, org_v.data(), dec_v.data(),
                            1, 1, org_shapeV[0], org_shapeV[1], org_shapeV[2]);
      double l_inf_error = L_infinity_error(org_shapeV[0], org_shapeV[1], org_shapeV[2], org_v.data(), dec_v.data());
      std::cout<< org_inStep[0] <<"," << compareResult->psnr << "," << l_inf_error <<std::endl;
      freeDataProperty(compareResult->property);
      freeCompareResult(compareResult);
    }
    iter++;
    org_reader.EndStep();
    dec_reader.EndStep();
  }
  return 0;
}

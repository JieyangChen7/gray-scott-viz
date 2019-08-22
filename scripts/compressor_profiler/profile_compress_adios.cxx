#include <adios2.h>
#include <mpi.h>
#include <string>
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {

  MPI_Init(NULL, NULL);
  int comm_size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  std::string pb_filename = argv[1];
  std::string xml_filename = argv[2];

  std::string output_bp_filename = argv[3];

  int compressU = std::atoi(argv[4]); // -1: do not store U; 0: store U without compressor;
 				      // 1: MGARD; 2: SZ; 3: ZFP;
  float toleranceU = std::atof(argv[5]);
  int compressV = std::atoi(argv[6]); // -1: do not store V; 0: store V without compressor;
                                      // 1: MGARD; 2: SZ; 3: ZFP;
  float toleranceV = std::atof(argv[7]);

  adios2::ADIOS adios(xml_filename, MPI_COMM_WORLD, adios2::DebugON);
  //const std::string input_fname = "gs.bp";
  adios2::IO inIO = adios.DeclareIO("SimulationOutput");
  adios2::Engine reader = inIO.Open(pb_filename, adios2::Mode::Read);




  adios2::Variable<double> inVarU, inVarV;
  const adios2::Variable<int> inVarStep = inIO.InquireVariable<int>("step");
  adios2::Dims shapeU, shapeV;

  if (compressU > -1) {
    inVarU = inIO.InquireVariable<double>("U");
    shapeU = inVarU.Shape();
  }

  if (compressV > -1) {
    inVarV = inIO.InquireVariable<double>("V");
    shapeV = inVarV.Shape();
  }

  adios2::IO outIO = adios.DeclareIO("CompressedSimulationOutput");
  
  /*std::string output_bp = "compressed_" + 
                          std::to_string(compressU) + "_" +
                          std::to_string(toleranceU) + "_" + 
                          std::to_string(compressV) + "_" + 
                          std::to_string(toleranceV) + "_" + 
                          pb_filename;
  */
  adios2::Engine writer = outIO.Open(output_bp_filename, adios2::Mode::Write);


  adios2::Variable<double> varU;
  adios2::Variable<double> varV;
  adios2::Variable<int> step = outIO.DefineVariable<int>("step");


  if (compressU > -1) {
    varU = outIO.DefineVariable<double>("U", {shapeU[0], shapeU[1], shapeU[2]},
                                    {0,0,0},
                                    {shapeU[0], shapeU[1], shapeU[2]});
  }

  if (compressV > -1) {
    varV = outIO.DefineVariable<double>("V", {shapeV[0], shapeV[1], shapeV[2]},
                                    {0,0,0},
                                    {shapeV[0], shapeV[1], shapeV[2]});
  }

  if (compressU == 1) { // MGARD
    adios2::Operator szOp =
            adios.DefineOperator("mgardCompressor_u", adios2::ops::LossyMGARD);
            varU.AddOperation(szOp, {{adios2::ops::mgard::key::tolerance, std::to_string(toleranceU)}});
  } else if (compressU == 2) { // SZ
    adios2::Operator szOp =
            adios.DefineOperator("szCompressor_u", adios2::ops::LossySZ);
            varU.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, std::to_string(toleranceU)}});
  } else if (compressU == 3) { // ZFP
    adios2::Operator szOp =
            adios.DefineOperator("ZFPCompressor_u", adios2::ops::LossyZFP);
            varU.AddOperation(szOp, {{adios2::ops::zfp::key::accuracy, std::to_string(toleranceU)}});
  }

  if (compressV == 1) { // MGARD
    adios2::Operator szOp =
            adios.DefineOperator("mgardCompressor_v", adios2::ops::LossyMGARD);
            varV.AddOperation(szOp, {{adios2::ops::mgard::key::tolerance, std::to_string(toleranceV)}});
  } else if (compressV == 2) { // SZ
    adios2::Operator szOp =
            adios.DefineOperator("szCompressor_v", adios2::ops::LossySZ);
            varV.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, std::to_string(toleranceV)}});
  } else if (compressV == 3) { // ZFP
    adios2::Operator szOp =
            adios.DefineOperator("ZFPCompressor_v", adios2::ops::LossyZFP);
            varV.AddOperation(szOp, {{adios2::ops::zfp::key::accuracy, std::to_string(toleranceV)}});
  }
  double io_time = 0.0;
  int iter = 0;
  while (true) {
      adios2::StepStatus status = reader.BeginStep(); 
      //status = reader.BeginStep(); 
      if (status != adios2::StepStatus::OK) {
          break;
      }
      //std::cout << "iter = " << iter << std::endl;

      writer.BeginStep();
      
      std::vector<int> inStep;
      reader.Get(inVarStep, inStep, adios2::Mode::Sync);
      writer.Put<int>(step, inStep[0], adios2::Mode::Sync);
      
      std::vector<double> u;
      std::vector<double> v;
      if (compressU > -1) {
        reader.Get(inVarU, u, adios2::Mode::Sync);
	
	std::cout << inStep[0] << ",";
	auto start = std::chrono::high_resolution_clock::now();
        writer.Put<double>(varU, u.data(), adios2::Mode::Sync);
	auto stop = std::chrono::high_resolution_clock::now();
	io_time = std::chrono::duration<double>(stop-start).count();
        std::cout << "," << io_time << std::endl;
      }

      if (compressV > -1) {
	std::cout << inStep[0] << ",";
        reader.Get(inVarV, v, adios2::Mode::Sync);
        auto start = std::chrono::high_resolution_clock::now();
	writer.Put<double>(varV, v.data(), adios2::Mode::Sync);
	auto stop = std::chrono::high_resolution_clock::now();
        io_time = std::chrono::duration<double>(stop-start).count();
        std::cout << "," << io_time << std::endl;
      }
      iter++;
      writer.EndStep();

  }
  //std::cout << io_time << std::endl;
  writer.Close();
  return 0;
}

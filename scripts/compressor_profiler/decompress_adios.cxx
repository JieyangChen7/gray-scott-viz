#include <adios2.h>
#include <mpi.h>
#include <string>

int main(int argc, char *argv[]) {

  MPI_Init(NULL, NULL);
  int comm_size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  std::string pb_filename = argv[1];
  std::string xml_filename = argv[2];


  adios2::ADIOS adios(xml_filename, MPI_COMM_WORLD, adios2::DebugON);
  //const std::string input_fname = "gs.bp";
  adios2::IO inIO = adios.DeclareIO("CompressedSimulationOutput");
  adios2::Engine reader = inIO.Open(pb_filename, adios2::Mode::Read);



  adios2::Variable<double> varU = inIO.InquireVariable<double>("U");
  adios2::Variable<double> varV = inIO.InquireVariable<double>("V");
  const adios2::Variable<int> varStep = inIO.InquireVariable<int>("step");
  adios2::Dims shapeU = varU.Shape();
  adios2::Dims shapeV = varV.Shape();


  adios2::IO outIO = adios.DeclareIO("DecompressedSimulationOutput");
  adios2::Engine writer = outIO.Open("decompressed_" + pb_filename, adios2::Mode::Write);

  adios2::Variable<double> varU2 = outIO.DefineVariable<double>("U", {shapeU[0], shapeU[1], shapeU[2]},
                                  {0,0,0},
                                  {shapeU[0], shapeU[1], shapeU[2]});

  adios2::Variable<double> varV2 = outIO.DefineVariable<double>("V", {shapeV[0], shapeV[1], shapeV[2]},
                                  {0,0,0},
                                  {shapeV[0], shapeV[1], shapeV[2]});

  adios2::Variable<int> step2 = outIO.DefineVariable<int>("step");



  while (true) {
  	  adios2::StepStatus status = reader.BeginStep(); 
      //status = reader.BeginStep(); 
      if (status != adios2::StepStatus::OK) {
          break;
      }
      
      

      std::vector<int> step;
      reader.Get(varStep, step, adios2::Mode::Sync);

      std::vector<double> u;
      reader.Get(varU, u, adios2::Mode::Sync);

      std::vector<double> v;
      reader.Get(varV, v, adios2::Mode::Sync);

      writer.BeginStep();
      writer.Put<int>(step2, step[0]);
      writer.Put<double>(varU2, u.data());
      writer.Put<double>(varV2, v.data());
      writer.EndStep();

  }
writer.Close();
return 0;
}

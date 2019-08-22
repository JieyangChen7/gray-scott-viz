#include <chrono>
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <vector>

#include <adios2.h>

#include "gray-scott.h"

void define_bpvtk_attribute(const Settings &s, adios2::IO& io)
{
    auto lf_VTKImage = [](const Settings &s, adios2::IO& io){

        const std::string extent = "0 " + std::to_string(s.L + 1) + " " +
                                   "0 " + std::to_string(s.L + 1) + " " +
                                   "0 " + std::to_string(s.L + 1);

        const std::string imageData = R"(
        <?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
              <CellData Scalars="U">
                  <DataArray Name="U" />
                  <DataArray Name="V" />
                  <DataArray Name="TIME">
                    step
                  </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

        io.DefineAttribute<std::string>("vtk.xml", imageData);
    };

    if(s.mesh_type == "image")
    {
        lf_VTKImage(s,io);
    }
    else if( s.mesh_type == "structured")
    {
        throw std::invalid_argument("ERROR: mesh_type=structured not yet "
                "   supported in settings.json, use mesh_type=image instead\n");
    }
    // TODO extend to other formats e.g. structured
}


void print_io_settings(const adios2::IO &io)
{
    std::cout << "Simulation writes data using engine type:              "
              << io.EngineType() << std::endl;
}

void print_settings(const Settings &s)
{
    std::cout << "grid:             " << s.L << "x" << s.L << "x" << s.L
              << std::endl;
    std::cout << "steps:            " << s.steps << std::endl;
    std::cout << "plotgap:          " << s.plotgap << std::endl;
    std::cout << "F:                " << s.F << std::endl;
    std::cout << "k:                " << s.k << std::endl;
    std::cout << "dt:               " << s.dt << std::endl;
    std::cout << "Du:               " << s.Du << std::endl;
    std::cout << "Dv:               " << s.Dv << std::endl;
    std::cout << "noise:            " << s.noise << std::endl;
    std::cout << "output:           " << s.output << std::endl;
    std::cout << "adios_config:     " << s.adios_config << std::endl;
}

void print_simulator_settings(const GrayScott &s)
{
    std::cout << "decomposition:    " << s.npx << "x" << s.npy << "x" << s.npz
              << std::endl;
    std::cout << "grid per process: " << s.size_x << "x" << s.size_y << "x"
              << s.size_z << std::endl;
}

std::chrono::milliseconds
diff(const std::chrono::steady_clock::time_point &start,
     const std::chrono::steady_clock::time_point &end)
{
    auto diff = end - start;

    return std::chrono::duration_cast<std::chrono::milliseconds>(diff);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 1;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    if (argc < 2) {
        if (rank == 0) {
            std::cerr << "Too few arguments" << std::endl;
            std::cerr << "Usage: grayscott settings.json" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    Settings settings = Settings::from_json(argv[1]);

    GrayScott sim(settings, comm);

    sim.init();

    adios2::ADIOS adios(settings.adios_config, comm, adios2::DebugON);

    adios2::IO io = adios.DeclareIO("SimulationOutput");

    if (rank == 0) {
        print_io_settings(io);
        std::cout << "========================================" << std::endl;
        print_settings(settings);
        print_simulator_settings(sim);
        std::cout << "========================================" << std::endl;
    }

    io.DefineAttribute<double>("F", settings.F);
    io.DefineAttribute<double>("k", settings.k);
    io.DefineAttribute<double>("dt", settings.dt);
    io.DefineAttribute<double>("Du", settings.Du);
    io.DefineAttribute<double>("Dv", settings.Dv);
    io.DefineAttribute<double>("noise", settings.noise);
    //define VTK visualization schema as an attribute
    if(!settings.mesh_type.empty())
    {
        define_bpvtk_attribute(settings, io);
    }

    adios2::Variable<double> varU =
        io.DefineVariable<double>("U", {settings.L, settings.L, settings.L},
                                  {sim.offset_z, sim.offset_y, sim.offset_x},
                                  {sim.size_z, sim.size_y, sim.size_x});

    adios2::Variable<double> varV =
        io.DefineVariable<double>("V", {settings.L, settings.L, settings.L},
                                  {sim.offset_z, sim.offset_y, sim.offset_x},
                                  {sim.size_z, sim.size_y, sim.size_x});


    /* added for compression test */
   /* int mode = std::atoi(argv[2]); // 1 compress only u; 2 compress only v; 3 compress both;
    int compressor = std::atoi(argv[3]); // 0 no compression; 1 MGARD; 2 SZ; 3 ZFP;
    float tolerance_u = std::atof(argv[4]);
    float tolerance_v = std::atof(argv[5]);

    if (compressor == 1) { // MGARD
        if (mode == 1) { // U
            adios2::Operator szOp =
            adios.DefineOperator("mgardCompressor_u", adios2::ops::LossyMGARD);
            varU.AddOperation(szOp, {{adios2::ops::mgard::key::tolerance, std::to_string(tolerance_u)}});
        } else if (mode == 2) { // V
            adios2::Operator szOp =
            adios.DefineOperator("mgardCompressor_v", adios2::ops::LossyMGARD);
            varV.AddOperation(szOp, {{adios2::ops::mgard::key::tolerance, std::to_string(tolerance_v)}});
        } else if (mode == 3) { // U&V
            adios2::Operator szOpU =
            adios.DefineOperator("mgardCompressor_u", adios2::ops::LossyMGARD);
            varU.AddOperation(szOpU, {{adios2::ops::mgard::key::tolerance, std::to_string(tolerance_u)}});
            adios2::Operator szOpV =
            adios.DefineOperator("mgardCompressor_v", adios2::ops::LossyMGARD);
            varV.AddOperation(szOpV, {{adios2::ops::mgard::key::tolerance, std::to_string(tolerance_v)}});
        }
    } else if (compressor == 2) { //SZ
        if (mode == 1) { // U
            adios2::Operator szOp =
            adios.DefineOperator("szCompressor_u", adios2::ops::LossySZ);
            varU.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, std::to_string(tolerance_u)}});
        } else if (mode == 2) { // V
            adios2::Operator szOp =
            adios.DefineOperator("szCompressor_v", adios2::ops::LossySZ);
            varV.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, std::to_string(tolerance_v)}});
        } else if (mode == 3) { // U&V
            adios2::Operator szOpU =
            adios.DefineOperator("szCompressor_u", adios2::ops::LossySZ);
            varU.AddOperation(szOpU, {{adios2::ops::sz::key::accuracy, std::to_string(tolerance_u)}});
            adios2::Operator szOpV =
            adios.DefineOperator("szCompressor_v", adios2::ops::LossySZ);
            varV.AddOperation(szOpV, {{adios2::ops::sz::key::accuracy, std::to_string(tolerance_v)}});
        }
    } else if (compressor == 3) { //ZFP
        if (mode == 1) { // U
            adios2::Operator szOp =
            adios.DefineOperator("ZFPCompressor_u", adios2::ops::LossyZFP);
            varU.AddOperation(szOp, {{adios2::ops::zfp::key::accuracy, std::to_string(tolerance_u)}});
        } else if (mode == 2) { // V
            adios2::Operator szOp =
            adios.DefineOperator("ZFPCompressor_v", adios2::ops::LossyZFP);
            varV.AddOperation(szOp, {{adios2::ops::zfp::key::accuracy, std::to_string(tolerance_v)}});
        } else if (mode == 3) { // U&V
            adios2::Operator szOpU =
            adios.DefineOperator("ZFPCompressor_u", adios2::ops::LossyZFP);
            varU.AddOperation(szOpU, {{adios2::ops::zfp::key::accuracy, std::to_string(tolerance_u)}});
            adios2::Operator szOpV =
            adios.DefineOperator("ZFPCompressor_v", adios2::ops::LossyZFP);
            varV.AddOperation(szOpV, {{adios2::ops::zfp::key::accuracy, std::to_string(tolerance_v)}});
        }
    }
    */
    

    adios2::Variable<int> varStep = io.DefineVariable<int>("step");

    adios2::Engine writer = io.Open(settings.output, adios2::Mode::Write);

    auto start_total = std::chrono::steady_clock::now();
    auto start_step = std::chrono::steady_clock::now();

    std::ofstream log("gray-scott.log");
    log << "step\tcompute_gs\twrite_gs" << std::endl;

    for (int i = 0; i < settings.steps; i++) {
        sim.iterate();

        if (i % settings.plotgap == 0) {
            if (rank == 0) {
                std::cout << "Simulation at step " << i
                          << " writing output step     " << i / settings.plotgap
                          << std::endl;
            }
            auto end_compute = std::chrono::steady_clock::now();


            if(settings.adios_span)
            {
            	writer.BeginStep();
            	writer.Put<int>(varStep, &i);

            	// provide memory directly from adios buffer
            	adios2::Variable<double>::Span u_span = writer.Put<double>(varU);
            	adios2::Variable<double>::Span v_span = writer.Put<double>(varV);

            	// populate spans
            	sim.u_noghost(u_span.data());
            	sim.v_noghost(v_span.data());

            	writer.EndStep();
            }
            else
            {
            	std::vector<double> u = sim.u_noghost();
            	std::vector<double> v = sim.v_noghost();
            	writer.BeginStep();
            	writer.Put<int>(varStep, &i);
            	writer.Put<double>(varU, u.data());
            	writer.Put<double>(varV, v.data());
            	writer.EndStep();
            }

            auto end_step = std::chrono::steady_clock::now();

            if (rank == 0) {
                log << i << "\t" << diff(start_step, end_compute).count()
                    << "\t" << diff(end_compute, end_step).count() << std::endl;
            }

            start_step = std::chrono::steady_clock::now();
        }
    }

    log.close();

    auto end_total = std::chrono::steady_clock::now();

    writer.Close();

    MPI_Finalize();
}

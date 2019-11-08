#include <chrono>
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <vector>
#include <string>
#include <adios2.h>
#include <sstream>
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

        //io.DefineAttribute<std::string>("vtk.xml", imageData);
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
    adios2::Mode mode;
    int start_iter;
    bool restore_mode = false;

  

    // start from beginning
    if (atoi(argv[2]) == 0 ) {
        if (rank == 0) {
            std::cout << "Start from beginning" << std::endl;
        }
        mode = adios2::Mode::Write;
        start_iter = 0;
    }

    // restore mode
    if (atoi(argv[2]) > 0) {
        if (rank == 0) {
            std::cout << "Restore mode" << std::endl;
        }
        restore_mode = true;
        int end_iter = atoi(argv[2]);
        start_iter = end_iter;
        mode = adios2::Mode::Append;
        adios2::ADIOS adios(settings.adios_config, comm, adios2::DebugON);
        adios2::IO io = adios.DeclareIO("SimulationOutput");
        io.SetEngine("BP4");
        adios2::Engine reader = io.Open(settings.output, adios2::Mode::Read);

        adios2::Box<adios2::Dims> sel({sim.offset_z, sim.offset_y, sim.offset_x}, 
                                      {sim.size_z, sim.size_y, sim.size_x});

        adios2::Variable<double> varU = io.InquireVariable<double>("U");
        varU.SetSelection(sel);

        adios2::Variable<double> varV = io.InquireVariable<double>("V");
        varV.SetSelection(sel);

        adios2::Variable<int> varStep = io.InquireVariable<int>("step");


        //std::cout << "Restore mode 1" << std::endl;
        
        //std::cout << "Restore mode 2" << std::endl;
        while (true) {
            adios2::StepStatus status = reader.BeginStep(); 
            //status = reader.BeginStep(); 
            if (status != adios2::StepStatus::OK) {
                std::cout << "Restoring failed" << std::endl;
                return 0;
            }
            std::vector<int> inStep;
            reader.Get(varStep, inStep, adios2::Mode::Sync);
            //std::cout << "Restore mode get step " << inStep[0] << std::endl;
            if (inStep[0] == end_iter) {
                //std::cout << "Load latest simulation data" << std::endl;
                std::vector<double> u;
                std::vector<double> v;
                reader.Get<double>(varU, u, adios2::Mode::Sync);
                reader.Get<double>(varV, v, adios2::Mode::Sync);
                sim.restore_field(u, v);
                break;
            }

        }
        reader.Close();
        //std::cout << "Restore mode done" << std::endl;
    }

    adios2::ADIOS adios(settings.adios_config, comm, adios2::DebugON);

    adios2::IO io = adios.DeclareIO("SimulationOutput");

    io.SetEngine("BP4");
    if (rank == 0) {
        print_io_settings(io);
        std::cout << "========================================" << std::endl;
        print_settings(settings);
        print_simulator_settings(sim);
        std::cout << "========================================" << std::endl;
    }

// start from beginning (this may cause when restoring)
    //if (!restore_mode) {
        // io.DefineAttribute<double>("F", settings.F);
        // io.DefineAttribute<double>("k", settings.k);
        // io.DefineAttribute<double>("dt", settings.dt);
        // io.DefineAttribute<double>("Du", settings.Du);
        // io.DefineAttribute<double>("Dv", settings.Dv);
        // io.DefineAttribute<double>("noise", settings.noise);
    //}   



    //define VTK visualization schema as an attribute
    // if(!settings.mesh_type.empty())
    // {
    //     define_bpvtk_attribute(settings, io);
    // }

    /* added for compression test */
    //-1: do not store U; 0: store U/V without compressor;
    //1: MGARD; 2: SZ-ABS; 3: SZ-REL; 4: SZ-PWR; 5: ZFP;
    int   compressU  = std::atoi(argv[3]);
    char* toleranceU = argv[4];
    int   compressV  = std::atoi(argv[5]);
    char* toleranceV = argv[6];

    adios2::Variable<double> varU;
    adios2::Variable<double> varV;
    if (compressU > -1) {
      varU = io.DefineVariable<double>("U", {settings.L, settings.L, settings.L},
                                       {sim.offset_z, sim.offset_y, sim.offset_x},
                                       {sim.size_z, sim.size_y, sim.size_x});
    }

    if (compressV > -1) {
      varV = io.DefineVariable<double>("V", {settings.L, settings.L, settings.L},
                                       {sim.offset_z, sim.offset_y, sim.offset_x},
                                       {sim.size_z, sim.size_y, sim.size_x});
    }
    
    if (compressU == 1) { // MGARD
      std::cout << "add mgard operator" << std::endl;
      adios2::Operator szOp =
            adios.DefineOperator("mgardCompressor_u", adios2::ops::LossyMGARD);
      varU.AddOperation(szOp, {{adios2::ops::mgard::key::tolerance, std::string(toleranceU)}});
    } else if (compressU == 2) { // SZ-ABS
      adios2::Operator szOp =
            adios.DefineOperator("szCompressor_u", adios2::ops::LossySZ);
      varU.AddOperation(szOp, {{"abs", std::string(toleranceU)}});
    } else if (compressU == 3) { // SZ-REL
      adios2::Operator szOp =
            adios.DefineOperator("szCompressor_u", adios2::ops::LossySZ);
      varU.AddOperation(szOp, {{"rel", std::string(toleranceU)}});
    } else if (compressU == 4) { // SZ-PWE
      adios2::Operator szOp =
            adios.DefineOperator("szCompressor_u", adios2::ops::LossySZ);
      varU.AddOperation(szOp, {{"pw", std::string(toleranceU)}});
    } else if (compressU == 5) { // ZFP
      adios2::Operator szOp =
            adios.DefineOperator("ZFPCompressor_u", adios2::ops::LossyZFP);
      varU.AddOperation(szOp, {{adios2::ops::zfp::key::accuracy, std::string(toleranceU)}});
    }

    if (compressV == 1) { // MGARD
      std::cout << "add mgard operator" << std::endl;
      adios2::Operator szOp =
            adios.DefineOperator("mgardCompressor_v", adios2::ops::LossyMGARD);
      varV.AddOperation(szOp, {{adios2::ops::mgard::key::accuracy, std::string(toleranceV)}});
    } else if (compressV == 2) { // SZ-ABS
      adios2::Operator szOp =
            adios.DefineOperator("szCompressor_v", adios2::ops::LossySZ);
      varV.AddOperation(szOp, {{"abs", std::string(toleranceV)}});
    } else if (compressV == 3) { // SZ-REL
      adios2::Operator szOp =
            adios.DefineOperator("szCompressor_v", adios2::ops::LossySZ);
      varV.AddOperation(szOp, {{"rel", std::string(toleranceV)}});
    } else if (compressV == 4) { // SZ-PW
      adios2::Operator szOp =
            adios.DefineOperator("szCompressor_v", adios2::ops::LossySZ);
      varV.AddOperation(szOp, {{"pw", std::string(toleranceV)}});
    } else if (compressV == 5) { // ZFP
      adios2::Operator szOp =
            adios.DefineOperator("ZFPCompressor_v", adios2::ops::LossyZFP);
      varV.AddOperation(szOp, {{adios2::ops::zfp::key::accuracy, std::string(toleranceV)}});
    }


    adios2::Variable<int> varStep = io.DefineVariable<int>("step");

    adios2::Engine writer = io.Open(settings.output, mode);

    auto start_total = std::chrono::steady_clock::now();
    auto start_step = std::chrono::steady_clock::now();

    std::ofstream log("gray-scott.log");
    log << "step\tcompute_gs\twrite_gs" << std::endl;

    std::ofstream results_csv;
    std::string csv_filename = "sim_" + std::to_string(compressU) + "_" + std::string(toleranceU) + "_" +
				std::to_string(compressV) + "_" + std::string(toleranceV) + "_" +
				"rank_" + std::to_string(rank) + ".csv"; 
    results_csv.open (csv_filename);

    for (int i = start_iter; i < settings.steps; i++) {
        //std::cout << "Simulation at step " << i << std::endl;
        auto t_start = std::chrono::high_resolution_clock::now();
        sim.iterate();
	auto t_end = std::chrono::high_resolution_clock::now();
	double sim_time = std::chrono::duration<double>(t_end-t_start).count();


        if (!restore_mode && i % settings.plotgap == 0 || 
            restore_mode && i % settings.plotgap == 0 && i != start_iter) {
            // if (rank == 0) {
            //     std::cout << " writing output step     " << i / settings.plotgap
            //               << std::endl;
            // }
            if (rank == 0) {
                std::cout << " writing output step     " << i << std::endl;
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
		// redirect stdout
		std::stringstream buffer;
  		std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
		
		std::cout << sim_time << ",";

                std::vector<double> u = sim.u_noghost();
                std::vector<double> v = sim.v_noghost();
                writer.BeginStep();
                writer.Put<int>(varStep, &i, adios2::Mode::Sync);

		t_start = std::chrono::high_resolution_clock::now();
		writer.Put<double>(varU, u.data(), adios2::Mode::Sync);
                t_end = std::chrono::high_resolution_clock::now();
		double write_time_u = std::chrono::duration<double>(t_end-t_start).count();
		std::cout << write_time_u << ",";

		t_start = std::chrono::high_resolution_clock::now();
		writer.Put<double>(varV, v.data(), adios2::Mode::Sync);
		t_end = std::chrono::high_resolution_clock::now();
		double write_time_v = std::chrono::duration<double>(t_end-t_start).count();
		std::cout << write_time_v << std::endl;
		results_csv << buffer.str();
                writer.EndStep();

		// restore stdout
		std::cout.rdbuf(old);
            }

            auto end_step = std::chrono::steady_clock::now();

            if (rank == 0) {
                log << i << "\t" << diff(start_step, end_compute).count()
                    << "\t" << diff(end_compute, end_step).count() << std::endl;
            }

            start_step = std::chrono::steady_clock::now();
        }
	
    }

    results_csv.close();

    log.close();

    auto end_total = std::chrono::steady_clock::now();

    writer.Close();

    MPI_Finalize();
}

/*
Created by Jieyang Chen
*/




#include <adios2.h>
#include <iostream>


#include <adios2.h>
#include <iostream>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/io/reader/VTKDataSetReader.h>

#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/filter/MarchingCubes.h>




/*
#include <vtkm/Types.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/filter/MarchingCubes.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
*/
using namespace std;

int main(int argc, char *argv[]) {


    /* Reading data from ADIOS file */
    //MPI_Init(&argc, &argv);

    const std::string input_fname(argv[1]);
    //const double isovalue = std::stod(argv[2]);


    adios2::ADIOS adios("adios2.xml", adios2::DebugON);
    adios2::IO inIO = adios.DeclareIO("SimulationOutput");
    adios2::Engine reader = inIO.Open(input_fname, adios2::Mode::Read);

    int total_iter = 48;
    int iter = 0;
    while (iter < total_iter) {
        adios2::StepStatus status = reader.BeginStep();
         //status = reader.BeginStep();

        if (status != adios2::StepStatus::OK) {
            cout << "Step error!";
        }


        adios2::Variable<double> varU = inIO.InquireVariable<double>("U");
        adios2::Variable<double> varV = inIO.InquireVariable<double>("V");
        const adios2::Variable<int> varStep = inIO.InquireVariable<int>("step");

        std::vector<int> step;
        reader.Get(varStep, step, adios2::Mode::Sync);
        cout << "Step: " << step[0] << endl;


        adios2::Dims shape = varU.Shape();
        cout << "Shape: " << shape[0] << ", " << shape[1] << ", " << shape[2] << endl;

        std::vector<double> pointvar_u;
        reader.Get(varU, pointvar_u, adios2::Mode::Sync);
        cout << "Size of pointvar_u: " << pointvar_u.size() << endl;


        std::vector<double> vars;


        // for (int i =0; i < 10; i++) {
        //     cout << pointvar_u[i] << endl;
            
        // }

        std::vector<double> pointvar_v;
        reader.Get(varV, pointvar_v, adios2::Mode::Sync);

        




        // vtkm::cont::DataSetBuilderUniform dsb;
  //         vtkm::Id3 dimensions(8, 8, 8);
  //         vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

        // vtkm::cont::DataSetFieldAdd dsf;
        // const int nVerts = 18;
        // //std::vector<double> vars = pointvar_v;
        // //{ 10.1f,  20.1f,  30.1f,  40.1f,  50.2f,  60.2f,
        // //                                 70.2f,  80.2f};

        // //Set point and cell scalar
        // dsf.AddPointField(dataSet, "pointvar", vars);

        





        /* Rendering using VTK-m */
        vtkm::cont::DataSet dataSet;
        vtkm::cont::DataSetBuilderUniform dataSetBuilder;
        vtkm::cont::DataSetFieldAdd dsf;


        vtkm::Id3 dims(shape[0], shape[1], shape[2]);
        vtkm::Id3 org(0,0,0);
        vtkm::Id3 spc(1,1,1);

        string dfname = "pointvar_u";
        string dfname2 = "pointvar_v";
        dataSet = dataSetBuilder.Create(dims, org, spc);
        
        dsf.AddPointField(dataSet, dfname, pointvar_u);
        dsf.AddPointField(dataSet, dfname2, pointvar_v);

        vtkm::filter::MarchingCubes filter;
        filter.SetGenerateNormals(true);
        filter.SetMergeDuplicatePoints(false);
        for (int j = 0; j < argc - 2; j++)
            filter.SetIsoValue(j, std::stod(argv[j+2]));
        //filter.SetIsoValue(1, 0.7);
        //filter.SetIsoValue(2, 0.5);
        // filter.SetIsoValue(3, 0.4);
        // filter.SetIsoValue(4, 0.5);
        // filter.SetIsoValue(5, 0.6);
        // filter.SetIsoValue(6, 0.7);
        // filter.SetIsoValue(7, 0.8);
        // filter.SetIsoValue(8, 0.9);
        // filter.SetIsoValue(9, 1.0);

        //filter.SetActiveField(dfname);
        filter.SetActiveField(dfname);
        filter.SetFieldsToPass({ dfname,dfname2 });

        vtkm::cont::DataSet outputData = filter.Execute(dataSet);

        vtkm::rendering::Scene scene;
        vtkm::cont::ColorTable colorTable("green");
        vtkm::cont::ColorTable colorTable2("inferno");
        scene.AddActor(vtkm::rendering::Actor(outputData.GetCellSet(),
                                            outputData.GetCoordinateSystem(),
                                            outputData.GetField(dfname),
                                            colorTable));
        scene.AddActor(vtkm::rendering::Actor(outputData.GetCellSet(),
                                            outputData.GetCoordinateSystem(),
                                            outputData.GetField(dfname2),
                                            colorTable2));
        
        

        using Mapper = vtkm::rendering::MapperWireframer;
        //using Mapper = vtkm::rendering::MapperRayTracer;
          using Canvas = vtkm::rendering::CanvasRayTracer;

        Mapper mapper;
        Canvas canvas(1024, 1024);

        vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);

        const vtkm::cont::CoordinateSystem coords = outputData.GetCoordinateSystem();
        vtkm::Bounds coordsBounds = coords.GetBounds();
        // cout << "X: " << coordsBounds.X.Min << " - " << coordsBounds.X.Max << endl;
        // cout << "Y: " << coordsBounds.Y.Min << " - " << coordsBounds.Y.Max << endl;
        // cout << "Z: " << coordsBounds.Z.Min << " - " << coordsBounds.Z.Max << endl;


        vtkm::rendering::Camera camera = vtkm::rendering::Camera();
        camera.ResetToBounds(coordsBounds);

        vtkm::Vec<vtkm::Float32, 3> totalExtent;
        // totalExtent[0] = vtkm::Float32(coordsBounds.X.Max - coordsBounds.X.Min);
        // totalExtent[1] = vtkm::Float32(coordsBounds.Y.Max - coordsBounds.Y.Min);
        // totalExtent[2] = vtkm::Float32(coordsBounds.Z.Max - coordsBounds.Z.Min);
        totalExtent[0] = vtkm::Float32(shape[0]);
        totalExtent[1] = vtkm::Float32(shape[1]);
        totalExtent[2] = vtkm::Float32(shape[2]);
        vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
        vtkm::Normalize(totalExtent);
        camera.SetLookAt(totalExtent * (mag * .5f));
        camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
        camera.SetClippingRange(1.f, 100.f);
        camera.SetFieldOfView(60.f);
        camera.SetPosition(totalExtent * (mag * 2.f));


        vtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);

        view.Initialize();
        view.Paint();
        string outputfile = "output";
        view.SaveAs(outputfile + std::to_string(step[0]) +" .pnm");
        iter++;
    }

}



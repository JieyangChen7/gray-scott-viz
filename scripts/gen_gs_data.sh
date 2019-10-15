#!/bin/bash

GSBIN=$HOME/dev/gray-scott-viz/gray-scott/build/gray-scott

VTKH_RENDER=$HOME/dev/gray-scott-viz/vtkh/build/gray-scott-vtkh
PROFILER_PREFIX=$HOME/dev/gray-scott-viz/scripts/compressor_profiler
NOCOMPRESSION_PROFILER=$PROFILER_PREFIX/build/profile_nocompression_adios
COMPRESSOR_PROFILER=$PROFILER_PREFIX/build/profile_compress_adios
DECOMPRESSOR_PROFILER=$PROFILER_PREFIX/build/profile_decompress_adios
ZC_PROFILER=$PROFILER_PREFIX/build/profile_zc



ZC_CONFIG=$PROFILER_PREFIX/zc.config
GS_NPROC=20
VTKH_NPROC=1
COMPRESS_NPROC=1
DECOMPRESS_NPROC=1

XMLFILE=adios2.xml


#[ ! -d "./gs_data" ]             && mkdir gs_data
#[ ! -d "./surface_area_results" ] && mkdir surface_area_results
#[ ! -d "./volume_results" ]      && mkdir volume_results
#[ ! -d "./performancce_results" ]  && mkdir performance_results

# Run GS and get simulation data
gen_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _STEPS=$3
    _GAP=$4
    _BPNAME=gs_${_SIZE}_${_NOISE}

    [ ! -d "./gs_data" ] && mkdir gs_data
    cd gs_data
    cp ../$XMLFILE .
    cp ../gs_template.json ./${_BPNAME}.json
    sed -i 's/SIZE/'"$_SIZE/"'g' ./${_BPNAME}.json
    sed -i 's/STEPS/'"$_STEPS"'/g' ./${_BPNAME}.json
    sed -i 's/GAP/'"$_GAP"'/g'  ./${_BPNAME}.json
    sed -i 's/NOISE/'"${_NOISE}"'/g'  ./${_BPNAME}.json
    sed -i 's/NNNN/'"${_BPNAME}.bp"'/g' ./${_BPNAME}.json
    mpirun -np $GS_NPROC $GSBIN ./${_BPNAME}.json
    cd ..
}


# Run GS and get simulation data
restore_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _STEPS=$3
    _GAP=$4
    _ENT_ITER=$5
    _BPNAME=gs_${_SIZE}_${_NOISE}


    [ ! -d "./gs_data" ] && mkdir gs_data
    cd gs_data
    cp ../$XMLFILE .
    cp ../gs_template.json ./${_BPNAME}.json
    sed -i 's/SIZE/'"$_SIZE/"'g' ./${_BPNAME}.json
    sed -i 's/STEPS/'"$_STEPS"'/g' ./${_BPNAME}.json
    sed -i 's/GAP/'"$_GAP"'/g'  ./${_BPNAME}.json
    sed -i 's/NOISE/'"${_NOISE}"'/g'  ./${_BPNAME}.json
    sed -i 's/NNNN/'"${_BPNAME}.bp"'/g' ./${_BPNAME}.json
    mpirun -np $GS_NPROC $GSBIN ./${_BPNAME}.json r ${_ENT_ITER}
    cd ..
}

# Filter out variables that we want to keep
process_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _COMPRESSOR_V=$4
    _BPNAME=gs_${_SIZE}_${_NOISE}
    _OUTPUT=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    cd gs_data
    mpirun -np ${COMPRESS_NPROC} $NOCOMPRESSION_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V}
    cd ..
}

# Measure read/write without compression
profile_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _COMPRESSOR_V=$4
    
    _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    _OUTPUT=profile_nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    cd gs_data
    [ -f ${_BPNAME}.csv ] && rm ${_BPNAME}.csv
    mpirun -np ${COMPRESS_NPROC} $NOCOMPRESSION_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_BPNAME}.csv
    cd ..
}

# Compress data
compress_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _SST=$7

    _COMPRESSOR_U_IN=${_COMPRESSOR_U}
    _COMPRESSOR_V_IN=${_COMPRESSOR_V}
    
    if ((${_COMPRESSOR_U} > -1))
    then
        _COMPRESSOR_U_IN=0
    fi
    if ((${_COMPRESSOR_V} > -1))
    then
        _COMPRESSOR_V_IN=0
    fi

    _BPNAME=nocompressed_${_COMPRESSOR_U_IN}_0_${_COMPRESSOR_V_IN}_0_gs_${_SIZE}_${_NOISE}
    _OUTPUT=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}


    cd gs_data
    [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv

    mpirun -np ${COMPRESS_NPROC} $COMPRESSOR_PROFILER ${_BPNAME}.bp ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_TOL_U} ${_COMPRESSOR_V} ${_TOL_V} ${_SST} >> ${_OUTPUT}.csv
    cd ..
}

# Compress data
compress_gs_data_remote() {
        _SIZE=$1
        _NOISE=$2
        _COMPRESSOR_U=$3
        _TOL_U=$4
        _COMPRESSOR_V=$5
        _TOL_V=$6
        _SST=$7
        _DIR=$8
        _HOST=$9

        _COMPRESSOR_U_IN=${_COMPRESSOR_U}
        _COMPRESSOR_V_IN=${_COMPRESSOR_V}

    if ((${_COMPRESSOR_U} > -1))
    then
        _COMPRESSOR_U_IN=0
    fi
    if ((${_COMPRESSOR_V} > -1))
    then
        _COMPRESSOR_V_IN=0
    fi

        _BPNAME=nocompressed_${_COMPRESSOR_U_IN}_0_${_COMPRESSOR_V_IN}_0_gs_${_SIZE}_${_NOISE}
        _OUTPUT=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}


        cd gs_data
        [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
    CMD='hostname && export PATH='$PATH' && export LD_LIBRARY_PATH='$LD_LIBRARY_PATH' && cd '${_DIR}'/gs_data && mpirun -np '${COMPRESS_NPROC}' '$COMPRESSOR_PROFILER' '${_BPNAME}'.bp '${_OUTPUT}'.bp '${_COMPRESSOR_U}' '${_TOL_U}' '${_COMPRESSOR_V}' '${_TOL_V}' '${_SST}' >> sst_'${_OUTPUT}'.csv'
    ssh ${_HOST} -X ${CMD}
        cd ..
}


# Decompress data
decompress_gs_data_remote() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _SST=$7
    _DIR=$8
    _HOST=$9


    _BPNAME=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
        _OUTPUT=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}

    cd gs_data
    [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
    CMD='hostname && export PATH='$PATH' && export LD_LIBRARY_PATH='$LD_LIBRARY_PATH' && cd '${_DIR}'/gs_data && mpirun -np '${DECOMPRESS_NPROC}' '$DECOMPRESSOR_PROFILER' '${_BPNAME}'.bp '${_OUTPUT}'.bp '${_COMPRESSOR_U}' '${_COMPRESSOR_V}' '${_SST}' >> sst_'${_OUTPUT}'.csv'
    ssh ${_HOST} -X ${CMD}
    cd ..
}


decompress_gs_data() {
        _SIZE=$1
        _NOISE=$2
        _COMPRESSOR_U=$3
        _TOL_U=$4
        _COMPRESSOR_V=$5
        _TOL_V=$6
        _SST=$7

        _BPNAME=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
        _OUTPUT=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}

        cd gs_data
        [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
        mpirun -np ${DECOMPRESS_NPROC} $DECOMPRESSOR_PROFILER ${_BPNAME}.bp ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${_SST} >> ${_OUTPUT}.csv
        cd ..
}

# delete data
delete_compressed_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6

    _BPNAME=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
    _OUTPUT=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}

    cd gs_data
    rm ${_BPNAME}.bp
    rm -rf ${_BPNAME}.bp.dir
    cd ..
}

delete_decompressed_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6

    _BPNAME=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
    _OUTPUT=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}

    cd gs_data
    rm ${_OUTPUT}.bp
    rm -rf ${_OUTPUT}.bp.dir
    cd ..
}

# Analyze and compare orginal and decompressed data via Z-Checker
analyze_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _COMPRESSOR_U_IN=${_COMPRESSOR_U}
    _COMPRESSOR_V_IN=${_COMPRESSOR_V}
        
    if ((${_COMPRESSOR_U} > -1))
    then
        _COMPRESSOR_U_IN=0
    fi
    if ((${_COMPRESSOR_V} > -1))
    then
        _COMPRESSOR_V_IN=0
    fi
    _ORG_BPNAME=nocompressed_${_COMPRESSOR_U_IN}_0_${_COMPRESSOR_V_IN}_0_gs_${_SIZE}_${_NOISE}
    _DEC_BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}

    _OUTPUT=analysis_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
    
    cd gs_data
    [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
    $ZC_PROFILER ${_ORG_BPNAME}.bp ${_DEC_BPNAME}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${ZC_CONFIG} >> ${_OUTPUT}.csv
    cd ..
}


# Profile read and write data without compression
nocompress_gs_data() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _COMPRESSOR_V=$4

    _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    _OUTPUT=profile_nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}

    cd gs_data
    [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
    $NOCOMPRESSION_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_OUTPUT}.csv
    cd ..
}


# Get surface area measurement via VISIT
measure_surface_area() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _START_ITER=$7
    _END_ITER=$8
    _ISO_VALUE=$9

    if (( ${_COMPRESSOR_U}>0 || ${_COMPRESSOR_V}>0 ))
        then
                echo 'decompressed version'
                _BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
        else
                echo 'original version'
                _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
        fi

    [ ! -d "./surface_area_results" ] && mkdir surface_area_results
    cd ./surface_area_results
     
    visit -cli -nowin -s ../get_surface_volume.py ../gs_data/${_BPNAME}.bp surface ${_BPNAME}_iso_${_ISO_VALUE}.csv ${_START_ITER} ${_END_ITER} ${_ISO_VALUE}
    cd ..
}

# Get volume measurement via VISIT
measure_volume() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _START_ITER=$7
    _END_ITER=$8

    if (( ${_COMPRESSOR_U}>0 || ${_COMPRESSOR_V}>0 ))
        then
                echo 'decompressed version'
                _BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
        else
                echo 'original version'
                _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
        fi
 
    [ ! -d "./volume_results" ] && mkdir volume_results
    cd ./volume_results
    
    visit -cli -nowin -s ../get_surface_volume.py ../gs_data/${_BPNAME}.bp volume ${_BPNAME}.csv ${_START_ITER} ${_END_ITER}

    cd ..
}

# Get performance data via VTK-h
measure_perf() {
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _START_ITER=$7
    _END_ITER=$8
    _GAP=$9
    _ISO_VALUE=${10}
   
    if (( ${_COMPRESSOR_U}>0 || ${_COMPRESSOR_V}>0 ))
    then
        echo 'decompressed version'
        _BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
    else
        echo 'original version'
        _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    fi

    [ ! -d "./performance_results" ]  && mkdir performance_results
    cd ./performance_results
    
    echo 'Running on serial'
    [ -f ${_BPNAME}_serial_iso_${_ISO_VALUE}.csv ] && rm ${_BPNAME}_serial_iso_${_ISO_VALUE}.csv
    mpirun -np ${VTKH_NPROC} ${VTKH_RENDER} ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 1 ${_BPNAME}_img_serial_iso_${_ISO_VALUE} ${_START_ITER} ${_END_ITER} ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${_GAP} ${_ISO_VALUE} >> ${_BPNAME}_serial_iso_${_ISO_VALUE}.csv

    echo 'Running on cuda'
    [ -f ${_BPNAME}_cuda_iso_${_ISO_VALUE}.csv ] && rm ${_BPNAME}_cuda_iso_${_ISO_VALUE}.csv
    mpirun -np ${VTKH_NPROC} ${VTKH_RENDER} ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 2 ${_BPNAME}_img_cuda_iso_${_ISO_VALUE} ${_START_ITER} ${_END_ITER} ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${_GAP} ${_ISO_VALUE} >> ${_BPNAME}_cuda_iso_${_ISO_VALUE}.csv

    echo 'Running on openmp'
    [ -f ${_BPNAME}_openmp_iso_${_ISO_VALUE}.csv ] && rm ${_BPNAME}_openmp_iso_${_ISO_VALUE}.csv
    mpirun -np ${VTKH_NPROC} ${VTKH_RENDER} ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 3 ${_BPNAME}_img_openmp_iso_${_ISO_VALUE} ${_START_ITER} ${_END_ITER} ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${_GAP} ${_ISO_VALUE} >> ${_BPNAME}_openmp_iso_${_ISO_VALUE}.csv

    cd ..
}

get_deriv(){
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _DERIV_AXIS=$7
    _ORDER_OF_DERIV=$8
    _ACCURACY=$9
    _STORE_DERIV=${10}
    _STORE_LAP=${11}
    _START_ITER=${12}
    _END_ITER=${13}

    if (( ${_COMPRESSOR_U}>0 || ${_COMPRESSOR_V}>0 ))
    then
        echo 'decompressed version'
        _BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
    else
        echo 'original version'
        _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    fi


    #[ ! -d "./deriv_results" ]  && mkdir deriv_results
    _OUTPUT=deriv_${_DERIV_AXIS}_${_ORDER_OF_DERIV}_${_ACCURACY}_${_BPNAME}.csv
    cd ./gs_data
    [ -f ${_OUTPUT} ] && rm ${_OUTPUT}
    ../fdm.py ${_BPNAME}.bp ${_OUTPUT} ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${_DERIV_AXIS} ${_ORDER_OF_DERIV} ${_ACCURACY} ${_STORE_DERIV} ${_STORE_LAP} ${_START_ITER} ${_END_ITER}
    cd ..
}


get_entropy(){
    _SIZE=$1
    _NOISE=$2
    _COMPRESSOR_U=$3
    _TOL_U=$4
    _COMPRESSOR_V=$5
    _TOL_V=$6
    _NUM_OF_BINS=$7
    _START_ITER=$8
    _END_ITER=$9

    if (( ${_COMPRESSOR_U}>0 || ${_COMPRESSOR_V}>0 ))
    then
        echo 'decompressed version'
        _BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}_${_NOISE}
    else
        echo 'original version'
        _BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}_${_NOISE}
    fi


    #[ ! -d "./deriv_results" ]  && mkdir deriv_results
    _OUTPUT=entropy_${_NUM_OF_BINS}_${_BPNAME}.csv
    cd ./gs_data
    [ -f ${_OUTPUT} ] && rm ${_OUTPUT}
    ../entropy.py ${_BPNAME}.bp ${_OUTPUT} ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${_NUM_OF_BINS} ${_START_ITER} ${_END_ITER}
    cd ..
}

STORE_U=-1
STORE_V=0
SIZE=64
NOISE=0.0
STEP=300
GAP=30
ITER=10


echo 'generating Gray-scott simulation data'
gen_gs_data ${SIZE} ${NOISE} ${STEP} ${GAP} 

STEP=600
GAP=30
ITER=20

echo 'restore Gray-scott simulation data'
restore_gs_data ${SIZE} ${NOISE} ${STEP} ${GAP} 270

echo 'processing data'
process_gs_data ${SIZE} ${NOISE} ${STORE_U} ${STORE_V}

echo 'profile read/write without compression'
profile_gs_data ${SIZE} ${NOISE} ${STORE_U} ${STORE_V}


#measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${ITER} 0.1
# measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${ITER} 0.2
# measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${ITER} 0.3
# measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${ITER} 0.4
# measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${ITER} 0.5


#echo 'measuring performance without compression'
#measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${STEP} ${GAP} 0.1
# measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${STEP} ${GAP} 0.2
# measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${STEP} ${GAP} 0.3
# measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${STEP} ${GAP} 0.4
# measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 ${STEP} ${GAP} 0.5

#get_deriv ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 0 4 4 0 0 0 10
#get_entropy ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 1000000 0 ${ITER}

#echo 'profile read write without compression for SST'
#SST=1
#NODE1=login4 
#NODE2=login5
#DIR=/ccs/home/jieyang/dev/gray-scott-viz/scripts
#compress_gs_data_remote ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 $SST $DIR $NODE1 &
#sleep 10
#decompress_gs_data_remote ${SIZE} ${NOISE} ${STORE_U} 0 ${STORE_V} 0 $SST $DIR $NODE2 


for COMPRESSOR in 1 #2 3 4 5 # 1=MAGRD; 2=SZ-ABS; 3=SZ-REL; 4=SZ=PWL; 5=ZFP
do
    echo 'using compressor #' $COMPRESSOR

    for TOL_V in 0.00000001 #0.0000001 0.000001 0.00001 0.0001 0.001 0.01 0.1 1 10
    do
        DUMMY=123
        SST=0
        echo 'compressing with tolerance = ' $TOL_V
        compress_gs_data ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V $SST

        echo 'decompressing on data with lossy tolerance = ' $TOL_V
        decompress_gs_data ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V $SST

        echo 'analyzing data'
        analyze_gs_data ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V

        #echo 'performance test on data with lossy tolerance =' $TOL_V
        #measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${STEP} ${GAP} 0.1
       # measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${STEP} ${GAP} 0.2
       # measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${STEP} ${GAP} 0.3
       # measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${STEP} ${GAP} 0.4
       # measure_perf ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${STEP} ${GAP} 0.5

       #echo 'measuring surface area'
       #measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${ITER} 0.1
       # measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${ITER} 0.2
       # measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${ITER} 0.3
       # measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${ITER} 0.4
       # measure_surface_area ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 ${ITER} 0.5
	
       #echo 'calculating derivative'
       #get_deriv ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 0 4 4 0 0 0 ${STEP}

 	#echo 'calculating entropy'
	#get_entropy ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V 1000000 0 ${ITER}

        # #SST=1
        # #compress_gs_data_remote ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V $SST $DIR $NODE1 &
        # #sleep 10
        # #decompress_gs_data_remote ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V $SST $DIR $NODE2
        #delete_compressed_data ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V
        #delete_decompressed_data ${SIZE} ${NOISE} ${STORE_U} 0 $COMPRESSOR $TOL_V
    done
done

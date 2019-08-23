#!/bin/bash

GSBIN=$HOME/dev/gray-scott-viz/gray-scott/build/gray-scott

VTKH_RENDER=$HOME/dev/gray-scott-viz/vtkh/build/gray-scott-vtkh
PROFILER_PREFIX=$HOME/dev/gray-scott-viz/scripts/compressor_profiler
NOCOMPRESSION_PROFILER=$PROFILER_PREFIX/build/profile_nocompression_adios
COMPRESSOR_PROFILER=$PROFILER_PREFIX/build/profile_compress_adios
DECOMPRESSOR_PROFILER=$PROFILER_PREFIX/build/profile_decompress_adios
ZC_PROFILER=$PROFILER_PREFIX/build/profile_zc



ZC_CONFIG=$PROFILER_PREFIX/zc.config
GS_NPROC=16
VTKH_NPROC=1
COMPRESS_NPROC=1
DECOMPRESS_NPROC=1

XMLFILE=adios2.xml


#[ ! -d "./gs_data" ] 	          && mkdir gs_data
#[ ! -d "./surface_area_results" ] && mkdir surface_area_results
#[ ! -d "./volume_results" ] 	  && mkdir volume_results
#[ ! -d "./performancce_results" ]  && mkdir performance_results

# Run GS and get simulation data
gen_gs_data() {
	_SIZE=$1
	_STEPS=$2
	_GAP=$3
        _STORE_U=$4
        _STORE_V=$5	
	_BPNAME=nocompressed_${_STORE_U}_0_${_STORE_V}_0_gs_${_SIZE}

	[ ! -d "./gs_data" ] && mkdir gs_data
	cd gs_data
	cp ../$XMLFILE .
	cp ../gs_template.json ./${_BPNAME}.json
	sed -i 's/SIZE/'"$_SIZE/"'g' ./${_BPNAME}.json
	sed -i 's/STEPS/'"$_STEPS"'/g' ./${_BPNAME}.json
	sed -i 's/GAP/'"$_GAP"'/g'  ./${_BPNAME}.json
	sed -i 's/NNNN/'"${_BPNAME}.bp"'/g' ./${_BPNAME}.json
	mpirun -np $GS_NPROC $GSBIN ./${_BPNAME}.json ${_STORE_U} ${_STORE_V}
	cd ..
}

# Filter out variables that we want to keep
process_gs_data() {
	_SIZE=$1
	_COMPRESSOR_U=$2
	_COMPRESSOR_V=$3
    _BPNAME=gs_${_SIZE}
    _OUTPUT=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}
    cd gs_data
    $NOCOMPRESSION_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V}
    cd ..
}

# Compress data
compress_gs_data() {
	_SIZE=$1
	_COMPRESSOR_U=$2
	_TOL_U=$3
	_COMPRESSOR_V=$4
	_TOL_V=$5

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

	_BPNAME=nocompressed_${_COMPRESSOR_U_IN}_0_${_COMPRESSOR_V_IN}_0_gs_${_SIZE}
	_OUTPUT=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}


	cd gs_data
	[ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
	#echo $COMPRESSOR
	mpirun -np ${COMPRESS_NPROC} $COMPRESSOR_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_TOL_U} ${_COMPRESSOR_V} ${_TOL_V} >> ${_OUTPUT}.csv
	cd ..
}

# Decompress data
decompress_gs_data() {
	_SIZE=$1
	_COMPRESSOR_U=$2
	_TOL_U=$3
	_COMPRESSOR_V=$4
	_TOL_V=$5

	_BPNAME=compressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}
        _OUTPUT=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}

	cd gs_data
	[ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
	mpirun -np ${DECOMPRESS_NPROC} $DECOMPRESSOR_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_OUTPUT}.csv
	cd ..
}

# Analyze and compare orginal and decompressed data via Z-Checker
analyze_gs_data() {
	_SIZE=$1
        _COMPRESSOR_U=$2
        _TOL_U=$3
        _COMPRESSOR_V=$4
        _TOL_V=$5
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
        _ORG_BPNAME=nocompressed_${_COMPRESSOR_U_IN}_0_${_COMPRESSOR_V_IN}_0_gs_${_SIZE}
        _DEC_BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}
 
        _OUTPUT=analysis_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}
        
        cd gs_data
        [ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
        $ZC_PROFILER ${_ORG_BPNAME}.bp ${_DEC_BPNAME}.bp $XMLFILE ${_COMPRESSOR_U} ${_COMPRESSOR_V} ${ZC_CONFIG} >> ${_OUTPUT}.csv
	cd ..
}


# Profile read and write data without compression
nocompress_gs_data() {
	_SIZE=$1
	_COMPRESSOR_U=$2
	_COMPRESSOR_V=$3

	_BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}
        _OUTPUT=profile_nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}

	cd gs_data
	[ -f ${_OUTPUT}.csv ] && rm ${_OUTPUT}.csv
	$NOCOMPRESSION_PROFILER ${_BPNAME}.bp $XMLFILE ${_OUTPUT}.bp ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_OUTPUT}.csv
	cd ..
}


# Get surface area measurement via VISIT
measure_surface_area() {
	_SIZE=$1
	_COMPRESSOR_U=$2
	_TOL_U=$3
	_COMPRESSOR_V=$4
	_TOL_V=$5

	if [ ${_COMPRESSOR} -gt 0 ]
	then
		_BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}
	else
		_BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}
	fi

	[ ! -d "./surface_area_results" ] && mkdir surface_area_results
	cd ./surface_area_results
	# simple 
	visit -cli -nowin -s ../test.py ../gs_data/${_BPNAME}.bp surface_simple ${_BPNAME}_simple.csv 0 100
	# medium
	visit -cli -nowin -s ../test.py ../gs_data/${_BPNAME}.bp surface_simple ${_BPNAME}_medium.csv 100 300
	# complex
	visit -cli -nowin -s ../test.py ../gs_data/${_BPNAME}.bp surface_complex ${_BPNAME}_complex.csv 200 300
	cd ..
}

# Get volume measurement via VISIT
measure_volume() {
	_SIZE=$1
	_COMPRESSOR_U=$2
	_TOL_U=$3
	_COMPRESSOR_V=$4
	_TOL_V=$5

	if [ ${_COMPRESSOR_U} -gt 0 ] && [ ${_COMPRESSOR_V} -gt 0 ]
	then
		_BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}
	else
		_BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}
	fi

	[ ! -d "./volume_results" ] && mkdir volume_results
	cd ./volume_results
	# simple 
	visit -cli -nowin -s ../test.py ../gs_data/${_BPNAME}.bp volume_simple ${_BPNAME}_simple.csv 0 100
	# medium
	visit -cli -nowin -s ../test.py ../gs_data/${_BPNAME}.bp volume_simple ${_BPNAME}_medium.csv 0 300
	# complex
	visit -cli -nowin -s ../test.py ../gs_data/${_BPNAME}.bp volume_complex ${_BPNAME}_complex.csv 200 300
	cd ..
}

# Get performance data via VTK-h
measure_perf() {
    _SIZE=$1
    _COMPRESSOR_U=$2
    _TOL_U=$3
    _COMPRESSOR_V=$4
    _TOL_V=$5
    _START_ITER=$6
    _END_ITER=$7
   
    if (( ${_COMPRESSOR_U}>0 || ${_COMPRESSOR_V}>0 ))
	then
		echo 'decompressed version'
		_BPNAME=decompressed_${_COMPRESSOR_U}_${_TOL_U}_${_COMPRESSOR_V}_${_TOL_V}_gs_${_SIZE}
	else
		echo 'original version'
		_BPNAME=nocompressed_${_COMPRESSOR_U}_0_${_COMPRESSOR_V}_0_gs_${_SIZE}
	fi

    [ ! -d "./performance_results" ]  && mkdir performance_results
    cd ./performance_results
    
    echo 'Running on serial'
    [ -f ${_BPNAME}_serial.csv ] && rm ${_BPNAME}_serial.csv
    mpirun -np ${VTKH_NPROC} ${VTKH_RENDER} ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 1 ${_BPNAME}_img_serial ${_START_ITER} ${_END_ITER} ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_BPNAME}_serial.csv

    #echo 'Running on openmp'
    #[ -f ${_BPNAME}_openmp.csv ] && rm ${_BPNAME}_openmp.csv
    #mpirun -np ${VTKH_NPROC} ${VTKH_RENDER} ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 1 ${_BPNAME}_img_openmp ${_START_ITER} ${_END_ITER} ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_BPNAME}_openmp.csv

    #echo 'Running on serial'
    #[ -f ${_BPNAME}_cuda.csv ] && rm ${_BPNAME}_cuda.csv
    #mpirun -np ${VTKH_NPROC} ${VTKH_RENDER} ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 1 ${_BPNAME}_img_cuda ${_START_ITER} ${_END_ITER} ${_COMPRESSOR_U} ${_COMPRESSOR_V} >> ${_BPNAME}_cuda.csv

    cd ..
}


STORE_U=-1
STORE_V=0

echo 'generating Gray-scott simulation data'
gen_gs_data 64 3000 10 ${STORE_U} ${STORE_V}


#measure_surface_area 64 3000 10 0 0 0 0
#measure_volume 64 3000 10 0 0 0 0
#echo 'measuring performance'
#measure_perf 64 ${STORE_U} 0 ${STORE_V} 0 0 100
#gen_data 128 3000 100
#gen_data 256 3000 100


#echo 'profile read write without compression'
#compress_gs_data 64 ${STORE_U} 0 ${STORE_V} 0 &
#decompress_gs_data 64 ${STORE_U} 0 ${STORE_V} 0 


for COMPRESSOR in 1 2 3 # 1=MAGRD; 2=SZ; 3=ZFP
do
	echo 'using compressor #' $COMPRESSOR

	for TOL_V in 0.000001 0.00001 0.0001 0.001 0.01 0.1
	do
		DUMMY=123
		#echo 'compressing with tolerance = ' $TOL_V
		#compress_gs_data 64 -1 0 $COMPRESSOR $TOL_V
	        #echo 'decompressing on data with lossy tolerance = ' $TOL_V
		#decompress_gs_data 64 -1 0 $COMPRESSOR $TOL_V
#		echo 'analyzing data'
#		analyze_gs_data 64 -1 0 $COMPRESSOR $TOL_V
		#echo 'performance test on data with lossy tolerance =' $TOL_V
		#measure_perf 64 -1 0 $COMPRESSOR $TOL_V
	done
done


for COMPRESSOR in 3 2 1 # 1=MAGRD; 2=SZ; 3=ZFP
do
	for TOL in 0.000001 0.00001 0.0001 0.001 0.01 0.1
	do
		VAR=2
		#gen_data 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
		#measure_surface_area 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
		#measure_volume 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
		#measure_perf 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
	done
done

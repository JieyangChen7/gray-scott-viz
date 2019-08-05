#!/bin/bash

GSBIN=/home/4yc/software/adiosvm/Tutorial/gray-scott/build/gray-scott
DECOMPRESSOR=/home/4yc/dev/gray-scott-viz/scripts/decompress_tool/build/decompress_adios
NPROC=16
XMLFILE=adios2.xml


#[ ! -d "./gs_data" ] 	          && mkdir gs_data
#[ ! -d "./surface_area_results" ] && mkdir surface_area_results
#[ ! -d "./volume_results" ] 	  && mkdir volume_results
#[ ! -d "./performancce_results" ]  && mkdir performance_results


gen_data() {
	_SIZE=$1
	_STEPS=$2
	_GAP=$3
	
	_COMPRESSOR=$4
	_VAR=$5
	_TOL_U=$6
	_TOL_V=$7

	_BPNAME=gs_${_SIZE}_${_COMPRESSOR}_${_VAR}_${_TOL_U}_${_TOL_V}

	[ ! -d "./gs_data" ] && mkdir gs_data
	cd gs_data
	cp ../$XMLFILE .
	cp ../gs_template.json ./${_BPNAME}.json
	sed -i 's/SIZE/'"$_SIZE/"'g' ./${_BPNAME}.json
	sed -i 's/STEPS/'"$_STEPS"'/g' ./${_BPNAME}.json
	sed -i 's/GAP/'"$_GAP"'/g'  ./${_BPNAME}.json
	sed -i 's/NNNN/'"${_BPNAME}.bp"'/g' ./${_BPNAME}.json
	mpirun -np $NPROC $GSBIN ./${_BPNAME}.json ${_VAR} ${_COMPRESSOR} ${_TOL_U} ${_TOL_V}
	if [ ${_COMPRESSOR} -gt 0 ]
	then
		$DECOMPRESSOR ${_BPNAME}.bp $XMLFILE
	fi
	cd ..
}

measure_surface_area() {
	_SIZE=$1
	_STEPS=$2
	_GAP=$3

	_COMPRESSOR=$4
	_VAR=$5
	_TOL_U=$6
	_TOL_V=$7

	if [ ${_COMPRESSOR} -gt 0 ]
	then
		_BPNAME=decompressed_gs_${_SIZE}_${_COMPRESSOR}_${_VAR}_${_TOL_U}_${_TOL_V}
	else
		_BPNAME=gs_${_SIZE}_${_COMPRESSOR}_${_VAR}_${_TOL_U}_${_TOL_V}
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


measure_volume() {
	_SIZE=$1
	_STEPS=$2
	_GAP=$3

	_COMPRESSOR=$4
	_VAR=$5
	_TOL_U=$6
	_TOL_V=$7

	if [ ${_COMPRESSOR} -gt 0 ]
	then
		_BPNAME=decompressed_gs_${_SIZE}_${_COMPRESSOR}_${_VAR}_${_TOL_U}_${_TOL_V}
	else
		_BPNAME=gs_${_SIZE}_${_COMPRESSOR}_${_VAR}_${_TOL_U}_${_TOL_V}
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


measure_perf() {
        _SIZE=$1
        _STEPS=$2
        _GAP=$3

        _COMPRESSOR=$4
        _VAR=$5
        _TOL_U=$6
        _TOL_V=$7

       
        _BPNAME=gs_${_SIZE}_${_COMPRESSOR}_${_VAR}_${_TOL_U}_${_TOL_V}

        [ ! -d "./performance_results" ]  && mkdir performance_results
        cd ./performance_results
        

        ../../vtkh/build/gray-scott-vtkh ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 1 ${_BPNAME}_img_simple_serial 0 100 >> ${_BPNAME}_simple_serial.csv

	../../vtkh/build/gray-scott-vtkh ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 2 ${_BPNAME}_img_simple_openmp 0 100 >> ${_BPNAME}_simple_openmp.csv

        ../../vtkh/build/gray-scott-vtkh ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 3 ${_BPNAME}_img_simple_cuda 0 100 >> ${_BPNAME}_simple_cuda.csv

	../../vtkh/build/gray-scott-vtkh ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 1 ${_BPNAME}_img_complex_serial 200 300 >> ${_BPNAME}_complex_serial.csv

        ../../vtkh/build/gray-scott-vtkh ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 2 ${_BPNAME}_img_complex_openmp 200 300 >> ${_BPNAME}_complex_openmp.csv

        ../../vtkh/build/gray-scott-vtkh ../gs_data/${_BPNAME}.bp ../gs_data/adios2.xml ${_SIZE} ${_SIZE} 3 ${_BPNAME}_img_complex_cuda 200 300 >> ${_BPNAME}_complex_cuda.csv

        cd ..
}






#gen_data 64 3000 10 0 0 0 0
#measure_surface_area 64 3000 10 0 0 0 0
#measure_volume 64 3000 10 0 0 0 0
measure_perf 64 3000 10 0 0 0 0
#gen_data 128 3000 100
#gen_data 256 3000 100



#for COMPRESSOR in 1 2 3 # 1=MAGRD; 2=SZ; 3=ZFP
#do
#	for VAR in 1 2 3 # only U=1; only V=2; both U and V=3
#	do
#		for TOL_U in 0.000001 0.00001 0.0001 0.001 0.01 0.1
#			for TOL_V in 0.000001 0.00001 0.0001 0.001 0.01 0.1
#			do
#				gen_data 64 3000 100 $COMPRESSOR $VAR $TOL_U $TOL_V
#			done
#		done
#	done
#done


for COMPRESSOR in 3 2 1 # 1=MAGRD; 2=SZ; 3=ZFP
do
	for TOL in 0.000001 0.00001 0.0001 0.001 0.01 0.1
	do
		VAR=2
		#gen_data 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
		#measure_surface_area 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
		#measure_volume 64 3000 10 $COMPRESSOR $VAR $TOL $TOL
	done
done

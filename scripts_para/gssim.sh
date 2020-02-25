#!/bin/bash 
### Begin BSUB Options 
#BSUB -P PPP
#BSUB -J Gray-Scott-Sim 
#BSUB -W TTT 
#BSUB -nnodes DDD 
#BSUB -alloc_flags "smt4"
#BSUB -o GSSim.%J
#BSUB -e GSSim.%J

### End BSUB Options and begin shell commands

JSRUN_CMD='jsrun --nrs DDD --tasks_per_rs RRR --cpu_per_rs 42 --rs_per_host 1 --latency_priority CPU-CPU --launch_distribution packed --bind packed:1'

CODE_DIR=$HOME/dev/gray-scott-viz
DATA_DIR=/gpfs/alpine/scratch/jieyang/PPP

GSBIN=$CODE_DIR/gray-scott/build/gray-scott
XMLFILE=adios2.xml

# Run GS and get simulation data
_SIZE=111
_NOISE=222
_COMP_U=333
_TOL_U=444
_COMP_V=555
_TOL_V=666
_NNODES=DDD
_PPNODE=RRR
_START_STEP=777
_END_STEP=888
_STEP_GAP=999

_RESTORE_FILE=gssim_data_${_SIZE}_${_NOISE}_0_0_0_0_${_NNODES}_${_PPNODE}_${_START_STEP}_${_END_STEP}_${_STEP_GAP}.bp

echo 'restore file:'
echo ${_RESTORE_FILE}



if (( ${_COMP_U}>0 || ${_COMP_V}>0 ))
then
    echo 'compressed version'
    _BPNAME=gssim_data_${_SIZE}_${_NOISE}_${_COMP_U}_${_TOL_U}_${_COMP_V}_${_TOL_V}_${_NNODES}_${_PPNODE}_${_START_STEP}_${_END_STEP}_${_STEP_GAP}
else
    echo 'original version'
    _BPNAME=gssim_data_${_SIZE}_${_NOISE}_0_0_0_0_${_NNODES}_${_PPNODE}_${_START_STEP}_${_END_STEP}_${_STEP_GAP}
fi

[ ! -d "${DATA_DIR}/gs_data" ] && mkdir -p ${DATA_DIR}/gs_data
cd ${DATA_DIR}/gs_data
cp ${CODE_DIR}/scripts/$XMLFILE .
cp ${CODE_DIR}/scripts/gs_template.json ./${_BPNAME}.json
sed -i 's/SIZE/'"${_SIZE}/"'g' ./${_BPNAME}.json
sed -i 's/STEPS/'"${_END_STEP}"'/g' ./${_BPNAME}.json
sed -i 's/GAP/'"${_STEP_GAP}"'/g'  ./${_BPNAME}.json
sed -i 's/NOISE/'"${_NOISE}"'/g'  ./${_BPNAME}.json
sed -i 's/NNNN/'"${_BPNAME}.bp"'/g' ./${_BPNAME}.json
${JSRUN_CMD} $GSBIN ./${_BPNAME}.json ${_COMP_U} ${_TOL_U} ${_COMP_V} ${_TOL_V} ${_NNODES} ${_PPNODE} ${_START_STEP} ${_RESTORE_FILE}




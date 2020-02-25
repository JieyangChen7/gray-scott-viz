#!/bin/bash

PROJ=csc331
TIME=00:12

NNODES=8
PPNODE=125

COMP_U=0
TOL_U=0
COMP_V=0
TOL_V=0
SIZE=512
NOISE=0.0
END_STEP=3000
STEP_GAP=100
TOTAL_ITER=$((${END_STEP}/${STEP_GAP}))
START_STEP=0

ISO_VALUE=0.1
EXEC_DEV=1

JOB_SCRIPT_TEMPLATE_SIM=./gssim.sh
JOB_SCRIPT_TEMPLATE_VIS=./vis.sh


submit_job_sim()
{
    _SIZE=$1
    _NOISE=$2
    _COMP_U=$3
    _TOL_U=$4
    _COMP_V=$5
    _TOL_V=$6	
    _START_STEP=$7
    _END_STEP=$8
    _STEP_GAP=$9

    JOB_SCRIPT=./gssim_${PROJ}_${TIME}_${NNODES}_${PPNODE}_${_SIZE}_${_NOISE}_${_START_STEP}_${_END_STEP}_${_STEP_GAP}_${_COMP_U}_${_TOL_U}_${_COMP_V}_${_TOL_V}.sh

    echo 'job script: '${JOB_SCRIPT}
    cp ${JOB_SCRIPT_TEMPLATE_SIM} ${JOB_SCRIPT}

    sed -i 's/PPP/'"${PROJ}"'/g' ${JOB_SCRIPT}
    sed -i 's/TTT/'"${TIME}"'/g' ${JOB_SCRIPT}
    sed -i 's/DDD/'"${NNODES}"'/g' ${JOB_SCRIPT}
    sed -i 's/RRR/'"${PPNODE}"'/g' ${JOB_SCRIPT}

    sed -i 's/111/'"${_SIZE}"'/g' ${JOB_SCRIPT}
    sed -i 's/222/'"${_NOISE}"'/g' ${JOB_SCRIPT}
    sed -i 's/333/'"${_COMP_U}"'/g' ${JOB_SCRIPT}
    sed -i 's/444/'"${_TOL_U}"'/g' ${JOB_SCRIPT}
    sed -i 's/555/'"${_COMP_V}"'/g' ${JOB_SCRIPT}
    sed -i 's/666/'"${_TOL_V}"'/g' ${JOB_SCRIPT}
    sed -i 's/777/'"${_START_STEP}"'/g' ${JOB_SCRIPT}
    sed -i 's/888/'"${_END_STEP}"'/g' ${JOB_SCRIPT}
    sed -i 's/999/'"${_STEP_GAP}"'/g' ${JOB_SCRIPT}


    echo 'bsub' ${JOB_SCRIPT}
    bsub ${JOB_SCRIPT}
}


submit_job_vis()
{
    _SIZE=$1
    _NOISE=$2
    _COMP_U=$3
    _TOL_U=$4
    _COMP_V=$5
    _TOL_V=$6
    _START_STEP=$7
    _END_STEP=$8
    _STEP_GAP=$9
    _ISO_VALUE=${10}
    _EXEC_DEV=${11}

    JOB_SCRIPT=./vis_${PROJ}_${TIME}_${NNODES}_${PPNODE}_${_SIZE}_${_NOISE}_${_START_STEP}_${_END_STEP}_${_STEP_GAP}_${_COMP_U}_${_TOL_U}_${_COMP_V}_${_TOL_V}_${_ISO_VALUE}_${_EXEC_DEV}.sh

    echo 'job script: '${JOB_SCRIPT}
    cp ${JOB_SCRIPT_TEMPLATE_VIS} ${JOB_SCRIPT}


    sed -i 's/PPP/'"${PROJ}"'/g' ${JOB_SCRIPT}
    sed -i 's/TTT/'"${TIME}"'/g' ${JOB_SCRIPT}
    sed -i 's/DDD/'"${NNODES}"'/g' ${JOB_SCRIPT}
    sed -i 's/RRR/'"${PPNODE}"'/g' ${JOB_SCRIPT}

    sed -i 's/111/'"${_SIZE}"'/g' ${JOB_SCRIPT}
    sed -i 's/222/'"${_NOISE}"'/g' ${JOB_SCRIPT}
    sed -i 's/333/'"${_COMP_U}"'/g' ${JOB_SCRIPT}
    sed -i 's/444/'"${_TOL_U}"'/g' ${JOB_SCRIPT}
    sed -i 's/555/'"${_COMP_V}"'/g' ${JOB_SCRIPT}
    sed -i 's/666/'"${_TOL_V}"'/g' ${JOB_SCRIPT}
    sed -i 's/777/'"${_START_STEP}"'/g' ${JOB_SCRIPT}
    sed -i 's/888/'"${_END_STEP}"'/g' ${JOB_SCRIPT}
    sed -i 's/999/'"${_STEP_GAP}"'/g' ${JOB_SCRIPT}
  
    sed -i 's/III/'"${_ISO_VALUE}"'/g' ${JOB_SCRIPT}
    sed -i 's/EEE/'"${_EXEC_DEV}"'/g' ${JOB_SCRIPT}


    echo 'bsub' ${JOB_SCRIPT}
    bsub ${JOB_SCRIPT}
}



submit_job_sim ${SIZE} ${NOISE} ${COMP_U} ${TOL_U} ${COMP_V} ${TOL_V} ${START_STEP} ${END_STEP} ${STEP_GAP}

#submit_job_vis ${SIZE} ${NOISE} ${COMP_U} ${TOL_U} ${COMP_V} ${TOL_V} ${START_STEP} ${END_STEP} ${STEP_GAP} ${ISO_VALUE} ${EXEC_DEV}

for COMP in 1 2 3 4 5 # 1=MAGRD; 2=SZ-ABS; 3=SZ-REL; 4=SZ=PWL; 5=ZFP
do
    #echo 'using compressor #' $COMPRESSOR

    for TOL in 0.00000001 0.0000001 0.000001 0.00001 0.0001 #0.001 0.01 0.1 1
    do
        DUMMY=123
        SST=0

        #echo 'generating Gray-scott simulation data'
        #submit_job_sim ${SIZE} ${NOISE} ${COMP} ${TOL} ${COMP} ${TOL} ${START_STEP} ${END_STEP} ${STEP_GAP}
    done

done

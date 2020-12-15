#!/bin/sh
# Usage: ./verify.sh <ABELIAN_EXECUTABLE_NAME> <INPUT_GRAPH_NAME>
# environment variables: ABELIAN_NON_HETEROGENEOUS ABELIAN_GALOIS_ROOT ABELIAN_EDGE_CUT_ONLY
# executes only on single machine
# assumes 2 GPU devices available (if heterogeneous)

execdirname="."
execname=$1
EXEC=${execdirname}/${execname}

inputdirname=/net/ohm/export/iss/dist-inputs
#inputdirname=/workspace/dist-inputs
inputname=$2
extension=gr

option=$3

outputdirname=/net/ohm/export/iss/dist-outputs
#outputdirname=/workspace/dist-outputs

IFS='_' read -ra EXECP <<< "$execname"
problem=${EXECP[0]}
OUTPUT=${outputdirname}/${inputname}.${problem}

# kcore output files have a number at the end specifying kcore number
if [[ $execname == *"kcore"* ]]; then
  # TODO: update this for non-100 kcore numbers
  OUTPUT=${outputdirname}/${inputname}.${problem}100
fi

# for bc, do single source outputs
if [[ ($execname == *"bc"*) ]]; then
  OUTPUT=${outputdirname}/${inputname}.ssbc
fi

# for bc, if using rmat15, then use all sources output (without ss)
if [[ ($execname == *"bc"*) && ($inputname == "rmat15") ]]; then
  OUTPUT=${outputdirname}/rmat15.bcbfsall
fi

MPI=mpiexec
LOG=.verify_log

FLAGS=
# kcore flag
if [[ $execname == *"kcore"* ]]; then
  FLAGS+=" -kcore=100"
fi
if [[ ($execname == *"bfs"*) || ($execname == *"sssp"*) ]]; then
  if [[ -f "${inputdirname}/${inputname}.source" ]]; then
    FLAGS+=" -startNode=`cat ${inputdirname}/${inputname}.source`"
  fi
fi

# bc: if rmat15 is not used, specify single source flags else do
# all sources for rmat15
if [[ ($execname == *"bc"*) && ! ($inputname == "rmat15") ]]; then
  FLAGS+=" -singleSource"
  FLAGS+=" -startNode=`cat ${inputdirname}/${inputname}.source`"
fi

# batch multiple sources if using mrbc
if [[ ($execname == *"bc_mr"*) ]]; then
  FLAGS+=" -numRoundSources=4096"
fi

source_file=${inputdirname}/source
if [[ $execname == *"cc"* || $execname == *"kcore"* ]]; then
  inputdirname=${inputdirname}/symmetric
  extension=sgr
  FLAGS+=" -symmetricGraph"
else 
  # for verify purposes, always pass in graph transpose just in case it is 
  # needed for non-symmetric graphs
  FLAGS+=" -graphTranspose=${inputdirname}/transpose/${inputname}.tgr"
fi

FLAGS+=" -maxIterations=10000000"

grep "${inputname}.${extension}" ${source_file} >>$LOG
INPUT=${inputdirname}/${inputname}.${extension}

if [ -z "$ABELIAN_GALOIS_ROOT" ]; then
  ABELIAN_GALOIS_ROOT=/net/velocity/workspace/SourceCode/Galois
fi
checker=${ABELIAN_GALOIS_ROOT}/scripts/result_checker.py
#checker=./result_checker.py

hostname=`hostname`

if [ -z "$ABELIAN_NON_HETEROGENEOUS" ]; then
  # assumes only 2 GPUs device available
  #SET="g,1,48 gg,2,24 gggg,4,12 gggggg,6,8 c,1,48 cc,2,24 cccc,4,12 cccccccc,8,6 cccccccccccccccc,16,3"
  SET="g,1,16 gg,2,8 gc,2,8 cg,2,8, ggc,3,4 cgg,3,4 c,1,16 cc,2,8 ccc,3,4 cccc,4,4 ccccc,5,2 cccccc,6,2 ccccccc,7,2 cccccccc,8,2 ccccccccc,9,1 cccccccccc,10,1 ccccccccccc,11,1 cccccccccccc,12,1 ccccccccccccc,13,1 cccccccccccccc,14,1 cccccccccccccc,15,1 ccccccccccccccc,16,1"
else
  #SET="c,1,48 cc,2,24 cccc,4,12 cccccccc,8,6 cccccccccccccccc,16,3"
  #SET="c,1,80 cc,2,40 cccc,4,20 cccccccc,8,10 ccccccccccccccc,16,5"
  SET="c,1,16 cc,2,8 ccc,3,4 cccc,4,4 ccccc,5,2 cccccc,6,2 ccccccc,7,2 cccccccc,8,2 ccccccccc,9,1 cccccccccc,10,1 ccccccccccc,11,1 cccccccccccc,12,1 ccccccccccccc,13,1 cccccccccccccc,14,1 cccccccccccccc,15,1 ccccccccccccccc,16,1"
fi

pass=0
fail=0
failed_cases=""
#for partition in 1 2 3 4 5 6 7 8 9 10 11 12; do
for partition in 1 2 3 4 5 6; do
#for partition in 1; do
  CUTTYPE=

  if [ $partition -eq 1 ]; then
    CUTTYPE+=" -partition=oec"
  elif [ $partition -eq 2 ]; then
    CUTTYPE+=" -partition=iec"
  elif [ $partition -eq 3 ]; then
    CUTTYPE+=" -partition=cvc"
  elif [ $partition -eq 4 ]; then
    CUTTYPE+=" -partition=cvc-iec"
  elif [ $partition -eq 5 ]; then
    CUTTYPE+=" -partition=hovc"
  elif [ $partition -eq 6 ]; then
    CUTTYPE+=" -partition=hivc"
  elif [ $partition -eq 7 ]; then
    CUTTYPE+=" -partition=fennel-o -stateRounds=100"
  elif [ $partition -eq 8 ]; then
    CUTTYPE+=" -partition=fennel-i -stateRounds=100"
  elif [ $partition -eq 9 ]; then
    CUTTYPE+=" -partition=ginger-o -stateRounds=100"
  elif [ $partition -eq 10 ]; then
    CUTTYPE+=" -partition=ginger-i -stateRounds=100"
  elif [ $partition -eq 11 ]; then
    CUTTYPE+=" -partition=sugar-o -stateRounds=100"
  elif [ $partition -eq 12 ]; then
    CUTTYPE+=" -partition=sugar-i -stateRounds=100"
  fi

  for task in $SET; do
    old_ifs=$IFS
    IFS=",";
    set $task;
    if [ -z "$ABELIAN_NON_HETEROGENEOUS" ]; then
      PFLAGS=" -pset=$1 -num_nodes=1"
    else
      PFLAGS=""
    fi
    PFLAGS+=$FLAGS
    if [[ ($1 == *"gc"*) || ($1 == *"cg"*) ]]; then
      PFLAGS+=" -scalegpu=3"
    fi
    rm -f output_*.log

    echo "GALOIS_DO_NOT_BIND_THREADS=1 $MPI -n=$2 ${EXEC} ${INPUT} -t=$3 ${option} ${PFLAGS} ${CUTTYPE} -verify" >>$LOG
    eval "GALOIS_DO_NOT_BIND_THREADS=1 $MPI -n=$2 ${EXEC} ${INPUT} -t=$3 ${option} ${PFLAGS} ${CUTTYPE} -verify" >>$LOG 2>&1

    eval "sort -nu output_${hostname}_*.log -o output_${hostname}_0.log"
    eval "python $checker -t=0.01 $OUTPUT output_${hostname}_0.log &> .output_diff"

    cat .output_diff >> $LOG
    if ! grep -q "SUCCESS" .output_diff ; then
      let fail=fail+1
      if [ $partition -eq 1 ]; then
        failed_cases+="outgoing edge-cut $1 devices with $3 threads; "
      elif [ $partition -eq 2 ]; then
        failed_cases+="incoming edge-cut $1 devices with $3 threads; "
      elif [ $partition -eq 3 ]; then
        failed_cases+="cartesian outgoing vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 4 ]; then
        failed_cases+="cartesian incoming vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 5 ]; then
        failed_cases+="hybrid outgoing vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 6 ]; then
        failed_cases+="hybrid incoming vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 7 ]; then
        failed_cases+="fennel outgoing edge-cut $1 devices with $3 threads; "
      elif [ $partition -eq 8 ]; then
        failed_cases+="fennel incoming edge-cut $1 devices with $3 threads; "
      elif [ $partition -eq 9 ]; then
        failed_cases+="ginger outgoing vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 10 ]; then
        failed_cases+="ginger incoming vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 11 ]; then
        failed_cases+="sugar outgoing vertex-cut $1 devices with $3 threads; "
      elif [ $partition -eq 12 ]; then
        failed_cases+="sugar outgoing vertex-cut $1 devices with $3 threads; "
      fi
    else
      let pass=pass+1
    fi
    rm .output_diff
    IFS=$old_ifs
  done
done

rm -f output_*.log

echo "---------------------------------------------------------------------------------------"
echo "Algorithm: " $execname
echo "Input: " $inputname
echo "Runtime option: " $option
echo $pass "passed test cases"
if [[ $fail == 0 ]] ; then
  echo "Status: SUCCESS"
else
  echo $fail "failed test cases:" $failed_cases
  echo "Status: FAILED"
fi
echo "---------------------------------------------------------------------------------------"

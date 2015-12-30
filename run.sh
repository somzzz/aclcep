#!/bin/bash
source $(dirname $0)/help.sh

if [ "$1" == "serial" ]
then
	cd serial;
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi

if [ "$1" == "pthreads" ]
then
	cd pthreads;
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi

if [ "$1" == "openmp" ]
then
	cd openmp;
	files_to_delete;
	delete_scripts_tests;
	make;
	#qsub -cwd -q all.q run.sh;
	qsub -cwd -q ibm-nehalem12.q run.sh;
	#echo $2 $3 $4
	#qsub -cwd -q ibm-opteron.q run.sh;
	cd ..;
fi

if [ "$1" == "mpi" ]
then
	cd mpi;
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi

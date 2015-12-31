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
	echo "Choose queue for submit the job";
	qstat -g c | cut -d ' ' -f1;
	read queue;
	qsub -cwd -q ${queue} -v image="$2",threads=$3,chunck=$4 run.sh;
	#echo -e "\t${queue}" >> creioane.txt;
	cd ..;
fi

if [ "$1" == "mpi" ]
then
	cd mpi;
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi

#!/bin/bash

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
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi

if [ "$1" == "mpi" ]
then
	cd mpi;
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi

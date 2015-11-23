#!/bin/bash

if [ "$1" == "serial" ]
then
	cd serial;
	make clean; make;
	qsub -cwd -q all.q run.sh;
	cd ..;
fi


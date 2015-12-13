#!/bin/bash
module load compilers/solarisstudio-12.3
collect mpirun -np 4 ./img_seg ../test/img1.pgm

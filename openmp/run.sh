#!/bin/bash
module load compilers/solarisstudio-12.3
collect ./img_seg ${image} ${threads} ${chunck}

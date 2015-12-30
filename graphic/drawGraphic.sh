#!/bin/bash
echo "Read input data file: name_picture.txt"
read input
echo "Read output data file, graphic name: name_picture.png"
read output
echo "Read name(header) of graphic"
read name
echo "Generating graphic..."
gnuplot -e "inputfile='${input}'; outputfile='${output}'; namegraphic='${name}'" GraphicGenerator.gnuplot

#!usr/bin/gnuplot
set terminal png
set output outputfile
set xlabel 'Num_Threads (number)'
set ylabel 'Time (sec)'
set title namegraphic
plot inputfile using 3:6 with lines title 'OpenMP'
set output
quit 

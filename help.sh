#!/bin/bash

files_to_delete () {
	#echo "For make"
	vars=$(find . -type f \( -name "img_seg" -o -name "*pgm" -o -name "*ppm" \));
	array=($vars);
	count=0;
	nr_elements=${#array[@]};
	end=$(($nr_elements - 1));
	for i in $(seq 0 $end)
	do
		if [[ ${array[$i]} == "./binary.pgm" ]] || [[ ${array[$i]} == "./connected.ppm" ]] || [[ ${array[$i]} == "./img_seg" ]]
		then
			((count++));
		fi
	done
	if [ $count -eq 3 ]
	then
		make clean;
	fi
}

delete_scripts_tests () {
	#echo "For test"
	scripts=$(find . -type f \( -name "run.sh.*" \));
	array_scripts=($scripts);
	tests=$(find . -type d \( -name "test.[0-9]*.er" \));
	array_tests=($tests);
	#echo "Test to delete: $tests"
	if [ ${#array_scripts[@]} -gt 0 ]
	then
		rm run.sh.*
	fi
	if [ ${#array_tests[@]} -gt 0 ]
	then
		#echo "delete tests"
		rm -r test.[0-9]*.er
	fi
}

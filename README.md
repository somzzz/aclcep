# aclcep - Image Segmentation
Un proiect grozav.

## IMPORTANT!
Va rog sa stergeti de pe git fisierele .ppm si .pgm, precum si orice executabil. Thx! :)
* am adaugat in .gitignore reguli ca fisierele .ppm si .pgm sa nu mai fie incluse in
commit by default
* daca vreti sa adaugati fisiere .pgm sau .ppm oricum => este un link in fisierul
.gitignore la o pagina de StackOverflow care indica cum sa faceti asta
* adaugati orice fisiere .pgm sau .ppm noi in directorul test

## Description
Exploring various parallel solutions for an image segmentation algorithm.

Original project can be found at:
https://github.com/hanchang/Image-Segmentation

## Make
Enter dir and run `make`

## Run and make on cluster
Use the run.sh script in the root dir:

`./run.sh 'version'`

where 'version' = serial/pthreads/mpi/openmp

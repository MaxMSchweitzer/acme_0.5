#!/bin/bash

#	Chad Coates
#	ECE 373
# Homework #6
# May 16, 2017

# bash script for executing the
# homework #6 tasks

BIN=dumb
LOOPS=3

make
make clean
make install

for((i=0;i<LOOPS;++i))
do
	if [ -a ${BIN} ];then
		printf "\n${BIN} iteration %d\n" $i
		./${BIN}
	else
		printf "\nERROR: ${BIN} not found\n\n" 
		break		
	fi
done

printf "\n"

make remove
make cleanall

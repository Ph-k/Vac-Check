#!/bin/bash

# Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)

if [ "$1" = "pipes" ]
then

   # passing all arguments (except the pipe option flag of the bash script to the implementations makefile)
   cd ./Named-Pipes-Src && make "${@:2}"

elif [ "$1" = "sockets" ]
then

   # passing all arguments (except the pipe option flag of the bash script to the implementations makefile)
   cd ./Socket-Src && make "${@:2}"

elif [ "$1" = "both" ]
then

   cd ./Socket-Src && make
   cd ../Named-Pipes-Src && make

elif [ "$1" = "clean" ]
then

   cd ./Socket-Src && make clean
   cd ../Named-Pipes-Src && make clean

else

   echo "Invalid '$1' option. Please choose one of: pipes, sockets"

fi

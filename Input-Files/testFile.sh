#!/bin/bash

# Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)

#This function takes a filename as an argument and checks if the script can work with it
function checkFile(){
	if [ ! -e $1 ]
	then
		echo "Error: $1 does not exist!"
		exit 1
	fi

	if [ ! -f $1 ]
	then
		echo "Error: $1 is not a regular file!"
		exit 1
	fi

	if [ ! -r $1 ]
	then
		echo "Error: I do not have read rights for $1!"
		exit 1
	fi
}

#main body of the script

#Input checking
if [ $# != 4 ]
then
	echo "Error: Invalid number of arguments!"
	exit 1
fi

virusesFile=$1
countriesFile=$2
numLines=$3
duplicatesAllowed=$4

echo "Given parameters: virusesFile=$1 countriesFile=$2 numLines=$3 duplicatesAllowed=$4"

checkFile $virusesFile
viruses=($(cat "$virusesFile"))
virusesCount=${#viruses[@]}

checkFile $countriesFile
countries=($(cat "$countriesFile"))
countriesCount=${#countries[@]}


if (( numLines>10000 && duplicatesAllowed==0 ))
then
	echo "Error: You requested the imposible, creating $numLines lines with only 10000 unique ID's. Please return with duplicatesAllowed=1"
	exit 2
fi

#Redirection of stdout
exec > inputFile.txt
RANDOM=$$

#Creating some duplicate IDs if requested
if [ $duplicatesAllowed == 1 ]
then
	duplicatesCount=$(( ( RANDOM % (numLines/10) ) + 1 )) #duplicate IDs range 1 - numLines/10
else
	duplicatesCount=0
fi

if (( numLines <= 10000 ))
then
	IdCount=$numLines
else
	IdCount=10000
fi

#Creating an arrey of UNIQUE IDs
for ((i=1; i<=$IdCount; i++))
do
	IdArrey[$i]=$i
done

#Shuffling the above arrey
for ((i=1; i<=$IdCount; i++))
do
	#shufling is done by swaping the current index with a randrom one
	index=$(( (RANDOM % IdCount) + 1 ))
	temp=${IdArrey[$i]}
	IdArrey[$i]=${IdArrey[$index]}

	if [ $duplicatesCount == 0 ]
	then
		IdArrey[$index]=$temp
	else
		#If duplicate records are requested, the shuffling proccedure also places the duplicates radromly
		ranProb=$((RANDOM % 2))
		if [ $ranProb == 0 ]
		then
			IdArrey[$index]=$temp
		else
			duplicatesCount=$((duplicatesCount-1))
		fi
	fi
done

#To create radrom strings
letters=(a b c d e f g h i j k l m n o p q r s t u v w x y z)

IFS=; #To remove white spaces when printing arrey

#This loops creates the records
for ((index=1, i=1; i<=$numLines; i++, index++))
do

	if (( index > IdCount ))
	then
		#If we are out of IDs (only when duplicates=1), we cyrcle through the availiable IDs
		index=$(( (RANDOM % IdCount) + 1 ))
	fi
	id=${IdArrey[$index]}

	#Generating first name
	length=$(( (RANDOM % 10) + 3 )); #in range 3-12
	for ((j=0; j<$length; j++))
	do
		fn[j]="${letters[ $(( RANDOM % 25 )) ]}"
	done

	#Generating last name
	length=$(( (RANDOM % 10) + 3 )); #in range 3-12
	for ((j=0; j<$length; j++))
	do
		ln[j]="${letters[ $(( RANDOM % 25 )) ]}"
	done

	#Generating the rest information
	age=$(( (RANDOM % 120) + 1 ))
	virus=${viruses[ $((RANDOM % $virusesCount)) ]}
	country=${countries[ $((RANDOM % $countriesCount)) ]}

	vaccinated=$((RANDOM % 2))
	if [ $vaccinated == 0 ]
	then
		echo "$id ${fn[*]} ${ln[*]} $country $age $virus YES $(( (RANDOM % 30) + 1 ))-$(( (RANDOM % 12) + 1 ))-$(( (RANDOM % 22) + 2000 ))"
	else
		echo "$id ${fn[*]} ${ln[*]} $country $age $virus NO"
	fi

	fn=()
	ln=()
done

exit 0
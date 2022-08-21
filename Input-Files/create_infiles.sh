#!/bin/bash

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
if [ $# != 3 ]
then
	echo "Error: Invalid number of arguments!"
	exit 1
fi

inputFile=$1
input_dir=$2
numFilesPerDirectory=$3

echo "Given parameters: inputFile='$1' input_dir='$2' numFilesPerDirectory=$3"

checkFile $inputFile
lines=($(sed '/^$/d' "$inputFile" | wc -l)) # Counting non empty lines of the file

# Getting the 4th columns of the files, which are the countries. And saving each country only once in the countries arrey
countries=($(awk 'country[$4]++==0{ print $4 }' $inputFile))

echo "There are '${#countries[@]}' distinct countries ('${countries[*]}')"

# Checking if the user has requested to create more files than records
fileFolderRatio=$(expr  ${#countries[@]} '*' $numFilesPerDirectory)
if (( "$fileFolderRatio" > "$lines" ))
then
	echo "Error: you requested the imposible, placing '$fileFolderRatio' records in files with '$lines' reccords"
	exit 3
fi

if [ ! -d $input_dir ]
then
	mkdir -p $input_dir # Creating the data directory only if it does not exist
else
	# If it exist we cannot continue with the allocation of records to files
	echo "Error: directory $input_dir already exists!"
	exit 2
fi

# Creating one directory for each country in the root directort
for country in "${countries[@]}"
do
	mkdir -p "$input_dir/$country"
done

recordCountry=""

# Temporary changing the end of line character
OldIFS=$IFS
IFS=$'\n'

# Sorting the data file by country (-k4) in order to have a faster round robin without an arrey of round robin counters
sortedByCountryInputFile=($(sort -k4 $inputFile))
IFS=$OldIFS

# In the beggining no country has been previously read
lastRecordCountry=""

# For all the records
for line in "${sortedByCountryInputFile[@]}"
do
	# "Getting" the country of the record
	recordArrey=($line)
	recordCountry=${recordArrey[3]}

	# Since the data is sorted by country
	if [ "$lastRecordCountry" == "" ] || [ "$lastRecordCountry" != "$recordCountry" ]
	then
		# We reset the round robin counter only if we come acros a different than the previous country (or in the first itteration)
		fileIndex=0
	else
		# Otherwise we simply incriment the round robin counter...
		fileIndex=$((fileIndex+1))
	fi
	
	# ...While resseting it if it has overflowed the reuquested value
	if (( "$fileIndex" > "$numFilesPerDirectory" ))
	then
		fileIndex=0
	fi
	
	# Creating file name in which the current record will be writen
	countryFileName=$recordCountry-$fileIndex
	
	# Writting the record
	echo $line >> $input_dir/$recordCountry/$countryFileName
	
	# Saving the last read country
	lastRecordCountry=$recordCountry
	
done
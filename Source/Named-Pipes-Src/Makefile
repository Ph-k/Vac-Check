CC = gcc

# Location for the source code which is used from both implimentations
commonSourcePath = ../Common-Src/

# Location for the implimentation source code which uses pipes
sourcePath = ./

# named pipe implimentation objects
travelObjectsLocation = $(sourcePath)travelMonitorSource/
travelObjects = $(travelObjectsLocation)travelMonitor.o $(travelObjectsLocation)virusBloomFilters.o $(travelObjectsLocation)travelController.o $(travelObjectsLocation)countriesRegister.o $(travelObjectsLocation)travelSignalHandlers.o

monitorObjectsLocation = $(sourcePath)monitorSource/
monitorObjects =  $(monitorObjectsLocation)monitor.o $(monitorObjectsLocation)fileReader.o $(monitorObjectsLocation)monitorController.o $(monitorObjectsLocation)monitorSignalHandlers.o $(monitorObjectsLocation)CountryDirectory.o

communicatorObjectLocation = $(sourcePath)
communicatorObject = $(communicatorObjectLocation)Communicator.o

# Common object files
UtilitiesLocation = $(commonSourcePath)Utilities/
UtilitiesObjects = $(UtilitiesLocation)Utilities.o $(UtilitiesLocation)HashFunctions.o $(UtilitiesLocation)LogFileWritter.o

EntitiesLocation = $(commonSourcePath)Entities/
EntitiesObjects = $(EntitiesLocation)Person.o $(EntitiesLocation)Virus.o $(EntitiesLocation)StringDict.o $(EntitiesLocation)Abacus.o

DataStructuresLocation = $(commonSourcePath)DataStructures/
DataStructuresObjects = $(DataStructuresLocation)HashTable.o $(DataStructuresLocation)BitArrey.o $(DataStructuresLocation)BloomFilter.o $(DataStructuresLocation)SkipList.o

utilityObjects = $(communicatorObject) $(DataStructuresObjects) $(EntitiesObjects) $(UtilitiesObjects)

includePaths = -I$(travelObjectsLocation) -I$(monitorObjectsLocation) -I$(communicatorObjectLocation) -I$(UtilitiesLocation) -I$(EntitiesLocation) -I$(DataStructuresLocation)


cflags = -Wall -g3 $(includePaths)

pflafs = -m 5 -b 10 -s 100000 -i ./countriesData

mainProgram = travelMonitor
monitorProgram = monitor

scriptName = create_infiles.sh
scriptLocation = ./

$(mainProgram): $(travelObjects) $(utilityObjects) $(monitorProgram)
	$(CC) $(cflags) $(travelObjects) $(utilityObjects) -o $(mainProgram)

$(monitorProgram): $(monitorObjects) $(utilityObjects)
	$(CC) $(cflags) $(monitorObjects) $(utilityObjects) -o $(monitorProgram)

# mainObjects
$(sourcePath)%.o: $(sourcePath)%.c $(sourcePath)%.h
	$(CC) $(cflags) -c $< -o $@

#travelObjects
$(travelObjectsLocation)%.o: $(travelObjectsLocation)%.c
	$(CC) $(cflags) -c $< -o $@

#travelObjects
$(monitorObjectsLocation)%.o: $(monitorObjectsLocation)%.c
	$(CC) $(cflags) -c $< -o $@

#Entities
$(EntitiesLocation)%.o: $(EntitiesLocation)%.c $(EntitiesLocation)%.h
	$(CC) $(cflags) -c $< -o $@

#Utilities
$(DataStructuresLocation)%.o: $(DataStructuresLocation)%.c $(DataStructuresLocation)%.h
	$(CC) $(cflags) -c $< -o $@

#Utilities
$(UtilitiesLocation)%.o: $(UtilitiesLocation)%.c $(UtilitiesLocation)%.h
	$(CC) $(cflags) -c $< -o $@

run: $(mainProgram)
	./$(mainProgram) $(pflafs)

val: $(mainProgram)
	valgrind --trace-children=yes --track-origins=yes --show-leak-kinds=all --leak-check=full ./$(mainProgram) $(pflafs)

gdb: $(mainProgram)
	gdb ./$(mainProgram)

countryFolders: $(scriptLocation)$(scriptName)
	cd $(scriptLocation); ./$(scriptName) inputFile.txt countriesData 5

clean:
	rm -f $(travelObjects) $(utilityObjects) $(monitorProgram) $(monitorObjects) $(mainProgram)

cleanFifos:
	rm -f ./Fifos/*

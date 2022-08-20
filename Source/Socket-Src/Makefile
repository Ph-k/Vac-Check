CC = gcc
cflags = -Wall -g3

includePaths = -I$(travelObjectsLocation) -I$(monitorObjectsLocation) -I$(communicatorObjectLocation) -I$(UtilitiesLocation) -I$(EntitiesLocation) -I$(DataStructuresLocation)

# Location for the implimentation source code which uses pipes
sourcePath = ./Named-Pipes-Src/

# Location for the source code which is used from both implimentations
commonSourcePath = ./Common-Src/

# named pipe implimentation objects
travelObjectsLocation = $(sourcePath)travelMonitorSource/
travelObjects = $(travelObjectsLocation)travelMonitor.o $(travelObjectsLocation)virusBloomFilters.o $(travelObjectsLocation)travelController.o $(travelObjectsLocation)countriesRegister.o

monitorObjectsLocation = $(sourcePath)monitorSource/
monitorObjects =  $(monitorObjectsLocation)monitor.o $(monitorObjectsLocation)fileReader.o $(monitorObjectsLocation)monitorController.o $(monitorObjectsLocation)CountryDirectory.o

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

pflafs = -m 5 -b 10 -c 4 -s 100000 -i ./countriesData -t 4

mainProgram = travelMonitorClient
monitorProgram = monitorServer

scriptName = create_infiles.sh
scriptLocation = ./

$(mainProgram): $(travelObjects) $(commonObjects) $(monitorProgram)
	$(CC) $(cflags) $(travelObjects) $(commonObjects) -o $(mainProgram)

$(monitorProgram): $(monitorObjects) $(commonObjects)
	$(CC) $(cflags) $(monitorObjects) $(commonObjects) -o $(monitorProgram) -lpthread


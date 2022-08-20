# mainObjects
$(sourcePath)%.o: $(sourcePath)%.c $(sourcePath)%.h
	$(CC) $(cflags) -c $< -o $@

#travelObjects
$(travelObjectsLocation)%.o: $(travelObjectsLocation)%.c $(travelObjectsLocation)%.h
	$(CC) $(cflags) -c $< -o $@

#travelObjects
$(monitorObjectsLocation)%.o: $(monitorObjectsLocation)%.c $(monitorObjectsLocation)%.h
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
	rm -f $(travelObjects) $(commonObjects) $(monitorProgram) $(monitorObjects) $(mainProgram)

cleanFifos:
	rm -f ./Fifos/*

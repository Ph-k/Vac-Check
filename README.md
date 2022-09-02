# Vac-Check

A distributed vaccination status monitor written in C which uses pipes *or* sockets.

*The project was the programing assignment of the Systems Programming at DIT – UoA.*

## Project description:

The Vac-Check (**Vac**cination **Check**er) project is a distributed tool which performs queries regarding the vaccination status of citizens for a plethora of viruses using multiple data structures. For that reason, the root process of the project, Vac-Check spawns monitor processes with which it communicates.

#### What does Vac-Check *(the parent process)* do:

On a high level, the parent process is given a hierarchy of directories each representing a country, each directory consists of several text files containing vaccination information about a plethora of citizens and viruses. Then the parent process spawns monitor process to which it distributes the directories so that each child process is responsible only for a unique number of countries. Afterwards, the parent process waits for queries from the user, which are executed with the help of the monitor child process responsible for that country.

Note that to maximize efficiency and minimize the number of queries that need to be sent and executed from the monitors. Vac-Check also maintains [bloom filters](https://en.wikipedia.org/wiki/Bloom_filter) for each virus, created from the monitor processes.

#### What do the monitor *(child processes)* do:

As mentioned, the monitor processes are responsible for holding the vaccination records of a plethora of citizens for several o countries. The information of the records is organized by viruses and saved in two [skip lists](https://en.wikipedia.org/wiki/Skip_list), one for vaccinated citizens and one for unvaccinated citizens. The organization of the viruses (and their respective skip lists) is done using hash tables.

### Inter process communication & different implementations of the project:

As you will quickly notice below, there are **two** different implementations of Vac-Check. The difference between the two is how the inter-process communication is performed, and how the program is executed concurrently.

**The first implementation** uses named pipes to communicate with the parent. The initialization of the program from the input directories is done concurrently from the monitors.

**The second implementation** uses sockets to communicate with the parent. The initialization of the program from the input directories is done concurrently from the monitors. And the initialization of the data from the files is done in parallel from the threads of each monitor process.


# Project compilation

In the Source directory you will find the `compile.sh` bash script which can compile the implementation of your choice by executing its makefile. The resulting executables `VacCheck` and `VacCheck_monitor` will be found in the implementations source directory *(`Named-Pipes-Src` or ` Socket-Src `)*.

`compile.sh` usage:
- `./compile.sh pipes` to compile the pipe implementation.
- `./compile.sh sockets` to compile the sockets implementation.
- `./compile.sh both` to compile the both of the implementations.
- `./compile.sh clean` to remove all the object and executable files for both implementations.

*The program was tested on Debian based linux distros, but all linux/unix machines running the gcc compiler are supported.*


# VacCheck usage

## Pipe implementation usage

The program can be executed from a cli as `./VacCheck –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir` where:

- VacCheck: is the executable of the project

- numMonitors: the number of monitor processes to be spawned and used.

- bufferSize: the size of the buffer that will be used to send information through the pipes.

- sizeOfBloom: sets the size of the bloomfilter *in bytes*.

- input_dir: is the directory which contains the input files hierarchy split in countries.

The input_dir must have the following form:
```
input_dir 
└───Greece
│   │   Greece-1.txt
│   │   Greece-2.txt
|
└───France
│   │   France-1.txt
│   │   France-2.txt
│   │   France-3.txt
│   
└───Germany
|   │   Germany-1.txt
|   │   Germany-2.txt
|   │   Germany-3.txt
```

## Socket implementation usage
The program can be executed from a cli as `./VacCheck –m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads` where:

- VacCheck: is the executable of the project

- numMonitors: the number of monitor processes to be spawned and used.

- socketBufferSize: the size of the buffer that will be used to send information through the sockets.

- cyclicBufferSize: The size of a buffer *in entries, ex 10 file names* which is used from the threads to read which input files they will read.

- sizeOfBloom: sets the size of the bloomfilter *in bytes*.

- numThreads: the number of threads that will be created.

- input_dir: is the directory which contains the input files hierarchy split in countries.

# Program interface usage
The program provides a tui to the user allowing him to interact with the information of the students. The tui instructions are the following:

1. `/travelRequest citizenID date countryFrom countryTo virusName`:  Checks if the citizen with citizenID  has been vaccinated for the virusName virus, and outputs this information. This is done by first checking the bloom filter. If the bloom filter cannot assure us about the vaccination status of the citizen, VacCheck asks the monitor process responsible for the countryFrom country through pipes or sockets for the vaccination status and prints the current message *(note, a  citizen is vaccinated if he has been vaccinated for less than 6 months before his travel date)* 

2. `/travelStats virusName date1 date2 [country]`: Outputs the total, accepted, and rejected number of vaccination-check requests between date1 and date2 for virusName. If a country is given, information about only this specific country is printed, otherwise the program prints statistics for all countries. *Date is in DD-MM-YYYY format*.

3. `/addVaccinationRecords country`: This command can be used when the user has added an input file in the countries folder *(input_dir/country)*. The parent/VacCheck process informs the children/monitor process about the addition of a file using a USR1 signal, then the monitor updates its data structures and returns to the parent its updated bloom filter using pipe/socket.

4. ` /searchVaccinationStatus citizenID`: Outputs all the vaccination information *(for which viruses he is or isn’t vaccinated)* the program has for the requested citizen. This is done from the parent/VacCheck process by sending to all the monitor processes via pipes/sockets this request, and then the monitor process responsible for the citizen with citizedID sends back via pipe/socket all the vaccination information it has about him.   

5. `/printPids`: prints the process id’s of all the child/monitor processes.
```
| not the same for both impl
V
````
6. ‘/exit’: send a signal to all the child/monitor processes which when received makes all the mointors to write some diagnostic information in their log files, de-allocate all the dynamically allocated memory, and then terminate. After all the monitor processes have terminated gracefully, VacCheck also de-allocates all its dynamically allocated memory and exits. 

7. `/violentlyExit`: The parent/VacCheck process send a sig kill signal to all the child/monitor processes. NOTE: USE ONLY WHEN ABSOLITY NESSECERY, it is recommended that you always use the exit command. 



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

# Vac-Check

The Vac-Check project is a distributed tool which performs queries regarding the vaccination status of citizens for a plethora of viruses. For that reason,  the root process of the project, `vacCheck` spawns `monitor` processes with which it communicates.  

*The project was the programing assignment of the Systems Programming at DIT – UoA.*

## Different implementations of Vac-Check
As you will quickly notice, there are two different implementations of Vac-Check. The difference between the two is how the inter-process communication is performed. The first implementation uses named pipes and the second uses sockets.

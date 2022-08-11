# Vac-Check

The Vac-Check project consists of the distributed tool `travelMonitor`. This tool performs queries regarding the vaccination status of citizens for a plethora of viruses. For that reason, travelsMonitor spawns `monitor` processes with which it communicates. The difference between the two implementations you will find here is the inter-process communication. The first implementation uses pipes and the second uses sockets. 

*The project was the programing assignment of the Systems Programming at DIT – UoA.*

#default: ddclient_local

ddclient_local: ddclient.o netpptk.o
	g++ -Wall -pedantic -g -O2 -lboost_thread-mt ddclient.o netpptk.o -o ../../built/ddclient

ddclient.o: ddclient.cpp
	g++ -Wall -pedantic -g -O2 -c ddclient.cpp -o ddclient.o

netpptk.o: ../lib/netpptk.h ../lib/netpptk.cpp
	g++ -Wall -pedantic -g -O2 -c ../lib/netpptk.cpp -o netpptk.o 
	

ddclient_clean:
	-rm -f ../../built/ddclient ddclient.o netpptk.o

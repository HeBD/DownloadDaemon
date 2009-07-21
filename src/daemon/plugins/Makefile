#default: plugins_local

plugins_local: rapidshare_com.o
	g++ -Wall -pedantic -lcurl -g -O2 rapidshare_com.o -o ../../../built/plugins/rapidshare.com


rapidshare_com.o: rapidshare_com.cpp
	g++ -Wall -pedantic -g -O2 -c rapidshare_com.cpp -o rapidshare_com.o

plugins_clean:
	-rm -f rapidshare_com.o ../../../built/plugins/rapidshare.com


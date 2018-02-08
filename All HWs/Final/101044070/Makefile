all:
	gcc -c server.c uici.c uiciname.c restart.c
	gcc -o server server.o uici.o uiciname.o restart.o -lpthread
	gcc -c clients.c uici.c uiciname.c restart.c
	gcc -o clients clients.o uici.o uiciname.o restart.o -lpthread
clean:
	rm *.o *~
	
	


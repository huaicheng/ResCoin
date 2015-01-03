CC = gcc
CFLAGS = -lvirt -lm -g

all: ResCoin gmmagent

# the memory gmmagent
gmmagent: 
	$(CC) -o gmmagent gmmagent.c 

# the main program
ResCoin: main.o rc.o common.o monitor.o schedule.o ringbuffer.o 
	$(CC) -o ResCoin main.o  schedule.o rc.o  ringbuffer.o  monitor.o  common.o $(CFLAGS)


schedule.o: schedule.c schedule.h ringbuffer.h monitor.h rc.h
	$(CC) -c schedule.c  $(CFLAGS)

rc.o: common.h rc.c rc.h
	$(CC) -c rc.c $(CFLAGS)

common.o: common.c common.h
	$(CC) -c common.c $(CFLAGS)

monitor.o: monitor.c monitor.h
	$(CC) -c monitor.c $(CFLAGS)

main.o: main.c ringbuffer.h ewma.h monitor.h schedule.h 
	$(CC) -c  main.c  $(CFLAGS)

ringbuffer.o: ringbuffer.c ringbuffer.h monitor.h
	$(CC) -c ringbuffer.c $(CFLAGS)

clean:
	rm -f test *.o 

dist-clean:
	rm -f test *.o gmmagent ResCoin

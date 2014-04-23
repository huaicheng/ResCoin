test: lvtest.o  rescon.o
	gcc -o test lvtest.o rescon.o -lvirt -g

lvtest.o: lvtest.c 
	gcc -c lvtest.c  -g

rescon.o: rescon.c rescon.h
	gcc -c rescon.c -g

clean:
	rm test *.o -f

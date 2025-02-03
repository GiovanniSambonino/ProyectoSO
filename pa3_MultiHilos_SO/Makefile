GCC = gcc
CFLAGS = -Wall -Wshadow
OBJS = ex5.o bmp.o filter.o

ex5: $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o $@

.c.o: 
	$(GCC) $(CFLAGS) -c $*.c 

clean:
	rm *.o ex5

test: ex5
	./ex5 testcases/test.bmp test_out.bmp
	diff test_out.bmp testcases/test_sol.bmp
	
testmem: ex5
	valgrind --tool=memcheck --leak-check=summary ./ex5 testcases/test.bmp test_out.bmp	

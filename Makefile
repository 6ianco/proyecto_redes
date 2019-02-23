CFLAGS = -g -Wall -Werror -pthread

TARGETS =  server qsub qstat qdel

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) core* *.o

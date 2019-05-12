######################################################################
#  Mastering the Raspberry Pi - ISBN13: 978-1-484201-82-4
#  Warren W. Gay VE3WWG
######################################################################

CC	= gcc
OPTS	= -Wall
DBG	= -O0 -g
CFLAGS	= $(OPTS) $(DBG)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $*.o

OBJS=dht22.o

all:	$(OBJS)
	$(CC) $(OBJS) -o dht22
	sudo chown root ./dht22
	sudo chmod u+s ./dht22

clean:
	rm -f *.o core errs.t dht22

clobber: clean
	rm -f dht22

dht22.o: dht22.c gpio_io.c timed_wait.c

######################################################################
#  End Makefile. Public domain license.
######################################################################

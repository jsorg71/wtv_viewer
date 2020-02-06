
OBJS=wtv_calls.o wtv_pa.o wtv_xcb.o

CFLAGS=-O2 -g -Wall -Wextra -fvisibility=hidden -fPIC

LDFLAGS=

LIBS=

wtv.a: $(OBJS)
	$(AR) rvu wtv.a $(OBJS)
	ranlib wtv.a

clean:
	rm -f wtv.a $(OBJS)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $<


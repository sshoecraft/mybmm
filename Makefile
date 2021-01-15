
PROG=mybmm
MODULES=can.c can_ip.c serial.c ip.c bt.c
SRCS=main.c display.c config.c db.c module.c inverter.c pack.c si.c jbd.c preh.c worker.c uuid.c list.c utils.c cfg.c conv.c $(MODULES)
OBJS=$(SRCS:.c=.o)
#LIBS+=-lsqlite3 -lgattlib -lglib-2.0 -pthread -ldl
LIBS+=-ldl -lgattlib -lglib-2.0 -lpthread -lsqlite3
#CFLAGS=-DMYBMM
#CFLAGS+=-Wall -O2 -pipe
CFLAGS+=-Wall -g -DDEBUG
LDFLAGS+=-rdynamic

all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): Makefile

gendb_bms.c:
	gendb -f siu -i id -n name bms

#include ../Makefile.dep

debug: $(PROG)
	gdb $(PROG)

install: $(PROG)
	install -m 755 -o bin -g bin $(NAME) /usr/sbin

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull

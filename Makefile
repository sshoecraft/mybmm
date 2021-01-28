
BLUETOOTH=yes
MQTT=yes

PROG=$(shell basename $(shell pwd))
INVERTERS=si.c
CELLMONS=jbd.c preh.c jk.c
TRANSPORTS=can.c can_ip.c dsfuncs.c serial.c ip.c bt.c
ifneq ($(BLUETOOTH),yes)
_TMPVAR := $(TRANSPORTS)
TRANSPORTS = $(filter-out bt.c, $(_TMPVAR))
endif
UTILS=worker.c uuid.c list.c utils.c cfg.c
SRCS=main.c config.c db.c module.c inverter.c pack.c battery.c parson.c $(INVERTERS) $(CELLMONS) $(TRANSPORTS) $(UTILS)
LIBS+=-ldl -lpthread
#CFLAGS=-DMYBMM
#CFLAGS+=-Wall -O2 -pipe
CFLAGS+=-Wall -g -DDEBUG=1
LDFLAGS+=-rdynamic
ifeq ($(BLUETOOTH),yes)
CFLAGS+=-DBLUETOOTH
LIBS+=-lgattlib -lglib-2.0 -lpthread
endif
ifeq ($(MQTT),yes)
SRCS+=mqtt.c
CFLAGS+=-DMQTT
LIBS+=-lpaho-mqtt3c
endif
OBJS=$(SRCS:.c=.o)

#export MQTT_C_CLIENT_TRACE=ON

.PHONY: all install
all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): Makefile

gendb_bms.c:
	gendb -f siu -i id -n name bms

include Makefile.dep

debug: $(PROG)
	gdb $(PROG)

install: $(PROG)
	install -m 755 -o bin -g bin $(PROG) /usr/local/bin

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull

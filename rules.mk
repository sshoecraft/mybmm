
LIBS+=-ldl

PI=$(shell test $$(cat /proc/cpuinfo  | grep ^model | grep -c ARM) -gt 0 && echo yes)

# Debug
ifeq ($(DEBUG),yes)
CFLAGS+=-Wall -g -DDEBUG=1
else
CFLAGS+=-Wall -O2 -pipe
endif

# Bluetooth
ifeq ($(BLUETOOTH),yes)
SRC+=bt.c
CFLAGS+=-DBLUETOOTH
ifeq ($(PI),yes)
LIBS+=./libgattlib.a -lglib-2.0
DEPS=./libgattlib.a
else
LIBS+=-lgattlib -lglib-2.0
endif
endif

# MQTT
ifeq ($(MQTT),yes)
SRCS+=mqtt.c
CFLAGS+=-DMQTT
ifeq ($(PI),yes)
LIBS+=./libpaho-mqtt3c.a
DEPS=./libpaho-mqtt3c.a
else
LIBS+=-lpaho-mqtt3c
endif
endif

OBJS=$(SRCS:.c=.o)
LIBS+=-lpthread
LDFLAGS+=-rdynamic

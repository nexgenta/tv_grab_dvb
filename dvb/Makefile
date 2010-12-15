TARGET_OUT = libdvb.a
TARGET_OBJ = platforms.o multiplexes.o services.o events.o networks.o \
	si.o pat.o sdt.o nit.o demux.o read.o crc32.o
TARGET_COMMON_DEPS = dvb.h p_dvb.h callbacks.h si_tables.h \
	platforms.h multiplexes.h services.h events.h networks.h

CFLAGS = -W -Wall -g

## Static library rules

all: $(TARGET_OUT)

clean:
	rm -f $(TARGET_OUT) $(TARGET_OBJ)

$(TARGET_OUT): $(TARGET_OBJ)
	$(AR) rcs $@ $+

## Dependencies

dvb-si.h: dvb-si.idl gentables
	./gentables < dvb-si.idl > dvb-si.h

gentables: gentables.o

platforms.o: platforms.c platforms.h
multiplexes.o: multiplexes.c multiplexes.h platforms.h
services.o: services.c services.h
events.o: events.c events.h services.h
networks.o: networks.c networks.h services.h
si.o: si.c $(TARGET_COMMON_DEPS)
pat.o: pat.c $(TARGET_COMMON_DEPS)
sdt.o: sdt.c $(TARGET_COMMON_DEPS)
nit.o: nit.c $(TARGET_COMMON_DEPS)
demux.o: demux.c $(TARGET_COMMON_DEPS)
read.o: read.c $(TARGET_COMMON_DEPS)
crc32.o: crc32.c $(TARGET_COMMON_DEPS)

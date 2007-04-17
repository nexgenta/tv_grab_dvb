#Perhaps you want a line like this instead. I've not used autoconf yet
#CFLAGS=-Wall -O2 -I/usr/src/dvb-kernel/linux/include/
CFLAGS=-Wall -O2

tv_grab_dvb:	tv_grab_dvb.o crc32.o lookup.o dvb_info_tables.o

tv_grab_dvb.o:  si_tables.h tv_grab_dvb.h

lookup.o:	tv_grab_dvb.h

dvb_info_tables.o: tv_grab_dvb.h

clean:
	rm *.o tv_grab_dvb

#Perhaps you want a line like this instead. I've not used autoconf yet
#CFLAGS=-Wall -O2 -I/usr/src/dvb-kernel/linux/include/
CFLAGS=-Wall -O2

tv_grab_dvb:	tv_grab_dvb.o crc32.o lookup.o
	gcc -o tv_grab_dvb tv_grab_dvb.o crc32.o lookup.o

tv_grab_dvb.o:  dvb_info_tables.h si_tables.h

lookup.o:	lookup.h

clean:
	rm *.o tv_grab_dvb

	

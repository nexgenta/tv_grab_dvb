CFLAGS=-Wall -O2

tv_grab_dvb:	tv_grab_dvb.o crc32.o lookup.o
	gcc -o tv_grab_dvb tv_grab_dvb.o crc32.o lookup.o

tv_grab_dvb.o:  dvb_info_tables.h si_tables.h

lookup.o:	lookup.h

clean:
	rm *.o tv_grab_dvb

	

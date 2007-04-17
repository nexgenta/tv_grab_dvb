#!/usr/bin/make -f

#Perhaps you want a line like this instead. I've not used autoconf yet
#CFLAGS=-Wall -O2 -I/usr/src/dvb-kernel/linux/include/
CFLAGS=-Wall -O0 -g

tv_grab_dvb:	tv_grab_dvb.o crc32.o lookup.o dvb_info_tables.o
tv_grab_dvb.o:  tv_grab_dvb.h si_tables.h
lookup.o:	tv_grab_dvb.h
dvb_info_tables.o:	tv_grab_dvb.h

tags: $(wildcard *.[ch])
	ctags $^

.PHONY: clean
clean:
	$(RM) *.o tv_grab_dvb

.PHONY: distclean
distclean: clean
	$(RM) tags

.PHONY: tar
tar: $(PWD)
	tar -c -f ../$(<F).tar.gz -z -h -C .. -v \
		--exclude=*.o --exclude=tags --exclude=.gdbinit \
		--exclude=tv_grab_dvb \
		--exclude=test --exclude=.*.swp \
		--exclude=*~ --exclude=*.bak --exclude=*.orig \
		$(<F)

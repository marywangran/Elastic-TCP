obj-m += tcp_elastic.o

all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

clean:
	rm -f *.ko .*.o.cmd *.o *.mod.c Module.symvers .*.mod.cmd modules.order .*.ko.cmd *.mod

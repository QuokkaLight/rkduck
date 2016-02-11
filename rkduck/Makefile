obj-m = rkduck.o
rkduck-objs = backdoor.o duck.o hijack.o misc.o persistence.o

KERNEL = $(shell uname -r)
PWD = $(shell pwd)

all: 
	make -C /lib/modules/$(KERNEL)/build M=$(PWD) modules
clean: 
	make -C /lib/modules/$(KERNEL)/build M=$(PWD) clean

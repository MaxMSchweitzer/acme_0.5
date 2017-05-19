
#	Chad Coates
#	ECE 373
# Homework #6
# May 16, 2017
#
# supercool makefile
#

HW=6
HOMEWORK=hw$(HW).sh

KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD :=$(shell pwd)
KO=acme.ko

CC=gcc
USER=ninja
USR_CFLAGS= -O0 -g -Wall -pthread 
FLAGS= -lpci -lz
DEPS=
SRCS=usr_acme.c
OBJS = $(SRCS:.c=.o)
obj-m += acme.o

default: $(KO) $(USER)

ko: $(KO)
	@echo $(KO) compiled

$(KO): 
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

user: $(USER)
	@echo $(USER) compiled

$(USER): $(OBJS) $(DEPS)
	$(CC) $(USR_CFLAGS) -o $(USER) $(OBJS) $(FLAGS)

.c.o: 
	$(CC) $(USR_CFLAGS) -c $?  -o $@

clean: 
	rm -f *.o

cleanall: 
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
	rm -f *.o $(USER)

install:
	sudo insmod ./$(KO)

remove:
	sudo rmmod $(KO)

hw6:
	sudo ./$(HOMEWORK)
	

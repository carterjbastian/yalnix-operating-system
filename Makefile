#
#	Sample Makefile for Yalnix kernel and user programs.
#	
#	Prepared by Sean Smith and Adam Salem and various Yalnix developers
#	of years past...
#
#	You must modify the KERNEL_SRCS and KERNEL_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix kernel.
#
#	You must modify the USER_SRCS and USER_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix user programs
#
#	The Yalnix kernel built will be named "yalnix".  *ALL* kernel
#	Makefiles for this lab must have a "yalnix" rule in them, and
#	must produce a kernel executable named "yalnix" -- we will run
#	your Makefile and will grade the resulting executable
#	named "yalnix".
#

#make all will make all the kernel objects and user objects
ALL = $(KERNEL_ALL) $(USER_APPS)
KERNEL_ALL = yalnix
SRCDIR = /media/sf_ringo/src
USRDIR = /media/sf_ringo/usr_progs
TESTDIR = /media/sf_ringo/test

#List all kernel source files here.  
KERNEL_SRCS = $(SRCDIR)/kernel.c $(SRCDIR)/PCB.c $(SRCDIR)/linked_list.c \
	      $(SRCDIR)/traps.c $(SRCDIR)/load_program.c $(SRCDIR)/syscalls.c \
	      $(SRCDIR)/blocks.c

#List the objects to be formed form the kernel source files here.  Should be the same as the prvious list, replacing ".c" with ".o" 
KERNEL_OBJS = $(SRCDIR)/kernel.o $(SRCDIR)/PCB.o $(SRCDIR)/linked_list.o \
	      $(SRCDIR)/traps.o $(SRCDIR)/load_program.o $(SRCDIR)/syscalls.o \
	      $(SRCDIR)/blocks.o

#List all of the header files necessary for your kernel
KERNEL_INCS = $(SRCDIR)/kernel.h $(SRCDIR)/PCB.h $(SRCDIR)/linked_list.h $(SRCDIR)/traps.h \
	      $(SRCDIR)/syscalls.h $(SRCDIR)/blocks.h $(SRCDIR)/cvar.h $(SRCDIR)/pipe.h \
	      $(SRCDIR)/lock.h $(SRCDIR)/tty.h



#List all user programs here.
USER_APPS = $(USRDIR)/init $(USRDIR)/simple_getpid $(USRDIR)/delay $(USRDIR)/brk \
	    $(USRDIR)/fork $(USRDIR)/new_prog $(USRDIR)/exec $(USRDIR)/exit \
	    $(USRDIR)/fatal_errors $(USRDIR)/tty $(USRDIR)/locks_cvars $(USRDIR)/wait_short \
	    $(USRDIR)/wait_long $(USRDIR)/pipe $(TESTDIR)/forktest $(TESTDIR)/torture \
		$(TESTDIR)/bigstack $(TESTDIR)/zero $(USRDIR)/pipes

#List all user program source files here.  SHould be the same as the previous list, with ".c" added to each file
USER_SRCS = $(USRDIR)/init.c $(USRDIR)/simple_getpid.c $(USRDIR)/delay.c $(USRDIR)/brk.c \
	    $(USRDIR)/fork.c $(USRDIR)/new_prog.c $(USRDIR)/exec.c $(USRDIR)/exit.c \
	    $(USRDIR)/fatal_errors.c $(USRDIR)/tty.c $(USRDIR)/locks_cvars.c $(USRDIR)/wait_short.c \
	    $(USRDIR)/wait_long.c $(USRDIR)/pipe.c $(TESTDIR)/forktest.c $(TESTDIR)/torture.c \
		$(TESTDIR)/bigstack.c $(TESTDIR)/zero.c $(USRDIR)/pipes.c

#List the objects to be formed form the user  source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
USER_OBJS = $(USRDIR)/init.o $(USRDIR)/simple_getpid.o $(USRDIR)/delay.o $(USRDIR)/brk.o \
	    $(USRDIR)/fork.o $(USRDIR)/new_prog.o $(USRDIR)/exec.o $(USRDIR)/exit.o \
	    $(USRDIR)/fatal_errors.o $(USRDIR)/tty.o $(USRDIR)/locks_cvars.o \
	    $(USRDIR)/wait_short.o $(USRDIR)/wait_long.o $(USRDIR)/pipe.o $(TESTDIR)/forktest.o \
		$(TESTDIR)/torture.o $(TESTDIR)/bigstack.o $(TESTDIR)/zero.o $(USRDIR)/pipes.o

#List all of the header files necessary for your user programs
USER_INCS = 

#write to output program yalnix
YALNIX_OUTPUT = yalnix

#
#	These definitions affect how your kernel is compiled and linked.
#       The kernel requires -DLINUX, to 
#	to add something like -g here, that's OK.
#

#Set additional parameters.  Students generally should not have to change this next section

#Use the gcc compiler for compiling and linking
CC = gcc

DDIR58 = /yalnix
LIBDIR = $(DDIR58)/lib
INCDIR = $(DDIR58)/include
ETCDIR = $(DDIR58)/etc

# any extra loading flags...
LD_EXTRA = 

KERNEL_LIBS = $(LIBDIR)/libkernel.a $(LIBDIR)/libhardware.so

# the "kernel.x" argument tells the loader to use the memory layout
# in the kernel.x file..
KERNEL_LDFLAGS = $(LD_EXTRA) -L$(LIBDIR) -lkernel -lelf -Wl,-T,$(ETCDIR)/kernel.x -Wl,-R$(LIBDIR) -lhardware
LINK_KERNEL = $(LINK.c)

#
#	These definitions affect how your Yalnix user programs are
#	compiled and linked.  Use these flags *only* when linking a
#	Yalnix user program.
#

USER_LIBS = $(LIBDIR)/libuser.a
ASFLAGS = -D__ASM__
CPPFLAGS= -m32 -fno-builtin -I. -I$(INCDIR) -g -DLINUX 


##########################
#Targets for different makes
# all: make all changed components (default)
# clean: remove all output (.o files, temp files, LOG files, TRACE, and yalnix)
# count: count and give info on source files
# list: list all c files and header files in current directory
# kill: close tty windows.  Useful if program crashes without closing tty windows.
# $(KERNEL_ALL): compile and link kernel files
# $(USER_ALL): compile and link user files
# %.o: %.c: rules for setting up dependencies.  Don't use this directly
# %: %.o: rules for setting up dependencies.  Don't use this directly

all: $(ALL)	

clean:
	rm -f *.o *~ TTYLOG* TRACE $(YALNIX_OUTPUT) $(USER_APPS) $(USER_OBJS)  core.*
	rm -f ./src/*.o ./src/syscalls/*.o

count:
	wc $(KERNEL_SRCS) $(USER_SRCS)

list:
	ls -l *.c *.h

kill:
	killall yalnixtty yalnixnet yalnix

no-core:
	rm -f core.*

$(KERNEL_ALL): $(KERNEL_OBJS) $(KERNEL_LIBS) $(KERNEL_INCS)
	$(LINK_KERNEL) -o $@ $(KERNEL_OBJS) $(KERNEL_LDFLAGS)

$(USER_APPS): $(USER_OBJS) $(USER_INCS)
	$(ETCDIR)/yuserbuild.sh $@ $(DDIR58) $@.o











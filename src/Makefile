#This makefile is usually called twice per build. When invoked by the command
#line it notices KERNELRELEASE variable not set, so it locates kernel source 
#directory by the symbolic link 'build' in the installed modules directory,
#which points back to the kernel build tree.

#If you're not actually running the kernel that you're building for, you supply
#KERNELDIR= option on the command line, set the KERNELDIR environment variable,
#or rewrite the line that sets KERNELDIR.

#Once the source tree has been found, the makefile inovkes the default: target,
#which runs the second command parameterized as $(MAKE) to invoke the kernel
#build system.

#On the second reading, the makefile sets obj-m, and the kernel makefile takes
#care of actually building the module.

#If KERNELRELEASE is defined, we've been invoked from the kernel build system 
#and can use its language.

RM  := rm -f
RMR := rm -rf

ifneq ($(KERNELRELEASE),)
	obj-m := read_write.o


#Otherwise we were called directly from the command line; 
#Invoke the kernel build system.
else
	KERNELDIR=/lib/modules/$(shell uname -r)/build
	PWD=$(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(RM)  *.o *~ core .depend .*.*cmd *ko *.mod.c
	$(RM)  Module.markers Module.symvers modules.order
	$(RMR) .tmp_versions Modules.symvers  		
endif


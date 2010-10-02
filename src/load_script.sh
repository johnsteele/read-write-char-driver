#!/bin/sh

#This is the name the driver registered within the __init function.
#It is then placed in the /proc/devices file under char devices.
module="read_write_module" 

#This is the name of the device (special) file that will be placed 
#in the /dev/ directory. Depending on your Makefile, it's usually
#the name of the .c driver file.
device="read_write"

#Readable/Writable to owner, readable to others.
mode="664"

#Group: Since distributions do it differently, look for wheel or use staff.
if grep -q '^staff' /etc/group; then
	group="staff"
else
	group="wheel"
fi

#Invoke insmod will all arguments we got
#and use a pathname, as insmod doesn't look in '.' by default.
/sbin/insmod ./$device.ko $* || exit 1

#Retrieve the major from /proc/devices, the name of the device
#is that which was registered in the __init function of the driver.
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)
echo ${major}
echo ${group}
#Remove any stale nodes.
rm -f /dev/${device} /dev/${device}0

#Make the device nodes (special files).
mknod /dev/${device}0 c $major 0

chgrp $group /dev/${device}0
chmod $mode  /dev/${device}0


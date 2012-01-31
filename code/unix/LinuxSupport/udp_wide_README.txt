UDP Wide Broadcast Patch for Kernel 2.4.19

Main purpose is to allow Quake 3 servers, and all games powered by the
engine, to be visible in the server browser when the servers are being run on
aliased IP addresses and multiple NICs using the +set net_ip option.

To apply the patch run "patch -p1 < udp_wide_broadcast.patch" from your
source directory and recompile.

Add "echo "1" > /proc/sys/net/ipv4/udp_wide_broadcast" to one of your startup
scripts, i.e./etc/rc.d/rc.local, and run thousands of servers from your
computer.


Patch by:
Rogier Mulhujzen <linux@bsdchicks.com>
John Tobin <ogre@sirinet.net>

A patch with the same functionality for FreeBSD is available from
http://www.bsdchicks.com/patches

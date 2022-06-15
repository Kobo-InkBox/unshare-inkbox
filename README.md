# unshare
Since the default `unshare` implementations of BusyBox or util-linux didn't work for some namespaces (the PID one, namely) on Kobo & Kindle, I discovered that using `clone()` instead of `unshare()` worked just fine on these devices. Thus, I went ahead and wrote my own implementation of it.
## Usage
```
./unshare: Run specified process in specified namespace(s).
Usage: ./unshare [arguments] -- [[<absolute path to executable>] [<executable arguments>]]
Arguments:
	-m:		   mounts namespace
	-u:		   UTS namespace
	-i:		   IPC namespace
	-n:		   network namespace
	-p:		   PID namespace
	-U:		   user namespace
	-P [<mountpoint>]: mount proc filesystem at mountpoint before executing process (chroot only)
	--help:		   Displays this help
```
## Proof-of-Concept/Examples
```
kindle:/# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 16436 qdisc noqueue state UNKNOWN 
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
2: usb0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP qlen 1000
    link/ether 96:c9:62:bf:a6:e9 brd ff:ff:ff:ff:ff:ff
    inet 192.168.3.2/24 brd 192.168.3.255 scope global usb0
kindle:/# echo $$
2667
kindle:/# unshare -p -n /bin/bash
kindle:/# ip a
5: lo: <LOOPBACK> mtu 16436 qdisc noop state DOWN 
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
kindle:/# echo $$
1
kindle:/#
```

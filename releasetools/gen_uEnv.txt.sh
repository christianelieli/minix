#!/bin/sh

#generate a u-boot u-env.
#The kernel is configured to start and run at 0x80200000
#but we use the u-boot uImage format to allow to use the
#bootm command. To ensure we can still boot "in place"
#we load the kernel 0x40 bellow 0x80200000. This allows
#us to "eXecute in place" XIP.
list="0x801fffc0 uImage
0x82000000 ds.elf
0x82800000 rs.elf
0x83000000 pm.elf
0x83800000 sched.elf
0x84000000 vfs.elf
0x84800000 memory.elf
0x85000000 log.elf
0x85800000 tty.elf
0x86000000 mfs.elf
0x86800000 vm.elf
0x87000000 pfs.elf
0x87800000 init.elf"

#
# PREFIX for loading file over tftp to allow hosting multiple
# version/devices.
NETBOOT_PREFIX=""
NETBOOT="no"
BOOT="mmcbootcmd"

#default for the beagleboard-xM
CONSOLE=tty02 
#verbosity
VERBOSE=3 
HZ=1000

while getopts "c:v:h:p:n?" c
do
        case "$c" in
        \?)
                echo "Usage: $0 [-p netboot_prefix] -n [-c consoletty] [-v level] " >&2
                exit 1
        	;;
        n)
		# genrate netbooting uEnv.txt
                BOOT="netbootcmd"
                NETBOOT="yes"
		;;
        p)
                NETBOOT_PREFIX=$OPTARG
		;;
        c)
                CONSOLE=$OPTARG
		;;
        v)
                VERBOSE=$OPTARG
		;;
        h)
                # system hz
		HZ=$OPTARG
		;;
	esac
done

fill_cmd() {
	#load == load method like fatload mmc 0:1
	#prefix is an optional directory containing the ending /
	load=$1
	prefix=$2
	export IFS=" " 
	echo $list | while true
	do
		if ! read -r mem addr
		then
			break
		fi
		#e.g. ; fatloat mmc 0:1 0x82000000 mydir/ds.elf
		echo -n "; $load $mem $prefix$addr"
	done
}


echo "# Set the command to be executed"
echo "uenvcmd=run $BOOT"
echo "bootargs=console=$CONSOLE rootdevname=c0d0p1 verbose=$VERBOSE hz=$HZ"
echo
echo "bootminix=setenv bootargs \$bootargs board_name=\$board_name ; echo \$bootargs ;  bootm  0x801fffc0"
echo 
echo "mmcbootcmd=echo starting from MMC ; mmc part 0; $(fill_cmd "fatload mmc 0:1" "") ; dcache off ; icache off ; run bootminix"
echo 
echo "# Netbooting."
echo "serverip=192.168.12.10"
echo "ipaddr=192.168.12.62"
echo "usbnet_devaddr=e8:03:9a:24:f9:10"
echo "usbethaddr=e8:03:9a:24:f9:11"
echo "netbootcmd=echo starting from TFTP;  $(fill_cmd "tftp" "$NETBOOT_PREFIX") ; dcache off ; icache off ; run bootminix"
exit 0

while getopts "c:v:?" c
do
        case "$c" in
        \?)
                exit 1
                ;;
        c)
                # genrate netbooting uEnv.txt
                CONSOLE=$OPTARG
		;;
        v)
                # genrate netbooting uEnv.txt
                VERBOSE=$OPTARG
		;;
        h)
                # system hz
		HZ=$OPTARG
		;;
        esac
done



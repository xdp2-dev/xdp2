#! /bin/bash

function die()
{
	echo $1
	exit -1
}

basedir=$(dirname $0)

usage()
{
	echo -e "Usage: $0 [--stop_on_error] [--enable_randpkt_tests] [--enable_disabled_tests] \n"
	exit 0
}

stop_on_error="no"
enable_randpkt_tests="no"
enable_disabled_tests="no"

while [ -n "$1" ]; do
	case $1 in
		"--stop_on_error")
			stop_on_error="yes";
			echo "stop_on_error enabled";;
		"--enable_randpkt_tests")
			enable_randpkt_tests="yes";
			echo "enable_randpkt_tests enabled";;
		"--enable_disabled_tests")
			enable_disabled_tests="yes";
			echo "enable_disabled_tests enabled";;
		*) usage $PROGNAME >&2; exit 1;;
	esac
	shift
done

echo "running flowdisector kernel parser basic validation tests"
#flowdis tests
$basedir/test_parser -i raw,$basedir/test-in.raw -c flowdis -o text | \
	grep -v Dumping | diff -u $basedir/test-out-flowdis.raw -
$basedir/test_parser -i pcap,$basedir/test-in.pcap -c flowdis -o text | \
	grep -v Dumping | diff -u $basedir/test-out-flowdis.pcap -
$basedir/test_parser -i tcpdump,$basedir/test-in.tcpdump -c flowdis -o text | \
	grep -v Dumping | diff -u $basedir/test-out-flowdis.tcpdump -
$basedir/test_parser -i fuzz -c flowdis -o text < $basedir/test-in.fuzz | \
	grep -v Dumping | diff -u $basedir/test-out-flowdis.fuzz -

echo "running xdp2 parser basic validation tests"
#xdp2 tests
$basedir/test_parser -i raw,$basedir/test-in.raw -c xdp2 -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.raw -
$basedir/test_parser -i pcap,$basedir/test-in.pcap -c xdp2 -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.pcap -
$basedir/test_parser -i tcpdump,$basedir/test-in.tcpdump -c xdp2 -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.tcpdump -
$basedir/test_parser -i fuzz -c xdp2 -o text < $basedir/test-in.fuzz | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.fuzz -

echo "running xdp2 optimized parser basic validation tests"
#xdp2 optimized tests
$basedir/test_parser -i raw,$basedir/test-in.raw -c xdp2opt -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.raw -
$basedir/test_parser -i pcap,$basedir/test-in.pcap -c xdp2opt -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.pcap -
$basedir/test_parser -i tcpdump,$basedir/test-in.tcpdump -c xdp2opt -o text \
	 | grep -v Dumping | diff -u $basedir/test-out-xdp2.tcpdump -
$basedir/test_parser -i fuzz -c xdp2opt -o text < $basedir/test-in.fuzz | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.fuzz -

arch=$(uname -m)
if [[ "$arch" != *"riscv"* ]]; then
	exit 0
fi
echo "running xdp2 asm big parser basic validation tests"
#xdp2 asm bigparser tests
$basedir/test_parser -i raw,$basedir/test-in.raw -c ins32 -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.raw -
$basedir/test_parser -i pcap,$basedir/test-in.pcap -c ins32 -o text | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.pcap -
$basedir/test_parser -i tcpdump,$basedir/test-in.tcpdump -c ins32 -o text | \
	 grep -v Dumping | diff -u $basedir/test-out-xdp2.tcpdump -
$basedir/test_parser -i fuzz -c ins32 -o text < $basedir/test-in.fuzz | \
	grep -v Dumping | diff -u $basedir/test-out-xdp2.fuzz -

echo "running xdp2 v/s ins32 core validation tests"
#more xdp2 asm bigparser tests and validate them against core xdp2 parser
declare -A skip_list
if [ "$enable_disabled_tests" = "no" ]; then
	skip_list[gre-pptp.pcap]="'Core xdp2 segfaults'"
	skip_list[PPTP_negotiation.cap]="'Core xdp2 segfaults'"
	skip_list[l2tp.pcap]="'Core xdp2 can not parse it'"
	skip_list[mpls-twolevel.cap]="'Core xdp2 can not parse it'"
fi

# NOTE: following comment out tests actually work fine, hence enabled.

if [ "$enable_randpkt_tests" = "no" ]; then
	skip_list[randpkt_arp.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_bgp.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_bvlc.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_dns.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_eth.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_fddi.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_giop.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_icmp.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_ieee802.15.4.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_ip.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_ipv6.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_llc.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_m2m.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_megaco.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_nbns.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_ncp2222.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_rand.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_sctp.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_syslog.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_tcp.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_tds.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_tr.pcap]="'--enable_randpkt_tests is not set.'"
#	skip_list[randpkt_udp.pcap]="'--enable_randpkt_tests is not set.'"
	skip_list[randpkt_usb-linux.pcap]="'--enable_randpkt_tests is not set.'"
fi

pcapfilesdir="$basedir/../data/pcaps/"

if [ ! -d "$pcapfilesdir" ]; then
	echo "$pcapfilesdir is missing, trying dev. setup path."
	pcapfilesdir="$basedir/../../../data/pcaps/"
	if [ ! -d "$pcapfilesdir" ]; then
		die "$pcapfilesdir is missing, further tests can't run!\
		    Use install_data\install_all\install_all_tar options."
	fi
fi

echo "pcaps dir: $pcapfilesdir"

for filename in $pcapfilesdir/*; do
	f=$(basename "$filename")
	if [[ ! " ${!skip_list[@]} " =~ " ${f}" ]]; then
		echo "Testing $f"
		f=$(realpath $filename)
		if [ "$stop_on_error" = "no" ]; then
			$basedir/test_parser -c xdp2 -i pcap,$f -o text -H &> p.out \
				|| echo "xdp2 core test for $f failed!"
			$basedir/test_parser -c ins32 -i pcap,$f -o text -H &> i.out \
				|| echo "ins32 core test for $f failed!"
			diff -q p.out i.out \
				|| echo "FAILURE: o/p of xdp2 and ins32 core are not same for $f !"
		else
			$basedir/test_parser -c xdp2 -i pcap,$f -o text -H &> p.out \
				|| die "xdp2 core test for $f failed!"
			$basedir/test_parser -c ins32 -i pcap,$f -o text -H &> i.out \
				|| die "ins32 core test for $f failed!"
			diff -q p.out i.out \
				|| die "FAILURE: o/p of xdp2 and ins32 core are not same for $f !"
		fi
	else
		echo "pcap $f is disabled due to bug ${skip_list[$f]}, skipped!"
	fi
done
rm p.out i.out

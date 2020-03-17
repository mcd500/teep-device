#!/bin/bash -eu

ETHER_IF=`ip link | awk -F: '$0 !~ "lo|vir|wl|pp|docker|br|^[^0-9]"{print $2;getline}'`

for ETHER in $ETHER_IF; do
myip=`ifconfig $ETHER | awk '/inet / {gsub("addr:","",$2); print $2}'`
done

if [ -z "$myip" ]; then echo Could not obtain IP address; exit 1;fi

echo $myip


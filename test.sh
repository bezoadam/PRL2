#!/bin/bash
if [ $# -eq 2 ]; then
    numbers=$1
    number_process=$2
else
    numbers=5
    number_process=5
fi

mpic++ --prefix /usr/local/share/OpenMPI -o mes mss.cpp
dd if=/dev/urandom bs=1 count=$numbers of=numbers > /dev/null 2>&1
mpirun -q --prefix /usr/local/share/OpenMPI -np $number_process mss
rm -f mss numbers

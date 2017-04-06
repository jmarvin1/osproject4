#!/bin/sh
for x in 2 10 20 30 40 50 60 70 80 90 100
do
	echo "rand w/ sort $x">>results.txt
	./virtmem 100 $x rand sort >>results.txt 2>&1
done

for y in 1 10 20 30 40 50 60 70 80 90 100
do
	echo "rand w/ focus $y">> results.txt
	./virtmem 100 $y rand focus >>results.txt 2>&1
done

for z in 1 10 20 30 40 50 60 70 80 90 100
do 
	echo "rand w/ focus $z" >> results.txt
	./virtmem 100 $z rand scan >>results.txt 2>&1
done  

for a in 2 10 20 30 40 50 60 70 80 90 100
do
	echo "fifo w/ sort $a" >>results.txt
	./virtmem 100 $a fifo sort >>results.txt 2>&1
done

for a in 1 10 20 30 40 50 60 70 80 90 100
do 
	echo "fifo w/ scan $a" >>results.txt
	./virtmem 100 $a fifo scan >>results.txt 2>&1
done
for a in 1 10 20 30 40 50 60 70 80 90 100
do
        echo "fifo w/ focus $a" >>results.txt
        ./virtmem 100 $a fifo focus >>results.txt 2>&1
done
for a in 1 10 20 30 40 50 60 70 80 90 100
do
        echo "custom  w/ scan $a" >>results.txt
        ./virtmem 100 $a custom scan >>results.txt 2>&1
done
for a in 2 10 20 30 40 50 60 70 80 90 100
do
        echo "custom  w/ sort  $a" >>results.txt
        ./virtmem 100 $a custom sort >>results.txt 2>&1
done
for a in 1 10 20 30 40 50 60 70 80 90 100
do
        echo "fifo w/ focus $a" >>results.txt
        ./virtmem 100 $a custom focus >>results.txt 2>&1
done

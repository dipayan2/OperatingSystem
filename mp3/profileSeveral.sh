a=1
b=1

while [ $a -le 20 ]
	do
	echo $a

	sudo mknod node c 248 0

	while [ $b -le $a ]
	do
	nice ./work 200 R 10000 &
	b=`expr $b + 1`
	done

	wait
   	sudo ./monitor > profile3_$a.data
	sudo rm node

   	a=`expr $a + 1`
   	b=1
done
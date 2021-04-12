sudo mknod node c 248 0
nice ./work 1024 R 50000 & nice ./work 1024 R 10000 &
wait
cat /proc/mp3/status
pkill work
sudo ./monitor > profile1.dat
sudo rm node

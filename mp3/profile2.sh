nice ./work 1024 R 50000 & nice ./work 1024 L 10000 &
wait
cat /proc/mp3/status
pkill work
sudo ./monitor > profile2.dat
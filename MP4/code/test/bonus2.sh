make clean
make
../build.linux/nachos -f
../build.linux/nachos -cp 1KB.txt /test1
../build.linux/nachos -cp 60KB.txt /test2
../build.linux/nachos -cp 120KB.txt /test3

echo -e "\n content of 1KB textfile\n"
../build.linux/nachos -p /test1
echo -e "\n content of 30KB textfile\n"
../build.linux/nachos -p /test2
echo -e "\n content of 120KB textfile\n"
../build.linux/nachos -p /test3
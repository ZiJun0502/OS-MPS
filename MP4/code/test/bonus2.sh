make clean
make
../build.linux/nachos -f
../build.linux/nachos -cp 1KB.txt /test1
../build.linux/nachos -cp 4MB.txt /test2
../build.linux/nachos -cp 120KB.txt /test3

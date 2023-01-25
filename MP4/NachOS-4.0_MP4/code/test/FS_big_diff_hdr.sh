../build.linux/nachos -f
../build.linux/nachos -mkdir /dir1
../build.linux/nachos -cp num_100.txt /num1.txt
../build.linux/nachos -mkdir /dir2
../build.linux/nachos -l /
../build.linux/nachos -cp num_1000.txt /dir1/num2.txt
../build.linux/nachos -lr /
../build.linux/nachos -mkdir /dir2/big
../build.linux/nachos -cp num_64MB.txt /dir2/big/num64.txt # takes some time...
../build.linux/nachos -lr /
echo "=======================[print and redirect file content to log files]======================="
../build.linux/nachos -p /num1.txt > logp100.log
../build.linux/nachos -p /dir1/num2.txt > logp1000.log
../build.linux/nachos -p /dir2/big/num64.txt > logp64.log
echo "=======================[print and redirect file header structure to log files]======================="
../build.linux/nachos -ps /num1.txt > logps100.log
../build.linux/nachos -ps /dir1/num2.txt > logps1000.log
../build.linux/nachos -ps /dir2/big/num64.txt > logps64.log
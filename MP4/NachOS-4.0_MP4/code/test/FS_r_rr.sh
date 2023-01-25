../build.linux/nachos -f
../build.linux/nachos -mkdir /mydir
../build.linux/nachos -lr /
../build.linux/nachos -cp num_1000.txt /num1.txt
../build.linux/nachos -cp num_1000.txt /num2.txt
../build.linux/nachos -cp num_1000.txt /num3.txt
../build.linux/nachos -cp num_1000.txt /num4.txt
../build.linux/nachos -cp num_1000.txt /mydir/num0.txt
../build.linux/nachos -cp num_1000.txt /mydir/num1.txt
../build.linux/nachos -cp num_1000.txt /mydir/num2.txt
../build.linux/nachos -lr /
../build.linux/nachos -mkdir /mydir/sub1
../build.linux/nachos -mkdir /mydir/sub2
../build.linux/nachos -cp num_1000.txt /mydir/num3.txt
../build.linux/nachos -cp num_1000.txt /mydir/num4.txt
../build.linux/nachos -cp num_1000.txt /mydir/sub1/num.txt
../build.linux/nachos -cp num_1000.txt /mydir/sub2/num.txt
../build.linux/nachos -lr /
../build.linux/nachos -mkdir /mydir/sub1/sub
../build.linux/nachos -mkdir /mydir/sub1/tmp
../build.linux/nachos -mkdir /mydir/sub2/tmp
../build.linux/nachos -cp num_1000.txt /mydir/sub1/sub/num.txt
../build.linux/nachos -lr /
echo "=======================[Remove: /mydir/num4.txt]======================="
../build.linux/nachos -r /mydir/num4.txt
../build.linux/nachos -lr /
echo "=======================[Remove: /mydir/sub2/tmp]======================="
../build.linux/nachos -rr /mydir/sub2/tmp
../build.linux/nachos -lr /
echo "=======================[Remove: /mydir/sub1]======================="
../build.linux/nachos -rr /mydir/sub1
../build.linux/nachos -lr /
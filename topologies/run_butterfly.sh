node 5000 > out0.txt &
node 5001 > out1.txt &
node 5002 > out2.txt &
node 5003 > out3.txt &
node 5004 > out4.txt &
node 5005 > out5.txt &
node 5006 > out6.txt &
node 5007 > out7.txt &
node 5008 > out8.txt &
node 5009 > out9.txt &

./net_sim -s 5 -C localhost:5000:5009 butterfly.dat

exit 0

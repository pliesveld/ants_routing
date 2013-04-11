node 5000 > out0.txt &
node 5001 > out1.txt &
node 5002 > out2.txt &
node 5003 > out3.txt &
node 5004 > out4.txt &
node 5005 > out5.txt &
node 5006 > out6.txt &
net_sim -C localhost:5000:5006 test7201.dat

exit 0
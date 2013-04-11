node 5000 > out0.txt &
node 5001 > out1.txt &
node 5002 > out2.txt &
node 5003 > out3.txt &
net_sim -C localhost:5000:5003 -n mesh.dat
exit 0
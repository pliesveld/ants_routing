all:idlparser nodeinterface.cpp rpcnode
idlparser:idlparse.cpp idlparse.h idltypes.h 
	g++ -o idlparser driver.cpp idlparse.cpp
nodeinterface.cpp: 
	./idlparser dlsim.idl
	sleep 2
nodeinterface.o:idlparser nodeinterface.cpp nodeinterface.h
	g++ -g -c nodeinterface.cpp
rpcnode:main_rpcnode.cpp simnode.o nodeinterface.o
	g++ -g -o rpcnode main_rpcnode.cpp antnode.o simnode.o nodeinterface.o 
simnode.o:antnode.o simnode.cpp simnode.h
	g++ -g -c simnode.cpp
antnode.o:antnode.cpp antnode.h
	g++ -g -c antnode.cpp

clean:
	rm -rf node simnode.o nodeinterface.o idlparser nodeinterface.cpp nodeinterface.h


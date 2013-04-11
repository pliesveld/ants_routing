#!/bin/sh
FILES=`ls node.*`
GRAPH=graphs/node


COLOR[0]="black"
COLOR[1]="red"
COLOR[2]="blue"
COLOR[3]="yellow"
COLOR[4]="orange"
COLOR[5]="teal"
COLOR[6]="purple"
COLOR[7]="magenta"
COLOR[8]="skyblue"
COLOR[9]="brightgreen"


for FILE in $FILES ; do
	NODE=`grep DATA\ NODE\: $FILE | cut -d \  -f 3`

	if [[ $NODE == '' ]] ;then
		break
	fi
	echo "parsing node $NODE"

	XLOC=0
	YLOC=0
	PORT=0

	while [[ PORT -lt 10 ]]; do
		FILE_NAME=graphs/node.$NODE.$PORT
		grep DATA\ NODE$PORT $FILE | cut -d \  -f 3- > $FILE_NAME
		
		DATA=`cat $FILE_NAME`

		if [[ $DATA == '' ]]; then
			rm $FILE_NAME
			#(( PORT=$PORT+1 ))
			#continue
			break
		fi

		XAXIS=`cat $FILE_NAME | wc -l | cut -d \  -f 1`
		NPORTS=`head -n 1 $FILE_NAME | wc -w`
		echo $NPORTS
		
		(( PORT=$PORT+1 ))
	done
	
	GRAPH_FILE=$GRAPH.$NODE
	echo "" > $GRAPH_FILE

	Y_PORT=0


	while [[ Y_PORT -lt $PORT ]]; do

	echo "#proc getdata" >> $GRAPH_FILE
	echo "file: /home/ants/graphs/node.$NODE.$Y_PORT" >> $GRAPH_FILE

	echo "#proc areadef" >> $GRAPH_FILE
	echo "title: NODE $NODE DESTINATION $Y_PORT" >> $GRAPH_FILE
	echo "xrange: 0 $XAXIS" >> $GRAPH_FILE

	echo -n "yautorange: datafield=1" >> $GRAPH_FILE

	I_PORT=2
	while [[ $I_PORT -lt $PORT ]]; do
		echo -n ",$I_PORT" >> $GRAPH_FILE
		((I_PORT++))
	done
	echo "" >> $GRAPH_FILE
	echo "box: 1.8 1.3" >> $GRAPH_FILE
	echo "location: $XLOC.5 $YLOC.5" >> $GRAPH_FILE

	if [[ $YLOC -lt 5 ]]; then
		((YLOC=YLOC + 2))
	else
		((XLOC=XLOC + 2))
		YLOC=0
	fi

	echo "#proc xaxis" >> $GRAPH_FILE
	echo "stubs: inc 50" >> $GRAPH_FILE
	echo "minorticinc: 10" >> $GRAPH_FILE


	echo "#proc yaxis" >> $GRAPH_FILE
	echo "stubs: inc 10" >> $GRAPH_FILE
	echo "minorticinc: 5" >> $GRAPH_FILE


	#grep ANTSTAT $FILE 

	I_PORT=1
	while [[ $I_PORT -le $NPORTS ]]; do
		echo "#proc lineplot" >> $GRAPH_FILE
		echo "yfield: $I_PORT" >> $GRAPH_FILE 
		echo "linedetails: color=${COLOR[$I_PORT]} width=0.5" >> $GRAPH_FILE
		((I_PORT++))
	done

	((Y_PORT++))
	done

done

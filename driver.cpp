#include <iostream>
#include <string>
#include "idlparse.h"

using namespace std;

int main(int argc, char** argv)
{

	// Correct usage test
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " <IDL File>" << endl;
		return 0;
	}

	IDLParser myParser;

	// Read the file
	if (myParser.readfile(argv[1]) == 1)
	{
		cerr << "Parsing failed!" << endl;
		return 1;
	}

	// Write the prototypes
	myParser.writefile("nodeinterface.idl");

	return 0;
}

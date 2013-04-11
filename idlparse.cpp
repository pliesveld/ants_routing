#include <list>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include "idlparse.h"

// Stuff to add:
// 2. Use Read and Write, in these places:
//		a) Eliminating send_return() entirely
//		b) Where send() is currently used in stubs.
// 3. Remove #include <socket>
// 4. Remove insocket and outsocket variables
// 5. Put delete[] on a new line :-P


using namespace std;

ostream& operator<< (ostream& os, const IDLtype& tp)
{
	switch(tp)
	{	
		case INT32:
			return  os << "int32";
			break;
		case INT16: 
			return os << "int16";
			break;
		case BYTEARRAY:
			return os << "bytearray";
			break;
		case VOID:
			return os << "";
			break;
		case INVALID:
			return os << "INVALID";
    }
    return os << "";
}

// overloaded input
istream& operator>> (istream& is, IDLtype& tp)
{
	// Read in
	string buff;
	getline(is, buff);
	
	// Determine type
	if (buff == "int32")
		tp = INT32;
	else if (buff == "int16")
		tp = INT16;
	else if (buff == "bytearray")
		tp = BYTEARRAY;
	else if (buff == "void")
		tp = VOID;
	else
	{
		tp = INVALID;
		is.clear(ios::badbit);
	}

	return is;
}

IDLParser::IDLParser()
{
}

int IDLParser::readfile(string fname)
{
	ifstream infile(fname.c_str());
	if (!infile)
	{
		cerr << "Cannot open " << fname << endl;
		return 1;
	}

	// Clean out the old, if there is anything
	inbound_procs.clear();
	outbound_procs.clear();

	procedure new_proc;
	variable new_var;
    

	// Loop through the file until it's done.
	string inputline;
	istringstream ss;		// To convert str to int, etc.
 	bool inbound_call = false;
	while (true)
	{		
		// Get the input.
		infile >> inputline;

		// End of file reached 
		if (infile.eof())
			break;

		// Check for comments, then ignore to end of line
		if (inputline[0] == '/')
		{
			string inbound_call_comment = "Inbound call";

			getline(infile, inputline);
			if(inputline.size() > 12)
			if(!inputline.compare(1, 12, inbound_call_comment))
			{
				cout << "inputline" << inputline << std::endl;
				inbound_call = true;
			}
			continue;
		}
		// Handle procedures
		if (inputline != "proc")
		{
			cerr << "Syntax error: Proc should be first line" << endl;
			return 1;
		}

		// Clear any old vars
		new_proc.vars.clear();
		
		infile >> new_proc.name;
		infile >> inputline; infile >> inputline;
		ss.str(inputline);
		ss.clear();
		ss >> new_proc.code;
		cout << "Found procedure " << new_proc.name << " with value " << new_proc.code << endl;

		// Handle takes
		infile >> inputline;
		if (inputline != "takes")
		{
			cerr << "Syntax error: takes should be second line" << endl;
			return 1;
		}

		infile >> inputline;
		while(inputline != "returns")
		{
			// Get the type
			ss.str(inputline);
			ss.clear();
			ss >> new_var.type;
			
			// Get a name
			if (new_var.type != VOID)
                		infile >> new_var.name;
			else
				new_var.name = "";
            
			// Insert the new variable
			cout << "  Found variable " << new_var.type << " " << new_var.name << endl;
			new_proc.vars.push_back(new_var);

			// Acquire next line.
			infile >> inputline;
		}

		// Handle takes
		if (inputline != "returns")
		{
			cerr << "Syntax error: returns should be third line" << endl;
			return 1;
		}

		// Get return type
		infile >> inputline;
		ss.str(inputline);
		ss.clear();
		ss >> new_proc.ret;
				
		// Insert procedure.
		if(inbound_call)
			inbound_procs.push_back(new_proc);
		else
			outbound_procs.push_back(new_proc);
		inbound_call = false;
	}		

	return 0;
}

int IDLParser::writefile(string fname)
{
	list<procedure> procs;
	procs.insert(procs.begin(),inbound_procs.begin(), inbound_procs.end());
	procs.insert(procs.end(),outbound_procs.begin(), outbound_procs.end());

// Drop the extension, if there is one
	string::size_type dot_loc = fname.find_last_of(".");
	if (dot_loc != string::npos)
		fname.erase(dot_loc);
	
	// Section for generating the header file
	ofstream outfile((fname + ".h").c_str());

	// Initial comments and header files
	outfile << "// GENERATED HEADER FILE" << endl;
	outfile << "#ifndef H_NODEINTERFACE\n#define H_NODEINTRFACE\n";
	outfile << "#include \"idltypes.h\"\n\n" << endl;

	// enum for the types of error values
	outfile << "enum calltype\n{\n\t";
	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		if (i != inbound_procs.begin())
			outfile << ",\n\t";
		outfile << "PROC_" << i->name << " = " << i->code;
	}
	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		//if (i != outbound_procs.begin())
			outfile << ",\n\t";
		outfile << "PROC_" << i->name << " = " << i->code;
	}
	outfile << "\n};\n\n" << endl;
    
	// enum for dataDirection
	outfile << "enum dataDirection { INBOUND, OUTBOUND };\n\n" << endl;

	outfile << "class RPCOutboundInterface\n{\npublic:\n" << endl;
	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") = 0;" << endl;
	}
	outfile << "\n};\n\n";

	outfile << "class RPCInboundInterface\n{\npublic:\n" << endl;
	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") = 0;" << endl;
	}
	outfile << "\n};\n\n";
	outfile << "class RPCInterface : public virtual RPCOutboundInterface, public virtual RPCInboundInterface { } ; " << endl;

	outfile << "class RPCOutboundStub\n{\npublic:\n" << endl;
	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " stub_" << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") = 0;" << endl;
	}
	outfile << "\n};\n\n";

	outfile << "class RPCInboundStub\n{\npublic:\n" << endl;
	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " stub_" << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") = 0;" << endl;
	}
	outfile << "\n};\n\n";

	outfile << "// " << __FILE__ << ':' << __LINE__ << endl;
	outfile << "class RPCStub : public virtual RPCOutboundStub, public virtual RPCInboundStub { } ; \n";
	outfile << "class RPC : public virtual RPCInterface, public virtual RPCStub { } ; \n\n";

	outfile << "class RPCOutboundImp : public virtual RPCOutboundStub, public virtual RPCStub, public virtual RPCInterface\n{\npublic:\n" << endl;
	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") { return stub_" << i->name << "(";
			for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
			{
				if (j != i->vars.begin())
					outfile << ", ";
				outfile << j->name;
			}
		outfile << "); } "<< endl;
	}

	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " stub_" << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") = 0;" << endl;
	}	outfile << "\n};\n\n";

	outfile << "class RPCInboundImp : public virtual RPCInboundStub, public virtual RPCStub, public virtual RPCInterface\n{\npublic:\n" << endl;
	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}



		outfile << ") { return stub_" << i->name << "(";
			for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
			{
				if (j != i->vars.begin())
					outfile << ", ";
				outfile << j->name;
			}
		outfile << "); } "<< endl;
	}

	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " stub_" << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ") = 0;" << endl;
	}		
	outfile << "\n};\n\n";




	outfile << "// " << __FILE__ << ':' << __LINE__ << endl;
	outfile << "class RPCClientInterface : public virtual RPCInboundImp, public virtual RPC\n{\npublic:\n";

	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ") = 0;" << endl;
	}

	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ") { return 0; }" << endl;
	}


	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " stub_" << i->name << "(";
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ") = 0;" << endl;
	}


	outfile << "\n};\n\n";

	outfile << "// " << __FILE__ << ':' << __LINE__ << endl;
	outfile << "class RPCServerInterface : public virtual RPCOutboundImp, public virtual RPC\n{\npublic:\n";

	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ") { return 0; } " << endl;
	}

	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " " << i->name << "(";
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ") = 0;" << endl;
	}


	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\tvirtual " << i->ret << " stub_" << i->name << "(";
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ") = 0;" << endl;
	}


	outfile << "\n};\n\n";


	// Class and functions
	outfile << "// " << __FILE__ << ':' << __LINE__ << endl;
	outfile << "class NodeInterface : public RPCInboundImp, public RPC\n{\npublic:\n\tvirtual ~NodeInterface() = 0;" << endl;
	outfile << "\n\t// Dispatch loop";
	outfile << "\n\tvirtual void dispatchLoop();" << endl;

	outfile << "\n\t// Functions to be defined in derived classes" << endl;



	
	outfile << "\n\t//Stub functions for calling RPC's; automatically defined in " << fname << ".cpp" << endl;
	for (list<procedure>::iterator i = outbound_procs.begin(); i != outbound_procs.end(); i++)
	{
		outfile << "\t" << i->ret << " stub_" << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ");" << endl;
	}

	for (list<procedure>::iterator i = inbound_procs.begin(); i != inbound_procs.end(); i++)
	{
		outfile << "\t" << i->ret << " stub_" << i->name << "(";
		
		// Iterate through variables for this procedure
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}

		outfile << ");" << endl;
	}
		
	// Virtual marshall and unmarshalls, send and receive.
	outfile << "// " << __FILE__ << ':' << __LINE__ << endl;
	outfile << "\t// Unmarshall and marshall: Must be defined in derived class!" << endl;
	outfile << "protected:\n\tchar* marshal(int32 in, char* out);\n"
			<< "\tchar* marshal(int16 in, char* out);\n"
			<< "\tchar* marshal(bytearray in, char* out);\n" << endl;
	outfile << "\tint32 unmarshal(dataDirection dir, int32* in);\n"
			<< "\tint16 unmarshal(dataDirection dir, int16* in);\n"
			<< "\tvoid unmarshal(dataDirection dir, bytearray* in);\n" << endl;
	outfile << "\tvirtual int Read(dataDirection dir, char* data, int len) = 0;" << endl;
	outfile << "\tvirtual int Send(dataDirection dir, char* data, int len) = 0;" << endl;
	outfile << "};\n#endif\n" << endl;

	// Section for the implementation file
	outfile.close();
	outfile.open((fname + ".cpp").c_str());
	
	// Includes
	outfile << "// GENERATED IMPLEMENTATION FILE\n#include \"idltypes.h\"\n" 
			<< "#include \"" << fname + ".h" << "\"\n" << endl;
	outfile << "#include <stdlib.h>\n#include <string.h>\n#include <netinet/in.h>\n" << endl;


	outfile << "char* NodeInterface::marshal(int32 in, char* out) \n\
{\n\
        int32 a = htonl(in); \n\
        memcpy(out,&a,sizeof(int32)); \n\
        return out + sizeof(int32); \n\
}\n\
\n\
char* NodeInterface::marshal(int16 in, char* out)\n\
{ \n\
        int16 a = htons(in);\n\
        memcpy(out,&a,sizeof(int16));\n\
        return out + sizeof(int16);\n\
}\n\
\n\
char* NodeInterface::marshal(bytearray in, char* out)\n\
{\n\
        char *p = out;\n\
        int32 len;\n\
        len = htonl(in.len);\n\
        memcpy(p,&len,sizeof(int32));\n\
        p = p + sizeof(int32);\n\
        memcpy(p,in.data,in.len);\n\
        p = p + in.len;\n\
        return p;\n\
}" << endl << endl;

	outfile << "int32 NodeInterface::unmarshal(dataDirection dir,int32 *in) \n\
{\n\
        char buf[5];\n\
        Read(dir,buf,sizeof(int32)); \n\
        memcpy(in,buf,sizeof(int32)); \n\
        *in = ntohl(*in);\n\
        return *in;\n\
}\n\
\n\
int16 NodeInterface::unmarshal(dataDirection dir,int16 *in)\n\
{\n\
        char buf[3];\n\
        Read(dir,buf,sizeof(int16));\n\
        memcpy(in,buf,sizeof(int16));\n\
        *in = ntohs(*in);\n\
        return *in;\n\
}\n\
\n\
void NodeInterface::unmarshal(dataDirection dir,bytearray *out)\n\
{\n\
        int32 len;\n\
        unmarshal(dir,&len);\n\
        out->len = len;\n\
\n\
        if(out->data != NULL)\n\
        {\n\
                fprintf(stderr,\"bomb here\\n\");\n\
                exit(1);\n\
        }\n\
        out->data = new char[len];\n\
        Read(dir,(char*)out->data,len);\n\
}" << endl << endl;

		
	// Destructor (default)
	outfile << "NodeInterface::~NodeInterface()\n{\n}\n\n" << endl;
	// Dispatch loop definition
	outfile << "// " << __FILE__ << ':' << __LINE__ << endl;
	outfile << "void NodeInterface::dispatchLoop()\n{" << endl;
	outfile << "\twhile (true)\n\t{" << endl;
	outfile << "\t\tcalltype msg;\n\t\tunmarshal(OUTBOUND, (int32*) &msg);"
			<< "\n\t\tswitch(msg)\n\t\t{" << endl;
	
	// Iterate through all the procedures, write variables, unmarshals
	for (list<procedure>::iterator i = procs.begin(); i != procs.end(); i++)
	{
		outfile << "\t\tcase " << "PROC_" << i->name << ":" << endl;
		outfile << "\t\t\t" << i->ret << " ret" << i->code << ";" << endl;
				
		// Declare and unmarshal variables for this proc.
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j->type != VOID)
			{
				outfile <<"\t\t\t" << j->type << " " << j->name << i->code << ";" << endl;
				if (j->type == BYTEARRAY)
					outfile << "\t\t\t" << j->name << i->code << ".data = 0;" << endl;
				outfile << "\t\t\tunmarshal(OUTBOUND, &" << j->name << i->code << ");" << endl; 
			}
		}

		outfile << "\t\t\tret" << i->code << " = " << i->name << "(";
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			if (j->type != VOID) 
				outfile << j->name << i->code;
		}
		outfile << ");" << endl;
		// Declare buffer, marshall send, delete
		outfile << "\t\t\t" << i->ret << " rbuf" << i->code << ";" << endl;
		//outfile << "\t\t\trbuf" << i->code << " = new char[sizeof(" << i->ret << ")];" << endl;
		outfile << "\t\t\tmarshal(ret" << i->code << ", (char*) &rbuf" << i->code << ");" << endl;
		outfile << "\t\t\tSend(OUTBOUND, (char*) &rbuf" << i->code << ", sizeof(" << i->ret << "));" << endl;
		//outfile << "\t\t\tdelete[] rbuf" << i->code << ";" << endl;
		
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
			if (j->type == BYTEARRAY)
				outfile << "\t\t\tdelete[] " << j->name << i->code << ".data;" << endl;
						
		//outfile << "\t\t\tsend_return(outsocket, ret" << i->code << ");" << endl;
		outfile << "\t\t\tbreak;" << endl;
	}
	outfile << "\t\t}\n\t}\n}\n\n" << endl;
		
	// Stub function definitions
	for (list<procedure>::iterator i = procs.begin(); i != procs.end(); i++)
	{
		outfile << i->ret << " NodeInterface::stub_" << i->name << "(";
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j != i->vars.begin())
				outfile << ", ";
			outfile << j->type << " " << j->name;
		}
		outfile << ")\n{" << endl;
		outfile << "\t" << i->ret << " ret;" << endl;
		outfile << "\tint len = sizeof(int32)";
		
		// Length and declaration of buffer
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
		{
			if (j->type != VOID)
			{
				if (j->type != BYTEARRAY) 
					outfile << " + sizeof(" << j->type << ")";
				else
					outfile << " + sizeof(int32) + " << j->name << ".len";
			}
		}
		outfile << ";\n\n\tchar* buf = new char[len];\n\tchar* p = buf;" << endl;
		
		// Marshal values to send
		outfile << "\tp = marshal((int32) PROC_" << i->name << ",p);\n";
		for (list<variable>::iterator j = i->vars.begin(); j != i->vars.end(); j++)
			if (j->type != VOID)
				outfile << "\tp = marshal(" << j->name << ",p);\n";
		// Send, get return, delete, exit
		outfile << "\n\tSend(INBOUND, buf, len);\n"
				<< "\tunmarshal(INBOUND, &ret);\n" 
				<< "\tdelete[] buf;\n\treturn ret;\n}\n\n" << endl;	
	}

	
	
	// File is done!
	return 0;

}



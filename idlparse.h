#include <list>
#include <string>
using namespace std;

enum IDLtype {INT32, INT16, BYTEARRAY, VOID, INVALID};

ostream& operator<< (ostream& os, const IDLtype& tp);
istream& operator>> (istream& is, IDLtype& tp);

struct variable
{
	IDLtype type;
	string name;
};


struct procedure
{
	int code;
	string name;
	list<variable> vars;
	IDLtype ret;
};


class IDLParser
{
public:
	IDLParser();
	int readfile(string infile);
	int writefile(string outfile);
	void ins_proc(procedure);
private:
	list<procedure> inbound_procs;
	list<procedure> outbound_procs;
};



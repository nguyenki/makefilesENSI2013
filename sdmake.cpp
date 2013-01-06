#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <mpi.h>
#include <sdmake.h>

int main( int argc, char* argv[] )

{


void parse(String &nameInputFile) {
	ifstream inFile;
	inFile.open(nameInputFile);
	if (inFile.bad()) {
		cerr <<"Bad bit is set in file"<< endl << endl;
		exit(BAD_FILE);
	}
	while (!inFile.eof()) {
		
	}
	
}



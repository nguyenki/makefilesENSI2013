#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <map>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <cstdlib>
#include <mpi.h>

using namespace std;
int main( int argc, char* argv[]) {
	int myRank;
	char name[MPI_MAX_PROCESSOR_NAME];
	int len;

	MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
        MPI_Get_processor_name(name, &len);
	cout <<"PROCESSEUR NAME:" << name <<endl;
	MPI_Finalize();
}


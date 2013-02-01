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

int main(int argc, char* argv[]) {
	int myRank;
	char name[MPI_MAX_PROCESSOR_NAME];
	int len;
	int size;
	int i=0;
	MPI_Status status;
	MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
        MPI_Get_processor_name(name, &len);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	cout <<"------------------------------- PROCESSEUR NAME: " << name << "----------------------" <<endl;

	if (myRank==0) {
		while (i<size) {
			cout << name <<": WAITING FOR SLAVE:" <<endl;
			i++;
			char msg[40]= "DO IT FOR ME SLAVE";
			MPI_Send(&msg, 40, MPI_CHAR, i, 456, MPI_COMM_WORLD);

			MPI_Recv(&len, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			cout <<"RECEIVE MESS DONE FROM:" << len <<endl;

		}
	} else {
		char msg1[40];
		MPI_Status status;
		cout << "CHO HANG TU MASTER ...................................." <<endl;
		MPI_Recv(&msg1, 40, MPI_CHAR, 0, 456, MPI_COMM_WORLD, &status);
		cout << "NHAN HANG TU MASTER:" << msg1 <<endl;

		cout <<"SEND MESS TO MASTER" <<endl;
		MPI_Request request;
		MPI_Isend(&myRank, 1, MPI_INT, 0, 123, MPI_COMM_WORLD, &request);
	}

	MPI_Finalize();
	return 0;
}


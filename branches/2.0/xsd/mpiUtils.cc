
#include "mpiUtils.h"

int MpiUtil::receiveString(int source, char* &char_buf, MPI_Status& status, bool blocking) {
	if (blocking) {
		MPI_Probe(source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	} else {
		int flag;
		MPI_Iprobe(source, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
		if (!flag)
			return flag;
	}
	int buffer_size;
	MPI_Get_count(&status, MPI_CHAR, &buffer_size);
	char_buf = new char[buffer_size];
	MPI_Recv(char_buf, buffer_size, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	return 1;
}
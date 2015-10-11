#include "worker.h"
#include "master.h"

using namespace std;

int main(int argc, char** argv) {

  int status = MPI_Init(&argc, &argv);
  if (status != 0) {
    std::cout << "MPI error\n";
    return -1;
  }

  int rank;
  int size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0) {
    MasterRoutine(size);
  } else {
    WorkerRoutine(size, rank);
  }

  MPI_Finalize();

  return 0;
}
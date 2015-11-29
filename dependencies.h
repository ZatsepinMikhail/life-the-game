#pragma once

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

using std::vector;
using std::string;

enum MessageType {
  INITIAL_FIELD_INFO,
  INITIAL_FIELD,
  ROW_EXCHANGE,
  GATHER_NEXT_STEP,
  GATHER_CURR_ITERATION,
  RUN_WORKERS,
  QUIT_WORKERS,
  STOP_WORKERS
};

const int MASTER = 0;
const int MANAGER = 1;

/* ToDoList

 */


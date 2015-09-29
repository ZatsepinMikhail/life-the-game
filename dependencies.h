#pragma once

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <pthread.h>
#include <random>
#include <ratio>
#include <semaphore.h>
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
  GATHER_NEXT_STEP
};

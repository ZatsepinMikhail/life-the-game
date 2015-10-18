#pragma once

#include "worker.h"

enum CommandType {
  START,
  STATUS,
  RUN,
  STOP,
  QUIT,
  WRONG_COMMAND
};

enum StateType {
  BEFORE_START,
  STARTED_NOT_RUNNING,
  RUNNING
};

enum ExtremeType {
  MAX_EXTREME,
  MIN_EXTREME
};

extern unsigned int max_iteration;
extern unsigned short workers_number;

extern vector<unsigned int> worker_iterations;

CommandType ParseCommand(std::string input_string);

int GetExtremeCurrentIteration(ExtremeType extremum);

void RunWorkers(Field* life_field);

void StopWorkers();



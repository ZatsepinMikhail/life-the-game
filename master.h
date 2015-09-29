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

extern bool game_finished;

extern vector<unsigned char> worker_states;
extern vector<unsigned int> worker_iterations;

CommandType ParseCommand(std::string input_string);

void MasterRoutine(const int workers_number);

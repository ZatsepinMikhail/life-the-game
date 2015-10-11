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

CommandType ParseCommand(std::string input_string);

void MasterRoutine(const int workers_number);

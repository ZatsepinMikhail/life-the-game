#include "master.h"

std::string state_type_strings[3] = {"BEFORE_START", "STARTED_NOT_RUNNING", "RUNNING"};

std::ostream& operator << (std::ostream& out, const StateType state) {
  out << state_type_strings[state];
  return out;
}

CommandType ParseCommand(std::string input_string) {
  int space_pos = input_string.find(' ');
  std::string parsed_command = input_string.substr(0, space_pos);
  CommandType result_command = WRONG_COMMAND;

  if (parsed_command == "START") {
    result_command = START;
  }
  if (parsed_command == "STATUS") {
    result_command = STATUS;
  }
  if (parsed_command == "RUN") {
    result_command = RUN;
  }
  if (parsed_command == "STOP") {
    result_command = STOP;
  }
  if (parsed_command == "QUIT") {
    result_command = QUIT;
  }
  return result_command;
}

int GetExtremeCurrentIteration(ExtremeType extremum) {
  int current_extreme_iteration = worker_iterations[0];
  int sign = (extremum == MAX_EXTREME) ? 1 : (-1);
  for (int i = 1; i < workers_number; ++i) {
    if (sign * worker_iterations[i] > sign * current_extreme_iteration) {
      current_extreme_iteration = worker_iterations[i];
    }
  }
  return current_extreme_iteration;
}

void RunWorkers(Field* life_field) {
  int id = omp_get_thread_num();
  //std::cout << omp_get_nested() << "\n";
  omp_set_nested(1);
  //std::cout << omp_get_nested() << "\n";
  if (id == 1) {
    //std::cout << "HERE\n";
#pragma omp parallel num_threads(workers_number)
    {
      //std::cout << "HERE\n";
      WorkerFuncArg *arg = new WorkerFuncArg();
      arg->field = life_field;
      arg->id = omp_get_thread_num();
      //std::cout << arg->id << "\n";
      WorkerFunction(arg);
    }
  }
}

void StopWorkers() {
#pragma omp critical(max_iteration)
  {
    max_iteration = -1;
  }
}




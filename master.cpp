#include "master.h"

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

void LockIterationSemaphores() {
  for (int i = 0; i < workers_number; ++i) {
    sem_wait(&iteration_semaphores[i]);
  }
}

void UnlockIterationSemaphores() {
  for (int i = workers_number - 1; i >= 0; --i) {
    sem_post(&iteration_semaphores[i]);
  }
}

//use only with LockIterationSemaphores() and UnlockIterationSemaphores()
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

void InitializeWorkerStructures() {
  int id = omp_get_thread_num();
  if (id == 1) {
#pragma omp parallel threads_num(workers_number)

  }
}


void CreateWorkers(vector<pthread_t>& workers, Field* life_field) {
  for (int i = 0; i < workers_number; ++i) {
    WorkerFuncArg* arg = new WorkerFuncArg();
    arg->field = life_field;
    arg->id = i;

    pthread_create(&workers[i], NULL, WorkerFunction, (void*) arg);
  }
}


void RerunWorkers(int steps_number) {
  pthread_mutex_lock(&game_finished_mutex);

  max_iteration += steps_number;
  pthread_cond_broadcast(&game_finished_cond_variable);

  pthread_mutex_unlock(&game_finished_mutex);

  UnlockIterationSemaphores();
}


void StopWorkers() {
  pthread_mutex_lock(&game_finished_mutex);

  game_finished = true;
  pthread_cond_broadcast(&game_finished_cond_variable);

  pthread_mutex_unlock(&game_finished_mutex);
}


void ReleaseResources(vector<pthread_t>& workers) {
  for (int i = 0; i < workers_number; ++i) {
    pthread_join(workers[i], NULL);
    pthread_mutex_destroy(&border_mutexes[i]);
  }
  pthread_mutex_destroy(&game_finished_mutex);
}




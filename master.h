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

extern vector<pthread_mutex_t> border_mutexes;
extern vector<pthread_cond_t> border_cond_variables;
extern vector<unsigned char> worker_states;
extern vector<unsigned int> worker_iterations;
extern vector<sem_t> iteration_semaphores;
extern pthread_mutex_t game_finished_mutex;
extern pthread_cond_t game_finished_cond_variable;

CommandType ParseCommand(std::string input_string);

int GetExtremeCurrentIteration(ExtremeType extremum);

void LockIterationSemaphores();

void UnlockIterationSemaphores();

void InitializeWorkerStructures(vector<pthread_t>& workers);

void CreateWorkers(vector<pthread_t>& workers, Field* life_field);

void RerunWorkers(int steps_number);

void StopWorkers();

void ReleaseResources(vector<pthread_t>& workers);


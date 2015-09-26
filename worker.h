#pragma once

#include "field.h"
#include <ratio>

//using namespace std::chrono;

unsigned int max_iteration = 0;
unsigned short workers_number = 0;

bool game_finished = false;

vector<pthread_mutex_t> border_mutexes;
vector<pthread_cond_t> border_cond_variables;

/*
There are 3 states of each thread:
0 - isn't red by other workers
1 - only left border is red by other worker
2 - two borders are red by other workers
*/
vector<unsigned char> worker_states;

vector<unsigned int> worker_iterations;

vector<sem_t> iteration_semaphores;

pthread_mutex_t game_finished_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t game_finished_cond_variable;

struct WorkerFuncArg {
  Field* field;
  int id; 
};

enum class ExtraRowType {
  UPPER, 
  LOWER, 
  NO
};

//steady_clock::time_point start_time;

bool CalculateOneCell(Field* life_field, int row, int cell, 
                      ExtraRowType need_extra_row, const vector<bool>& extra_row);


void CalculateNextStep(Field* life_field, int lower_bound, int upper_bound, 
                       const vector<vector<bool>>& neighbour_rows);

bool NeedNextStep(int worker_id, Field* life_field);

void* WorkerFunction(void* structed_args);
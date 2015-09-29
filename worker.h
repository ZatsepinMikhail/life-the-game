#pragma once

#include "field.h"

/*
There are 3 states of each thread:
0 - isn't red by other workers
1 - only left border is red by other worker
2 - two borders are red by other workers
*/

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

void WorkerRoutine(const int worker_number, const int rank);
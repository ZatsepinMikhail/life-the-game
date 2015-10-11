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

bool CalculateOneCell(vector<vector<bool> >& field, int row, int cell,
                      ExtraRowType need_extra_row, bool* extra_row);


void CalculateNextStep(vector<vector<bool> >& field_piece,
                       bool* lower_raw_row, bool* upper_raw_row);

bool NeedNextStep(const int comm_size, const int rank,
                  bool* raw_field_piece, const vector<vector<bool> >& field_piece);

void WorkerRoutine(const int worker_number, const int rank);

void StructureFieldPieceRaw(bool* raw_field, vector<vector<bool> >& structured_field);
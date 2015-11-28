#pragma once

#include "field.h"

struct WorkerFuncArg {
  Field* field;
  int id; 
};

bool CalculateOneCell(vector<vector<bool> >& field, int row, int cell);


void CalculateNextStep(vector<vector<bool> >& field_piece);

bool NeedNextStep(const int comm_size, const int rank,
                  bool* raw_field_piece, const vector<vector<bool> >& field_piece);

void WorkerRoutine(const int worker_number, const int rank);
#pragma once

#include "dependencies.h"

struct Field {
  Field() {}

  Field(string field_info);

  void show_field();

  int height_;
  int width_;
  vector<vector<bool>> field_;

private:

  void random_field_init();

  void file_field_init(string filename);
};
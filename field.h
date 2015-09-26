#pragma once

#include <vector>
#include <string>
#include <random>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <cstdlib>
#include <chrono>

using std::vector;
using std::string;


struct Field {
  Field() {}

  Field(string field_info);

  void show_field();

  size_t height_;
  size_t width_;
  vector<vector<bool>> field_;

private:

  void random_field_init();

  void file_field_init(string filename);
};
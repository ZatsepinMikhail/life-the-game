#include "worker.h"

clock_t start_time;
clock_t end_time;

unsigned int max_iteration = 0;
unsigned short workers_number = 0;

bool game_finished = false;

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


bool CalculateOneCell(Field* life_field, int row, int cell,
                      ExtraRowType need_extra_row, const vector<bool>& extra_row) {

  int alive_neighbour_number = 0;
  int width = life_field->width_;
  int height = life_field->height_;

  int lower_bound_shift = -1;
  int upper_bound_shift = 1;

  //process cell's row
  alive_neighbour_number += life_field->field_[row][cell == 0 ? width - 1 : cell - 1];
  alive_neighbour_number += life_field->field_[row][(cell + 1) % width];

  if (need_extra_row != NO_ROW) {
    alive_neighbour_number += extra_row[cell == 0 ? width - 1 : cell - 1];
    alive_neighbour_number += extra_row[(cell + 1) % width];
    alive_neighbour_number += extra_row[cell];
    if (need_extra_row == LOWER_ROW) {
      lower_bound_shift = 1;
    } else {
      upper_bound_shift = -1;
    }
  }
  for (int i = lower_bound_shift; i <= upper_bound_shift; i += 2) {
    int current_row = (row + i < 0) ? height - 1 : ((row + i) % height);
    for (int j = -1; j <= 1; ++j) {
      int current_cell = (cell + j < 0) ? (width - 1) : ((cell + j) % width);
      alive_neighbour_number += life_field->field_[current_row][current_cell];
    }
  }

  return (!life_field->field_[row][cell] && alive_neighbour_number == 3) ||
         (life_field->field_[row][cell] && alive_neighbour_number >= 2 && alive_neighbour_number <= 3);
}


void CalculateNextStep(Field* life_field, int lower_bound, int upper_bound,
                       const vector<vector<bool> >& neighbour_rows) {

  int width = life_field->width_;

  //make local copy
  vector<bool> empty_initializer(width, false);
  vector<vector<bool> > local_copy_of_field(upper_bound - lower_bound + 1, empty_initializer);

  for (int i = lower_bound; i <= upper_bound; ++i) {
    for (int j = 0; j < width; ++j) {
      if (i == lower_bound) {

        local_copy_of_field[i - lower_bound][j] = CalculateOneCell(life_field, i, j, LOWER_ROW, neighbour_rows[0]);

      } else if (i == upper_bound) {

        local_copy_of_field[i - lower_bound][j] = CalculateOneCell(life_field, i, j, UPPER_ROW, neighbour_rows[1]);

      } else {

        local_copy_of_field[i - lower_bound][j] = CalculateOneCell(life_field, i, j, NO_ROW, empty_initializer);

      }
    }
  }

  for (int i = lower_bound; i <= upper_bound; ++i) {
    life_field->field_[i] = local_copy_of_field[i - lower_bound];
  }
}


bool NeedNextStep(int worker_id) {

  bool result = true;

  sem_wait(&iteration_semaphores[worker_id]);
  if (worker_iterations[worker_id] >= max_iteration) {
    result = false;
  } else {
    ++worker_iterations[worker_id];
  }
  sem_post(&iteration_semaphores[worker_id]);

  if (!result) {

    //time measurement
    if (worker_id == 0) {
      time(&end_time);

      std::cout << "It took me " << difftime(end_time, start_time) << " seconds\n";
    }

    pthread_mutex_lock(&game_finished_mutex);
    while(worker_iterations[worker_id] >= max_iteration && !game_finished) {
      pthread_cond_wait(&game_finished_cond_variable, &game_finished_mutex);
    }
    result = !game_finished;
    ++worker_iterations[worker_id];
    pthread_mutex_unlock(&game_finished_mutex);

  }

  return result;
}


void* WorkerFunction(void* structed_args) {

  WorkerFuncArg* arg = (WorkerFuncArg*) structed_args;

  int worker_id = arg->id;
  Field* life_field = arg->field;

  int height = life_field->height_;
  int width = life_field->width_;

  int block_size = height / workers_number;
  int lower_bound = worker_id * block_size;
  int upper_bound = (worker_id == workers_number - 1) ? (height - 1) : (lower_bound + block_size - 1);


  int lower_neighbour_row_index = (lower_bound == 0) ? (height - 1) : (lower_bound - 1);
  int upper_neighbour_row_index = (upper_bound + 1) % height;

  vector<bool> empty_initializer(width, false);
  vector<vector<bool> > neighbour_rows(2, empty_initializer);

  if (worker_id == 0) {
    time(&start_time);
  }

  while(NeedNextStep(worker_id)) {

    int curr_neighbour_row_index = lower_neighbour_row_index;

    for (int i = 0; i < 2; ++i) {

      neighbour_rows[i] = life_field->field_[curr_neighbour_row_index];
#pragma omp barrier
      curr_neighbour_row_index = upper_neighbour_row_index;
    }

    CalculateNextStep(life_field, lower_bound, upper_bound, neighbour_rows);
#pragma omp barrier
  }

  return NULL;
}

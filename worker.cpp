#include "worker.h"

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

  if (need_extra_row != ExtraRowType::NO) {
    alive_neighbour_number += extra_row[cell == 0 ? width - 1 : cell - 1];
    alive_neighbour_number += extra_row[(cell + 1) % width];
    alive_neighbour_number += extra_row[cell];
    if (need_extra_row == ExtraRowType::LOWER) {
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
                       const vector<vector<bool>>& neighbour_rows) {

  int width = life_field->width_;

  //make local copy
  vector<bool> empty_initializer(width, false);
  vector<vector<bool>> local_copy_of_field(upper_bound - lower_bound + 1, empty_initializer);

  for (int i = lower_bound; i <= upper_bound; ++i) {
    for (int j = 0; j < width; ++j) {
      if (i == lower_bound) {

        local_copy_of_field[i - lower_bound][j] = CalculateOneCell(life_field, i, j, ExtraRowType::LOWER, neighbour_rows[0]);

      } else if (i == upper_bound) {

        local_copy_of_field[i - lower_bound][j] = CalculateOneCell(life_field, i, j, ExtraRowType::UPPER, neighbour_rows[1]);

      } else {

        local_copy_of_field[i - lower_bound][j] = CalculateOneCell(life_field, i, j, ExtraRowType::NO, empty_initializer);

      }
    }
  }

  for (int i = lower_bound; i <= upper_bound; ++i) {
    life_field->field_[i] = local_copy_of_field[i - lower_bound];
  }
}


/*bool NeedNextStep(int worker_id) {

  bool result = true;

  sem_wait(&iteration_semaphores[worker_id]);
  if (worker_iterations[worker_id] >= max_iteration) {
    result = false;
  } else {
    ++worker_iterations[worker_id];
  }
  sem_post(&iteration_semaphores[worker_id]);

  if (!result) {

    /*if (worker_id == 0) {
      steady_clock::time_point end_time = steady_clock::now();

      duration<double> time_span = duration_cast<duration<double>>(end_time - start_time);

      std::cout << "It took me " << time_span.count() << " seconds.";
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
}*/


void StructureFieldPeaceRaw(bool* raw_field, vector<vector<bool> >& structured_field,
                            const int height, const int width) {
  int curr_index = 0;
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j, ++curr_index) {
      structured_field[i][j] = raw_field[curr_index];
    }
  }
}

void SerializeRow(const vector<bool>& row, bool* raw_row, const int width) {
  for (int i = 0; i < width; ++i) {
    raw_row[i] = row[i];
  }
}

void WorkerRoutine(const int comm_size, const int rank) {

  //get field peace
  int initial_field_info[2];

  MPI_Status status;
  MPI_Recv(initial_field_info, 2, MPI::INT, 0, INITIAL_FIELD_INFO, MPI_COMM_WORLD, &status);

  int height = initial_field_info[0];
  int width = initial_field_info[1];

  int peace_size = width * height;
  bool* raw_field_peace = new bool(peace_size);
  MPI_Recv(raw_field_peace, peace_size, MPI::BOOL, 0, INITIAL_FIELD, MPI_COMM_WORLD, &status);

  vector<bool> empty_initializer(width, false);
  vector<vector<bool> > field_peace(height, empty_initializer);
  StructureFieldPeaceRaw(raw_field_peace, field_peace, height, width);

  int lower_worker_rank = (rank == 1) ? (comm_size - 1) : (rank - 1);
  int upper_worker_rank = (rank == comm_size - 1) ? 1 : (rank + 1);

  vector<vector<bool>> neighbour_rows(2, empty_initializer);

  bool* lower_raw_row = new bool(width);
  bool* upper_raw_row = new bool(width);


  while(NeedNextStep(rank)) {

    int curr_neighbour_raw_index = 0;
    int curr_neighbour_rank = lower_worker_rank;
    bool* curr_raw_row = lower_raw_row;

    for (int i = 0; i < 2; ++i) {
      SerializeRow(field_peace[curr_neighbour_raw_index], curr_raw_row, width);
      MPI_Sendrecv()

      curr_neighbour_rank = upper_worker_rank;
      curr_raw_row = upper_raw_row;
      curr_neighbour_raw_index = height - 1;
    }

    pthread_mutex_lock(&border_mutexes[worker_id]);
    while(worker_states[worker_id] != 2) {
      pthread_cond_wait(&border_cond_variables[worker_id], &border_mutexes[worker_id]);
    }

    pthread_mutex_unlock(&border_mutexes[worker_id]);

    CalculateNextStep(life_field, lower_bound, upper_bound, neighbour_rows);


    pthread_mutex_lock(&border_mutexes[worker_id]);

    worker_states[worker_id] = 0;
    pthread_cond_broadcast(&border_cond_variables[worker_id]);
    pthread_mutex_unlock(&border_mutexes[worker_id]);
  }

  */
}

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


bool CalculateOneCell(vector<vector<bool> >& field, int row, int cell,
                      ExtraRowType need_extra_row, bool* extra_row) {

  int alive_neighbour_number = 0;
  int width = field[0].size();
  int height = field.size();

  int lower_bound_shift = -1;
  int upper_bound_shift = 1;

  //process cell's row
  alive_neighbour_number += field[row][cell == 0 ? width - 1 : cell - 1];
  alive_neighbour_number += field[row][(cell + 1) % width];

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
      alive_neighbour_number += field[current_row][current_cell];
    }
  }

  return (!field[row][cell] && alive_neighbour_number == 3) ||
         (field[row][cell] && alive_neighbour_number >= 2 && alive_neighbour_number <= 3);
  return true;
}


void CalculateNextStep(vector<vector<bool> >& field_piece,
                       bool* lower_raw_row, bool* upper_raw_row) {

  int width = field_piece[0].size();
  int height = field_piece.size();

  vector<vector<bool>> next_step_field_piece(field_piece);

  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      if (i == 0) {

        next_step_field_piece[i][j] = CalculateOneCell(field_piece, i, j, ExtraRowType::LOWER, lower_raw_row);

      } else if (i == height - 1) {

        next_step_field_piece[i][j] = CalculateOneCell(field_piece, i, j, ExtraRowType::UPPER, upper_raw_row);

      } else {

        next_step_field_piece[i][j] = CalculateOneCell(field_piece, i, j, ExtraRowType::NO, NULL);

      }
    }
  }

  for (int i = 0; i < height; ++i) {
    field_piece[i] = next_step_field_piece[i];
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

void StructureFieldPieceRaw(bool* raw_field, vector<vector<bool> >& structured_field) {
  int curr_index = 0;
  for (int i = 0; i < structured_field.size(); ++i) {
    for (int j = 0; j < structured_field[0].size(); ++j, ++curr_index) {
      structured_field[i][j] = raw_field[curr_index];
    }
  }
}


void SerializeRow(const vector<bool>& row, bool* raw_row) {
  for (int i = 0; i < row.size(); ++i) {
    raw_row[i] = row[i];
  }
}

void SerializeField(const vector<vector<bool> >& field, bool* raw_field) {
  int shift = 0;
  int width = field[0].size();
  for (int i = 0; i < field.size(); ++i) {
    SerializeRow(field[i], raw_field + shift);
    shift += width;
  }
}

void WorkerRoutine(const int comm_size, const int rank) {

  //get field piece
  int initial_field_info[2];

  MPI_Status status;
  MPI_Recv(initial_field_info, 2, MPI::INT, 0, INITIAL_FIELD_INFO, MPI_COMM_WORLD, &status);

  int height = initial_field_info[0];
  int width = initial_field_info[1];

  int piece_size = width * height;

  bool* raw_field_piece = new bool[piece_size];
  MPI_Recv(raw_field_piece, piece_size, MPI::BOOL, 0, INITIAL_FIELD, MPI_COMM_WORLD, &status);

  vector<bool> empty_initializer(width, false);
  vector<vector<bool> > field_piece(height, empty_initializer);
  StructureFieldPieceRaw(raw_field_piece, field_piece);
  //end

  int lower_worker_rank = (rank == 1) ? (comm_size - 1) : (rank - 1);
  int upper_worker_rank = (rank == comm_size - 1) ? 1 : (rank + 1);

  //vector<vector<bool>> neighbour_rows(2, empty_initializer);

  bool* lower_raw_row_send = new bool[width];
  bool* upper_raw_row_send = new bool[width];
  bool* lower_raw_row_recv = new bool[width];
  bool* upper_raw_row_recv = new bool[width];


  //while

  int curr_structed_raw_send = 0;

  int curr_neighbour_rank_send = lower_worker_rank;
  int curr_neighbour_rank_recv = upper_worker_rank;

  bool* curr_raw_row_send = lower_raw_row_send;
  bool* curr_raw_row_recv = upper_raw_row_recv;

  for (int i = 0; i < 2; ++i) {
    SerializeRow(field_piece[curr_structed_raw_send], curr_raw_row_send);

    MPI_Sendrecv(curr_raw_row_send, width, MPI::BOOL, curr_neighbour_rank_send, ROW_EXCHANGE,
                 curr_raw_row_recv, width, MPI::BOOL, curr_neighbour_rank_recv, ROW_EXCHANGE,
                 MPI_COMM_WORLD, &status);

    curr_neighbour_rank_send = upper_worker_rank;
    curr_neighbour_rank_recv = lower_worker_rank;

    curr_raw_row_send = upper_raw_row_send;
    curr_raw_row_recv = lower_raw_row_recv;

    curr_structed_raw_send = height - 1;
  }

  CalculateNextStep(field_piece, lower_raw_row_recv, upper_raw_row_recv);

  SerializeField(field_piece, raw_field_piece);
  
  MPI_Send(raw_field_piece, piece_size, MPI::BOOL, 0, GATHER_NEXT_STEP, MPI_COMM_WORLD);

/*delete[] raw_field_piece;
delete[] lower_raw_row_recv;
delete[] lower_raw_row_send;
delete[] upper_raw_row_recv;
delete[] upper_raw_row_send;*/
}

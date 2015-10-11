#include "worker.h"

unsigned int max_iteration = 0;
unsigned int curr_iteration = 0;

int width = 0;
int height = 0;

bool* message_buffer;

MPI_Request stop_request;
bool init_irecv = false;
bool need_irecv = true;

bool CalculateOneCell(vector<vector<bool> >& field, int row, int cell,
                      ExtraRowType need_extra_row, bool* extra_row) {

  int alive_neighbour_number = 0;

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
}


void CalculateNextStep(vector<vector<bool> >& field_piece,
                       bool* lower_raw_row, bool* upper_raw_row) {

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

int ParseIteration(bool* buffer, int size) {
  int result = 0;
  int degree = 1;
  for (int i = 0; i < size; ++i) {
    if (buffer[i]) {
      result += degree;
    }
    degree *= 2;
  }
  return result;
}


void SerializeIteration(bool* buffer, int size, int iteration) {
  for (int i = 0; i < size; ++i) {
    buffer[i] = iteration % 2;
    iteration /= 2;
  }
}



void SerializeRow(const vector<bool>& row, bool* raw_row) {
  for (int i = 0; i < row.size(); ++i) {
    raw_row[i] = row[i];
  }
}

void SerializeField(const vector<vector<bool> >& field, bool* raw_field) {
  int shift = 0;
  for (int i = 0; i < field.size(); ++i) {
    SerializeRow(field[i], raw_field + shift);
    shift += width;
  }
}


bool NeedNextStep(const int comm_size, const int rank,
                  bool* raw_field_piece, const vector<vector<bool> >& field_piece) {

  int control_message;
  int flag = false;

  MPI_Status status;
  bool sent_iteration = false;

  while (curr_iteration == max_iteration) {
    flag = false;
    if (!sent_iteration && curr_iteration != 0) {
      MPI_Send(&max_iteration, 1, MPI::INT, 0, GATHER_CURR_ITERATION, MPI_COMM_WORLD);
      std::cout << rank << " sent to master its iteration\n";
      sent_iteration = true;
    }
    //std::cout << "WAIT FOR A MESSAGE!\n";

    if (init_irecv && need_irecv) {
      //std::cout << "WAIT FOR A MESSAGE!\n";
      while(!flag) {
        MPI_Test(&stop_request, &flag, &status);
      }
      init_irecv = false;
    } else {
      //std::cout << rank << " USE RECV\n";
      MPI_Recv(&control_message, 1, MPI::INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
    //std::cout << "GOT MESSAGE!\n";
    switch (status.MPI_TAG) {
      case RUN_WORKERS:
        max_iteration += control_message;
        //std::cout << "\"RUN " << control_message << "\"\n";
        return true;
      case QUIT_WORKERS:
        return false;
      case STOP_WORKERS:
        //std::cout << rank << " got stop message\n";
        break;
      case GATHER_NEXT_STEP:
        //std::cout << "worker " << rank << " received from master\n";
        SerializeField(field_piece, raw_field_piece);
        MPI_Send(raw_field_piece, height * width, MPI::BOOL, 0, GATHER_NEXT_STEP, MPI_COMM_WORLD);
        //std::cout << "worker " << rank << " sent to master\n";
        break;
    }
  }

  //std::cout << "EXIT FROM WHILE\n";
  if (rank == 1 && need_irecv) {
    if (!init_irecv) {
      MPI_Irecv(&control_message, 1, MPI::INT, 0, STOP_WORKERS, MPI_COMM_WORLD, &stop_request);
      init_irecv = true;
    }

    //std::cout << "BEFORE TEST\n";
    MPI_Test(&stop_request, &flag, &status);
    if (flag) {

      //std::cout << "GOT MESSAGE WHILE WORKING!\n";
      init_irecv = false;

      if (max_iteration - curr_iteration == 1) {
        ++curr_iteration;
        return true;
      }

      //std::cout << "HERE\n";
      max_iteration = curr_iteration + 3;
      std::cout << "new max iteration = " << max_iteration << "\n";
      SerializeIteration(message_buffer, width, max_iteration);

      for (int i = 2; i < comm_size; ++i) {
        MPI_Send(message_buffer, width, MPI::BOOL, i, STOP_WORKERS, MPI_COMM_WORLD);
      }

      need_irecv = false;
    }
  }

  ++curr_iteration;
  return true;
}

void StructureFieldPieceRaw(bool* raw_field, vector<vector<bool> >& structured_field) {
  int curr_index = 0;
  for (int i = 0; i < structured_field.size(); ++i) {
    for (int j = 0; j < structured_field[0].size(); ++j, ++curr_index) {
      structured_field[i][j] = raw_field[curr_index];
    }
  }
}

void WorkerRoutine(const int comm_size, const int rank) {

  //get field piece
  int initial_field_info[2];

  MPI_Status status;
  MPI_Recv(initial_field_info, 2, MPI::INT, 0, INITIAL_FIELD_INFO, MPI_COMM_WORLD, &status);

  height = initial_field_info[0];
  width = initial_field_info[1];

  int piece_size = width * height;

  bool* raw_field_piece = new bool[piece_size];
  MPI_Recv(raw_field_piece, piece_size, MPI::BOOL, 0, INITIAL_FIELD, MPI_COMM_WORLD, &status);

  vector<bool> empty_initializer(width, false);
  vector<vector<bool> > field_piece(height, empty_initializer);
  StructureFieldPieceRaw(raw_field_piece, field_piece);
  //end

  int lower_worker_rank = (rank == 1) ? (comm_size - 1) : (rank - 1);
  int upper_worker_rank = (rank == comm_size - 1) ? 1 : (rank + 1);

  bool* lower_raw_row_send = new bool[width];
  bool* upper_raw_row_send = new bool[width];
  bool* lower_raw_row_recv = new bool[width];
  bool* upper_raw_row_recv = new bool[width];

  message_buffer = new bool[width];

  while(NeedNextStep(comm_size, rank, raw_field_piece, field_piece)) {

    int curr_structed_raw_send = 0;

    int curr_neighbour_rank_send = lower_worker_rank;
    int curr_neighbour_rank_recv = upper_worker_rank;

    bool *curr_raw_row_send = lower_raw_row_send;
    bool *curr_raw_row_recv = upper_raw_row_recv;

    for (int i = 0; i < 2; ++i) {
      SerializeRow(field_piece[curr_structed_raw_send], curr_raw_row_send);

      MPI_Sendrecv(curr_raw_row_send, width, MPI::BOOL, curr_neighbour_rank_send, ROW_EXCHANGE,
                   curr_raw_row_recv, width, MPI::BOOL, MPI_ANY_SOURCE, MPI_ANY_TAG,
                   MPI_COMM_WORLD, &status);

      if (status.MPI_TAG == STOP_WORKERS) {
        max_iteration = ParseIteration(curr_raw_row_recv, width);
        std::cout << rank << ":YEAHHHHH! (" << curr_iteration << "-" << max_iteration << ")\n";
        if (max_iteration < curr_iteration) {
          curr_iteration = max_iteration;
          continue;
        }


        std::cout << "before receieving good message\n";
        MPI_Recv(curr_raw_row_recv, width, MPI::BOOL, curr_neighbour_rank_recv, ROW_EXCHANGE,
                 MPI_COMM_WORLD, &status);
        std::cout << "receive good message\n";
      }


      curr_neighbour_rank_send = upper_worker_rank;
      curr_neighbour_rank_recv = lower_worker_rank;

      curr_raw_row_send = upper_raw_row_send;
      curr_raw_row_recv = lower_raw_row_recv;

      curr_structed_raw_send = height - 1;
    }

    CalculateNextStep(field_piece, lower_raw_row_recv, upper_raw_row_recv);
  }

  delete[] message_buffer;
  delete[] raw_field_piece;
  delete[] lower_raw_row_recv;
  delete[] lower_raw_row_send;
  delete[] upper_raw_row_recv;
  delete[] upper_raw_row_send;
}

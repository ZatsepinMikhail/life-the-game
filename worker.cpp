#include "worker.h"

unsigned int max_iteration = 0;
unsigned int curr_iteration = 0;

int width = 0;
int height = 0;
int expanded_height = 0;

bool* message_buffer;

MPI_Request stop_request;
bool init_irecv = false;
bool need_irecv = true;

bool CalculateOneCell(vector<vector<bool> >& field, int row, int cell) {

  int alive_neighbour_number = 0;

  for (int i = row - 1; i <= row + 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      if (i == row && j == 0) {
        continue;
      }
      int curr_cell = (cell + j < 0) ? (width - 1) : ((cell + j) % width);
      alive_neighbour_number += field[i][curr_cell];
    }
  }

  return (!field[row][cell] && alive_neighbour_number == 3) ||
         (field[row][cell] && alive_neighbour_number >= 2 && alive_neighbour_number <= 3);
}


void CalculateNextStep(vector<vector<bool> >& field_piece) {

  vector<vector<bool>> next_step_field_piece(field_piece);

  for (int i = 1; i < expanded_height - 1; ++i) {
    for (int j = 0; j < width; ++j) {
      next_step_field_piece[i][j] = CalculateOneCell(field_piece, i, j);
    }
  }

  for (int i = 1; i < expanded_height - 1; ++i) {
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

void DeserializeRow(vector<bool>& row, bool* raw_row) {
  for (int i = 0; i < row.size(); ++i) {
    row[i] = raw_row[i];
  }
}

void SerializeField(const vector<vector<bool> >& field, bool* raw_field) {
  int shift = 0;
  for (int i = 1; i <= height; ++i) {
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
      MPI_Send(&max_iteration, 1, MPI::INT, MASTER, GATHER_CURR_ITERATION, MPI_COMM_WORLD);
      sent_iteration = true;
    }

    if (init_irecv && need_irecv) {

      while(!flag) {
        MPI_Test(&stop_request, &flag, &status);
      }
      init_irecv = false;
    } else {
      MPI_Recv(&control_message, 1, MPI::INT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }

    switch (status.MPI_TAG) {
      case RUN_WORKERS:
        max_iteration += control_message;
        need_irecv = true;
        return true;
      case QUIT_WORKERS:
        return false;
      case STOP_WORKERS:

        break;
      case GATHER_NEXT_STEP:
        SerializeField(field_piece, raw_field_piece);
        MPI_Send(raw_field_piece, height * width, MPI::BOOL, MASTER, GATHER_NEXT_STEP, MPI_COMM_WORLD);
        break;
    }
  }

  if (rank == 1 && need_irecv) {
    if (!init_irecv) {
      MPI_Irecv(&control_message, MANAGER, MPI::INT, MASTER, STOP_WORKERS, MPI_COMM_WORLD, &stop_request);
      init_irecv = true;
    }

    MPI_Test(&stop_request, &flag, &status);
    if (flag) {
      init_irecv = false;
      if (max_iteration - curr_iteration <= 10) {
        ++curr_iteration;
        return true;
      }

      max_iteration = curr_iteration + 10;
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
  for (int i = 1; i <= height; ++i) {
    for (int j = 0; j < width; ++j, ++curr_index) {
      structured_field[i][j] = raw_field[curr_index];
    }
  }
}

void WorkerRoutine(const int comm_size, const int rank) {

  int initial_field_info[2];

  MPI_Status status;

  MPI_Recv(initial_field_info, 2, MPI::INT, MASTER, INITIAL_FIELD_INFO, MPI_COMM_WORLD, &status);

  height = initial_field_info[0];
  width = initial_field_info[1];
  expanded_height = height + 2;

  int piece_size = width * height;
  bool* raw_field_piece = new bool[piece_size];
  MPI_Recv(raw_field_piece, piece_size, MPI::BOOL, MASTER, INITIAL_FIELD, MPI_COMM_WORLD, &status);

  vector<bool> empty_initializer(width, false);
  vector<vector<bool> > field_piece(expanded_height, empty_initializer);
  StructureFieldPieceRaw(raw_field_piece, field_piece);

  int lower_worker_rank = (rank == 1) ? (comm_size - 1) : (rank - 1);
  int upper_worker_rank = (rank == comm_size - 1) ? 1 : (rank + 1);

  bool* lower_raw_row_send = new bool[width];
  bool* upper_raw_row_send = new bool[width];
  bool* lower_raw_row_recv = new bool[width];
  bool* upper_raw_row_recv = new bool[width];

  message_buffer = new bool[width];

  while(NeedNextStep(comm_size, rank, raw_field_piece, field_piece)) {

    int curr_structed_raw_send = 1;

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

        if (max_iteration < curr_iteration) {
          curr_iteration = max_iteration;
          continue;
        }

        MPI_Recv(curr_raw_row_recv, width, MPI::BOOL, curr_neighbour_rank_recv, ROW_EXCHANGE,
                 MPI_COMM_WORLD, &status);
      }


      curr_neighbour_rank_send = upper_worker_rank;
      curr_neighbour_rank_recv = lower_worker_rank;

      curr_raw_row_send = upper_raw_row_send;
      curr_raw_row_recv = lower_raw_row_recv;

      curr_structed_raw_send = height;
    }
    DeserializeRow(field_piece[0], lower_raw_row_recv);
    DeserializeRow(field_piece[expanded_height - 1], upper_raw_row_recv);
    CalculateNextStep(field_piece);
  }

  delete[] message_buffer;
  delete[] raw_field_piece;
  delete[] lower_raw_row_recv;
  delete[] lower_raw_row_send;
  delete[] upper_raw_row_recv;
  delete[] upper_raw_row_send;
}

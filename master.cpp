#include "master.h"

CommandType ParseCommand(std::string input_string) {
  int space_pos = input_string.find(' ');
  std::string parsed_command = input_string.substr(0, space_pos);
  CommandType result_command = WRONG_COMMAND;

  if (parsed_command == "START") {
    result_command = START;
  }
  if (parsed_command == "STATUS") {
    result_command = STATUS;
  }
  if (parsed_command == "RUN") {
    result_command = RUN;
  }
  if (parsed_command == "STOP") {
    result_command = STOP;
  }
  if (parsed_command == "QUIT") {
    result_command = QUIT;
  }
  return result_command;
}

void GetBuffer(Field* life_field, const int start_row, const int row_number, bool* buffer) {
  int curr_index = 0;
  for (int i = 0; i < row_number; ++i) {
    for (int j = 0; j < life_field->width_; ++j, ++curr_index) {
      buffer[curr_index] = life_field->field_[i + start_row][j];
    }
  }
}

void MasterRoutine(const int comm_size) {

  bool is_game_finished = false;

  StateType current_state = BEFORE_START;

  std::string command;
  Field* life_field;
  bool* field_buffer;
  int current_iteration = 0;

  int initial_field_info[2];

  while(!is_game_finished) {
    std::cout << "$: ";
    std::cin >> command;
    CommandType current_command = ParseCommand(command);

    switch (current_command) {
      case START: {
        std::string field_info_string;

        std::cin >> field_info_string;

        if (current_state == STARTED_NOT_RUNNING) {
          std::cout << "The system has already started\n";
          break;
        }

        if (current_state == RUNNING) {
          std::cout << "The system is already running, you can't start it.\n";
          break;
        }

        //create field
        try {
          life_field = new Field(field_info_string);
        } catch (std::invalid_argument &e) {
          std::cout << e.what() << "\n";
          break;
        }

        int start_piece_index = 0;

        life_field->show_field();

        std::cout << "----------------------\n";

        field_buffer = new bool[life_field->height_ * life_field->width_];

        initial_field_info[0] = life_field->height_ / (comm_size - 1);
        initial_field_info[1] = life_field->width_;

        for (int i = 1; i < comm_size; ++i) {
          if (i == comm_size - 1) {
            initial_field_info[0] += life_field->height_ % (comm_size - 1);
          }
          MPI_Send(initial_field_info, 2, MPI::INT, i, INITIAL_FIELD_INFO, MPI_COMM_WORLD);

          GetBuffer(life_field, start_piece_index, initial_field_info[0], field_buffer);

          MPI_Send(field_buffer, initial_field_info[0] * life_field->width_, MPI::BOOL,
                   i, INITIAL_FIELD, MPI_COMM_WORLD);

          start_piece_index += initial_field_info[0];
        }

        current_state = STARTED_NOT_RUNNING;
        break;
      }

      case STATUS: {
        if (current_state == BEFORE_START || current_state == RUNNING) {
          std::cout << "The system can't show status in this state (" << current_state << ")\n";
          break;
        }

        MPI_Status status;
        initial_field_info[0] = life_field->height_ / (comm_size - 1);

        bool *curr_start_point = field_buffer;
        int control_message = 0;
        for (int i = comm_size - 1; i >= 1; --i) {
          if (i == comm_size - 1) {
            initial_field_info[0] += life_field->height_ % (comm_size - 1);
          }

          MPI_Send(&control_message, 1, MPI::INT,
                   i, GATHER_NEXT_STEP, MPI_COMM_WORLD);
          MPI_Recv(curr_start_point, initial_field_info[0] * life_field->width_, MPI::BOOL,
                   i, GATHER_NEXT_STEP, MPI_COMM_WORLD, &status);
          curr_start_point += initial_field_info[0] * life_field->width_;
        }

        StructureFieldPieceRaw(field_buffer, life_field->field_);

        std::cout << "at iteration " << current_iteration << "\n";
        life_field->show_field();
        break;
      }

      case STOP: {
        if (current_state == BEFORE_START || current_state == STARTED_NOT_RUNNING) {
          std::cout << "The system isn't running now.\n";
          break;
        }

        int stop_message = 0;
        MPI_Send(&stop_message, 1, MPI::INT, 1, STOP_WORKERS, MPI_COMM_WORLD);

        MPI_Status status;
        for (int i = 1; i < comm_size; ++i) {
          MPI_Recv(&current_iteration, 1, MPI::INT, i, GATHER_CURR_ITERATION, MPI_COMM_WORLD, &status);
        }

        std::cout << "Workers stopped at " << current_iteration << "\n";
        current_state = STARTED_NOT_RUNNING;
        break;
      }

      case RUN: {
        string steps_number_string;
        std::cin >> steps_number_string;

        if (current_state == BEFORE_START) {
          std::cout << "You can't run system before start.\n";
          break;
        }

        if (current_state == RUNNING) {
          std::cout << "the system is already running.\n";
          break;
        }

        int steps_number;

        try {
          steps_number = std::stoi(steps_number_string);
        } catch(std::invalid_argument& e) {
          std::cout << "Enter correct NUMBER of steps, please.\n";
          break;
        } catch (std::out_of_range &e) {
          std::cout << "Too big value of number of steps.\n";
          break;
        }

        for (int i = 1; i < comm_size; ++i) {
          MPI_Send(&steps_number, 1, MPI::INT, i, RUN_WORKERS, MPI_COMM_WORLD);
        }

        current_state = RUNNING;
        break;
      }

      case QUIT: {
        int message = 0;
        for (int i = 1; i < comm_size; ++i) {
          MPI_Send(&message, 1, MPI::INT, i, QUIT_WORKERS, MPI_COMM_WORLD);
        }
        is_game_finished = true;
        break;
      }

      case WRONG_COMMAND:
        std::cout << "You've entered wrong command. Try again, please.\n";
        break;
      }

  }
}

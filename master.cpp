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

bool* GetBuffer(Field* life_field, const int start_row, const int row_number) {
  bool* buffer = new bool(row_number * life_field->width_);
  int curr_index = 0;
  for (int i = 0; i < row_number; ++i) {
    for (int j = 0; j < life_field->width_; ++j, ++curr_index) {
      buffer[curr_index] = life_field->field_[i + start_row][j];
      std::cout << buffer[curr_index];
    }
  }
  std::cout << "\n";
  return buffer;
}

void MasterRoutine(const int comm_size) {

  StateType current_state = BEFORE_START;

  std::string command;
  Field* life_field;

  //while(!game_finished) {
    std::cout << "$: ";
    std::cin >> command;
    CommandType current_command = ParseCommand(command);

    //maybe workers have already finished
    /*if (current_state == RUNNING) {

      LockIterationSemaphores();

      int current_min_iteration = GetExtremeCurrentIteration(MIN_EXTREME);
      if (current_min_iteration == max_iteration) {
        current_state = STARTED_NOT_RUNNING;
      }

      UnlockIterationSemaphores();
    }*/

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

        int initial_field_info[2];
        initial_field_info[0] = life_field->height_ / (comm_size - 1);
        initial_field_info[1] = life_field->width_;

        int start_peace_index = 0;

        life_field->show_field();

        std::cout << "----------------------\n";

        for (int i = 1; i < comm_size; ++i) {
          if (i == comm_size - 1) {
            initial_field_info[0] += life_field->height_ % (comm_size - 1);
          }
          MPI_Send(&initial_field_info, 2, MPI::INT, i, INITIAL_FIELD_INFO, MPI_COMM_WORLD);

          bool* field_buffer = GetBuffer(life_field, start_peace_index, initial_field_info[0]);

          MPI_Send(field_buffer, initial_field_info[0] * life_field->width_, MPI::BOOL,
                   i, INITIAL_FIELD, MPI_COMM_WORLD);

          start_peace_index += initial_field_info[0];
        }

        std::cout << "SENDED\n";

        //initialize workers' structures
        //InitializeWorkerStructures(workers);

        current_state = STARTED_NOT_RUNNING;
        break;
      }

      case STATUS:
        if (current_state == BEFORE_START || current_state == RUNNING) {
          std::cout << "The system can't show status in this state (" << current_state << ")\n";
          break;
        }

        std::cout << "current_iteration = " << max_iteration << "\n";
        life_field->show_field();
        break;

        /*case STOP:
        {
          if (current_state == BEFORE_START || current_state == STARTED_NOT_RUNNING) {
            cout << "The system isn't running now.\n";
            break;
          }

          LockIterationSemaphores();
          max_iteration = GetExtremeCurrentIteration(MAX_EXTREME);
          UnlockIterationSemaphores();

          current_state = STARTED_NOT_RUNNING;
          break;
        }

        case RUN:
        {
          string steps_number_string;
          cin >> steps_number_string;

          if (current_state == BEFORE_START) {
            cout << "You can't run system before start.\n";
            break;
          }

          if (current_state == RUNNING) {
            cout << "the system is already running.\n";
            break;
          }

          int steps_number;

          try {
            steps_number = std::stoi(steps_number_string);
          } catch(std::invalid_argument& e) {
            cout << "Enter correct NUMBER of steps, please.\n";
            break;
          } catch (std::out_of_range &e) {
            cout << "Too big value of number of steps.\n";
            break;
          }

          if (!is_workers_initialized) {

            max_iteration += steps_number;
            CreateWorkers(workers, life_field);
            is_workers_initialized = true;

          } else {
            RerunWorkers(steps_number);
          }

          current_state = RUNNING;
          break;
        }

        case QUIT:
          StopWorkers();
          ReleaseResources(workers);
          break;

        case WRONG_COMMAND:
          cout << "You've entered wrong command. Try again, please.\n";
          break;
      }*/
    }
  //}
}





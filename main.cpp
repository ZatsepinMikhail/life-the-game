#include "worker.h"
#include "master.h"

using namespace std;

int main() {

  bool game_finished = false;
  StateType current_state = BEFORE_START;

  std::string command;
  Field* life_field;
  CommandType current_command;

#pragma omp parallel num_threads(2)
  {
    while (!game_finished) {
#pragma omp master
      {
        cout << "$: ";
        cin >> command;
        current_command = ParseCommand(command);
      }

    //maybe workers have already finished
#pragma omp master
      {
        if (current_state == RUNNING) {
#pragma omp critical(max_iteration)
          {
            int current_min_iteration = GetExtremeCurrentIteration(MIN_EXTREME);
            if (current_min_iteration == max_iteration) {
              current_state = STARTED_NOT_RUNNING;
            }
          }
        }
      }

      switch (current_command) {
        case START: {

          std::string field_info_string;
          std::string workers_number_string;

#pragma omp master
          {
            cin >> field_info_string;
            cin >> workers_number_string;
          }

          if (current_state == STARTED_NOT_RUNNING) {
#pragma omp master
            {
              cout << "The system has already started\n";
            }
            break;
          }

          if (current_state == RUNNING) {
#pragma omp master
            {
              cout << "The system is already running, you can't start it.\n";
            }
            break;
          }

          //create field
          try {
#pragma omp master
            {
              life_field = new Field(field_info_string);
            }
          } catch (std::invalid_argument &e) {
#pragma omp master
            {
              cout << e.what() << "\n";
            }
            break;
          }

          //get threads number
          try {
#pragma omp master
            {
              workers_number = std::atoi(workers_number_string.c_str());
            }

            if (workers_number <= 0) {
#pragma omp master
              {
                cout << "The number of workers can't be negative or zero. Enter correct value, please.\n";
              }
              break;
            }

            if (workers_number * 4 > life_field->height_) {
#pragma omp master
              {
                cout <<
                "For correctness and profit the height of field should be at least 4 times bigger than the number of workers.\n";
              }
              break;
            }

          } catch (std::invalid_argument &e) {
#pragma omp master
            {
              cout << "Enter correct NUMBER of thread workers, please.\n";
            }
            break;
          }

#pragma omp master
          {
            worker_iterations.resize(workers_number, 0);
            current_state = STARTED_NOT_RUNNING;
          }
          break;
        }

        case STATUS: {
          if (current_state == BEFORE_START || current_state == RUNNING) {
#pragma omp master
            {
              cout << "The system can't show status in this state (" << current_state << ")\n";
            }
            break;
          }

#pragma omp master
          {
            cout << "current_iteration = " << max_iteration << "\n";
            life_field->show_field();
          }
          break;
        }


        case STOP: {
          if (current_state == BEFORE_START || current_state == STARTED_NOT_RUNNING) {
#pragma omp master
            {
              cout << "The system isn't running now.\n";
            }
            break;
          }

#pragma omp critical(max_iteration)
          {
            max_iteration = GetExtremeCurrentIteration(MAX_EXTREME);
            current_state = STARTED_NOT_RUNNING;
          }

          break;
        }

        case RUN: {
          string steps_number_string;
#pragma omp master
          {
            cin >> steps_number_string;
          }

          if (current_state == BEFORE_START) {
#pragma omp master
            {
              cout << "You can't run system before start.\n";
            }
            break;
          }

          if (current_state == RUNNING) {
#pragma omp master
            {
              cout << "the system is already running.\n";
            }
            break;
          }

          int steps_number;

          try {
#pragma omp master
            {
              steps_number = std::atoi(steps_number_string.c_str());
            }
          } catch (std::invalid_argument &e) {
#pragma omp master
            {
              cout << "Enter correct NUMBER of steps, please.\n";
            }
            break;
          } catch (std::out_of_range &e) {
#pragma omp master
            {
              cout << "Too big value of number of steps.\n";
            }
            break;
          }

#pragma omp master
          {
            max_iteration = steps_number;
            current_state = RUNNING;
          }

          RunWorkers(life_field);
          break;
        }

        case QUIT: {
#pragma omp master
          {
            StopWorkers();
            game_finished = true;
          }
          break;
        }

        case WRONG_COMMAND: {
#pragma omp master
          {
            cout << "You've entered wrong command. Try again, please.\n";
          }
          break;
        }
      }
    }
  }
  return 0;
}
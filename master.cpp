#include "worker.h"

using namespace std;

enum CommandType {
  START,
  STATUS,
  RUN,
  STOP,
  QUIT,
  WRONG_COMMAND
};

enum StateType {
  BEFORE_START,
  STARTED_NOT_RUNNING,
  RUNNING
};

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


void LockIterationSemaphores() {
  for (int i = 0; i < workers_number; ++i) {
    sem_wait(&iteration_semaphores[i]);
  }
}

void UnlockIterationSemaphores() {
  for (int i = workers_number - 1; i >= 0; --i) {
    sem_post(&iteration_semaphores[i]);
  }
}

//use only with LockIterationSemaphores() and UnlockIterationSemaphores()
int GetMaxCurrentIteration() {
  int current_max_iteration = 0;
  for (int i = 0; i < workers_number; ++i) {
    if (worker_iterations[i] > current_max_iteration) {
      current_max_iteration = worker_iterations[i];
    }
  }
  return current_max_iteration;
}


//use only with LockIterationSemaphores() and UnlockIterationSemaphores()
int GetMinCurrentIteration() {
  int current_min_iteration = max_iteration;
  for (int i = 0; i < workers_number; ++i) {
    if (worker_iterations[i] < current_min_iteration) {
      current_min_iteration = worker_iterations[i];
    }
  }
  return current_min_iteration;
}


void InitializeWorkerStructures(vector<pthread_t>& workers) {
  workers.resize(workers_number);
  border_mutexes.resize(workers_number, PTHREAD_MUTEX_INITIALIZER);
  border_cond_variables.resize(workers_number, PTHREAD_COND_INITIALIZER);
  worker_states.resize(workers_number, 0);
  worker_iterations.resize(workers_number, 0);
  iteration_semaphores.resize(workers_number);
  for (int i = 0; i < workers_number; ++i) {
    sem_init(&iteration_semaphores[i], 0, 1);
  }
}


void CreateWorkers(vector<pthread_t>& workers, Field* life_field) {
  for (int i = 0; i < workers_number; ++i) {
    WorkerFuncArg* arg = new WorkerFuncArg();
    arg->field = life_field;
    arg->id = i;
          
    pthread_create(&workers[i], NULL, WorkerFunction, (void*) arg);
  }
}


void RerunWorkers(int steps_number) {
  LockIterationSemaphores();

  pthread_mutex_lock(&game_finished_mutex);

  max_iteration += steps_number;
  pthread_cond_broadcast(&game_finished_cond_variable);

  pthread_mutex_unlock(&game_finished_mutex);

  UnlockIterationSemaphores();
}


void StopWorkers() {
  pthread_mutex_lock(&game_finished_mutex); 

  game_finished = true;
  pthread_cond_broadcast(&game_finished_cond_variable);

  pthread_mutex_unlock(&game_finished_mutex);
}


void ReleaseResources(vector<pthread_t>& workers) {
  for (int i = 0; i < workers_number; ++i) {
    pthread_join(workers[i], NULL);
    pthread_mutex_destroy(&border_mutexes[i]);
  }
  pthread_mutex_destroy(&game_finished_mutex);
}



int main() {

  bool is_workers_initialized = false;

  vector<pthread_t> workers;
  StateType current_state = BEFORE_START;

  std::string command;
  Field* life_field;

  while(!game_finished) {
    cout << "$: ";
    cin >> command;
    CommandType current_command = ParseCommand(command);

    //maybe workers have already finished
    if (current_state == RUNNING) {

      LockIterationSemaphores();

      int current_min_iteration = GetMinCurrentIteration();
      if (current_min_iteration == max_iteration) {
        current_state = STARTED_NOT_RUNNING;
      }

      UnlockIterationSemaphores();
    }

    switch(current_command) {
      case START: 
      {
        std::string field_info_string;
        std::string workers_number_string;
        
        cin >> field_info_string;
        cin >> workers_number_string;

        if (current_state == STARTED_NOT_RUNNING) {
          cout << "The system has already started\n";
          break;
        }

        if (current_state == RUNNING) {
          cout << "The system is already running, you can't start it.\n";
          break;
        }

        //create field
        try {
          life_field = new Field(field_info_string);
        } catch (std::invalid_argument& e) {
          cout << e.what() << "\n";
          break;
        }

        //get threads number
        try {
          workers_number = std::stoi(workers_number_string);
          if (workers_number <= 0) {
            cout << "The number of workers can't be negative or zero. Enter correct value, please.\n";
            break;
          }
          if (workers_number * 4 > life_field->height_) {
            cout << "For correctness and profit the height of field should be at least 4 times bigger than the number of workers.\n";  
            break;
          }
        } catch (std::invalid_argument& e) {
          cout << "Enter correct NUMBER of thread workers, please.\n";
          break;
        }
        
        //initialize workers' structures
        InitializeWorkerStructures(workers);

        current_state = STARTED_NOT_RUNNING;
        break;
      }

      case STATUS:
        if (current_state == BEFORE_START || current_state == RUNNING) {
          cout << "The system can't show status in this state (" << current_state << ")\n";
          break;
        }
        cout << "current_iteration = " << max_iteration << "\n";
        life_field->show_field();
        break;

      case STOP:
      {
        if (current_state == BEFORE_START || current_state == STARTED_NOT_RUNNING) {
          cout << "The system isn't running now.\n";
          break;
        }
        
        LockIterationSemaphores();
        max_iteration = GetMaxCurrentIteration();
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
    }
  }
  return 0;
}
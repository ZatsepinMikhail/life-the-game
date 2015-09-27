#include "field.h"

Field::Field(string field_info) {

  if (field_info.find('/') != string::npos) {
    file_field_init(field_info);
  } else {

    int x_pos = field_info.find('x');
    if (x_pos >= field_info.size() - 1 || x_pos == string::npos) {
      throw std::invalid_argument("Incorrect input format of width and height.");
    }
    string height_string = field_info.substr(0, x_pos);
    string width_string = field_info.substr(x_pos + 1, field_info.size());

    height_ = atoi(height_string.c_str());
    width_ = atoi(width_string.c_str());
    if (height_ <= 0 || width_ <= 0) {
      throw std::invalid_argument("Width and height of field should be positive.");
    }

    random_field_init();
  }
}

void Field::show_field() {
  for (int i = 0; i < height_; ++i) {
    for (int j = 0; j < width_; ++j) {
      if (field_[i][j]) {
        std::cout << "*";
      } else {
        std::cout << ".";
      }
    }
    std::cout << "\n";
  }
}

void Field::random_field_init() {
  vector<bool> row(width_);
  field_.resize(height_, row);

  srand(time(NULL));

  for (size_t i = 0; i < height_; ++i) {
    for (size_t j = 0; j < width_; ++j) {
      field_[i][j] = rand() % 2;
    }
  }
}

void Field::file_field_init(string filename) {
  std::ifstream fin(filename.data());
  if (fin.fail()) {
    throw std::invalid_argument("This file can't be openned.");
  }
  string one_string;
  vector<bool> one_row;
  int current_size = 0;

  while(!(fin >> one_string).eof()) {

    for (int i = 0; i < one_string.size(); ++i) {

      //only 1 or 0 can be at even positions
      if (i % 2 == 0) {
        switch(one_string[i]) {
          case '1':
            one_row.push_back(1);
            break;
          case '0':
            one_row.push_back(0);
            break;
          default:
            throw std::invalid_argument("The format of file is wrong.");
        }
      } else if (i % 2 == 1 && one_string[i] != ',') {
        throw std::invalid_argument("The format of file is wrong.");
      }
    }

    //check for rectangle
    if (field_.size() == 0) {
      current_size = one_row.size();
    } else if (current_size != one_row.size()) {
      throw std::invalid_argument("Field in file isn't rectangle.");
    }
    field_.push_back(one_row);
    one_row.clear();
  }

  if (field_.size() < 3 || current_size < 3) {
    throw std::invalid_argument("Field must be 3x3 at least.");
  }
  height_ = field_.size();
  width_ = current_size;
}

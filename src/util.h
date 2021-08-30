#pragma once

#include <vector>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>

using namespace std;

template<typename T> ostream& operator<<(ostream& out, const vector<T>& v)
{
  out << "{";
  size_t last = v.size() - 1;
  for (size_t i = 0; i < v.size(); ++i) {
    out << fixed << setprecision(9) << v[i];
    if (i != last)
      out << ", ";
  }
  out << "}";
  return out;
}

inline bool file_exists(const string& fname)
{
    struct stat buffer;
    return (stat(fname.c_str(), &buffer) == 0);
}

inline size_t file_size(const char* filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    return in.tellg();
}

static string read_file(const string& path)
{
    ifstream input_file(path);
    string pre = "Could not open the file - '";
    string post = "'";
    if (!input_file.is_open()) {
        cerr << pre << path << post << endl;
        exit(EXIT_FAILURE);
    }
    return string{istreambuf_iterator<char>{input_file}, {}};
}

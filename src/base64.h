#pragma once

#include <string>

using std::string;

class b64
{
private:
  string b64_idx = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "abcdefghijklmnopqrstuvwxyz"
                   "0123456789+/";

public:
  b64() {}
  b64(const string& b64_chars) : b64_idx(b64_chars) {}
  inline bool is_b64(char c) { return (isalnum(c) || (c == '+') || (c == '/')); }
  string encode(const char* bytes);
  string decode(const string& encoded);
  string idx() { return b64_idx; }
};

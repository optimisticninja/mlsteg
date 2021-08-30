#pragma once

#include <numeric>

template<typename T> T sech(T x)
{
  T sh = 1.0 / cosh(x);
  return sh * sh;
};

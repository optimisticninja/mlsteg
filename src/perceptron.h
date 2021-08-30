#pragma once

#include <iostream>
#include <random>
#include <vector>

#include "maths.h"

using namespace std;

template<typename T = float> class perceptron
{
private:
  vector<T> _weights;
  T (*activation)(T);
  T _output;
  T _delta;
  bool biased;

public:
  T (*derivative)(T);
  perceptron(size_t num_inputs, T (*activation)(T x) = tanh, T (*derivative)(T x) = sech<T>,
             bool biased = true)
      : activation(activation), biased(biased), derivative(derivative)
  {
    static default_random_engine gen;
    static uniform_real_distribution<T> dis(0.0, 1.0);
    for (size_t weight_idx = 0; weight_idx < num_inputs + biased; weight_idx++)
      _weights.push_back(dis(gen));
  }

  T activate(const vector<T>& inputs)
  {
    _output = activation(inner_product(_weights.begin(), _weights.end() - biased, inputs.begin(), 0.0)) +
              (biased ? _weights[_weights.size() - 1] : 0);
    _delta = derivative(_output);
    return _output;
  }

  T train_one(const vector<T>& inputs, T expected, T learning_rate = .01)
  {
    auto y = activate(inputs);
    auto error = expected - y;
    auto size = _weights.size();
    for (size_t weight_idx = 0; weight_idx < size - biased; weight_idx++)
      _weights[weight_idx] += learning_rate * error * inputs[weight_idx];
    if (biased)
      _weights[size - 1] += learning_rate * error;
    return error;
  }

  void train(vector<vector<T>>& samples, vector<T> expected, size_t iterations, T learning_rate = .01)
  {
    for (size_t i = 0; i < iterations; i++)
      for (size_t sample_idx = 0; sample_idx < samples.size(); sample_idx++)
        cout << "iter=" << i << ", lrate=" << learning_rate
             << ", error=" << train_one(samples[sample_idx], expected[sample_idx], learning_rate) << "\r";
    cout << endl;
  }

  T weight(size_t index) { return _weights[index]; }
  void weight(size_t index, T x) { _weights[index] = x; }
  vector<T> weights() { return _weights; }
  void weights(const vector<T>& weights) { _weights = weights; }
  T output() { return _output; }
  T delta() { return _delta; }
  void delta(T x) { _delta = x; }
};

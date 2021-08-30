#pragma once

#include <vector>

#include "perceptron.h"

using namespace std;

template<typename T> class bpnn
{
private:
  vector<vector<perceptron<T>>> _net;

public:
  bpnn(const vector<size_t>& shape)
  {
    for (size_t layer_idx = 0; layer_idx < shape.size() - 1; layer_idx++) {
      vector<perceptron<T>> layer;
      for (size_t j = 0; j < shape[layer_idx + 1]; j++)
        layer.push_back(perceptron<T>(shape[layer_idx]));
      _net.push_back(layer);
    }
  }

  vector<T> forward(const vector<T>& inputs)
  {
    vector<T> x(inputs.begin(), inputs.end());
    for (auto& layer : _net) {
      vector<T> layer_outs;
      for (auto& neuron : layer)
        layer_outs.push_back(neuron.activate(x));
      x = layer_outs;
    }
    return x;
  }

  void backward(const vector<T>& expected)
  {
    for (int layer_idx = _net.size() - 1; layer_idx >= 0; --layer_idx) {
      vector<T> errors;
      if (layer_idx != (int) (_net.size() - 1)) {
        for (int neuron_idx = 0; neuron_idx < (int) _net[layer_idx].size(); neuron_idx++) {
          T error = 0.0;
          for (auto& neuron : _net[layer_idx + 1])
            error += (neuron.weight(neuron_idx) * neuron.delta());
          errors.push_back(error);
        }
      } else {
        for (size_t neuron_idx = 0; neuron_idx < _net[layer_idx].size(); neuron_idx++)
          errors.push_back(expected[neuron_idx] - _net[layer_idx][neuron_idx].output());
      }

      for (size_t neuron_idx = 0; neuron_idx < _net[layer_idx].size(); neuron_idx++)
        _net[layer_idx][neuron_idx].delta(errors[neuron_idx] * _net[layer_idx][neuron_idx].derivative(
                                                                   _net[layer_idx][neuron_idx].output()));
    }
  }

  void update_weights(const vector<T>& inputs, T learning_rate)
  {
    auto x = inputs;
    for (size_t layer = 0; layer < _net.size(); layer++) {
      if (layer != 0) {
        x = vector<T>();
        for (auto& neuron : _net[layer - 1])
          x.push_back(neuron.output());
      }

      for (auto& neuron : _net[layer]) {
        for (size_t input_idx = 0; input_idx < x.size(); input_idx++)
          neuron.weight(input_idx, neuron.weight(input_idx) + learning_rate * neuron.delta() * x[input_idx]);
        neuron.weight(x.size() + 1, neuron.weight(x.size() + 1) + learning_rate * neuron.delta());
      }
    }
  }

  T train_one(const vector<T>& inputs, vector<T> expected, T learning_rate)
  {
    auto outputs = forward(inputs);
    backward(expected);
    update_weights(inputs, learning_rate);
    T sum = 0;
    for (size_t output_idx = 0; output_idx < expected.size(); output_idx++)
      sum += pow(expected[output_idx] - outputs[output_idx], 2);
    return sum;
  }

  void train(const vector<vector<T>>& inputs, const vector<vector<T>>& expected, size_t iterations = 3000,
             T lrate = 0.01)
  {
    for (size_t iter = 0; iter < iterations; iter++) {
      T sum_error = 0;
      for (size_t sample_idx = 0; sample_idx < inputs.size(); sample_idx++) {
        sum_error += train_one(inputs[sample_idx], expected[sample_idx], lrate);
        cout << ">iter=" << iter << ", lrate=" << fixed << setprecision(3) << lrate << ", error=" << fixed
             << setprecision(3) << sum_error << "\r";
      }
    }
    cout << endl;
  }

  vector<vector<perceptron<T>>>& net() { return _net; };
  void net(vector<vector<perceptron<T>>>& net) { _net = net; };
};

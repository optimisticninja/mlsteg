#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace std;

template<typename T> ostream& operator<<(ostream& out, const vector<T>& v)
{
  out << "{";
  size_t last = v.size() - 1;
  for (size_t i = 0; i < v.size(); ++i) {
    out << v[i];
    if (i != last)
      out << ", ";
  }
  out << "}";
  return out;
}

template<typename T> T sech(T x)
{
  T sh = 1.0 / cosh(x);
  return sh * sh;
};

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
  T output() { return _output; }
  T delta() { return _delta; }
  void delta(T x) { _delta = x; }
};

template<typename T> class bpnn
{
private:
  vector<vector<perceptron<T>>> net;

public:
  bpnn(const vector<size_t>& shape)
  {
    for (size_t layer_idx = 0; layer_idx < shape.size() - 1; layer_idx++) {
      vector<perceptron<T>> layer;
      for (size_t j = 0; j < shape[layer_idx + 1]; j++)
        layer.push_back(perceptron<T>(shape[layer_idx]));
      net.push_back(layer);
    }
  }

  vector<T> forward(const vector<T>& inputs)
  {
    vector<T> x(inputs.begin(), inputs.end());
    for (auto& layer : net) {
      vector<T> layer_outs;
      for (auto& neuron : layer)
        layer_outs.push_back(neuron.activate(x));
      x = layer_outs;
    }
    return x;
  }

  void backward(const vector<T>& expected)
  {
    for (int layer_idx = net.size() - 1; layer_idx >= 0; --layer_idx) {
      vector<T> errors;
      if (layer_idx != (int) (net.size() - 1)) {
        for (int neuron_idx = 0; neuron_idx < (int) net[layer_idx].size(); neuron_idx++) {
          T error = 0.0;
          for (auto& neuron : net[layer_idx + 1])
            error += (neuron.weight(neuron_idx) * neuron.delta());
          errors.push_back(error);
        }
      } else {
        for (size_t neuron_idx = 0; neuron_idx < net[layer_idx].size(); neuron_idx++)
          errors.push_back(expected[neuron_idx] - net[layer_idx][neuron_idx].output());
      }

      for (size_t neuron_idx = 0; neuron_idx < net[layer_idx].size(); neuron_idx++)
        net[layer_idx][neuron_idx].delta(
            errors[neuron_idx] * net[layer_idx][neuron_idx].derivative(net[layer_idx][neuron_idx].output()));
    }
  }

  void update_weights(const vector<T>& inputs, T learning_rate)
  {
    auto x = inputs;
    for (size_t layer = 0; layer < net.size(); layer++) {
      if (layer != 0) {
        x = vector<T>();
        for (auto& neuron : net[layer - 1])
          x.push_back(neuron.output());
      }

      for (auto& neuron : net[layer]) {
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
        cout << ">iter=" << iter << ", lrate=" << lrate << ", error=" << sum_error << "\r";
      }
    }
    cout << endl;
  }
};

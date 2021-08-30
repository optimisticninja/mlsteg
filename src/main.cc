#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <vector>

#include <boost/program_options.hpp>

#include <crypto++/cryptlib.h>
// #include <crypto++/eax.h>
#include <crypto++/files.h>
#include <crypto++/hex.h>
#include <crypto++/modes.h>
#include <crypto++/osrng.h>
#include <crypto++/pwdbased.h>
#include <crypto++/rijndael.h>
#include <crypto++/sha.h>

#include <jsoncpp/json/json.h>

#include "base64.h"
#include "bpnn.h"
// #include "compression.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace CryptoPP;

namespace po = boost::program_options;

enum { SUCCESS, ERROR_IN_COMMAND_LINE, ERROR_UNHANDLED_EXCEPTION, ERROR_INVALID_JSON };

class VectorSink : public Bufferless<Sink>
{
public:
  VectorSink(vector<uint8_t>& out) : _out(&out) {}

  size_t Put2(const byte* inString, size_t length, int /*messageEnd*/, bool /*blocking*/)
  {
    _out->insert(_out->end(), inString, inString + length);
    return 0;
  }

private:
  vector<uint8_t>* _out;
};

static void help(const po::options_description& desc)
{
  string msg = "mlsteg - hide messages in neural network weights";
  cout << msg << endl << desc << endl;
}

void encrypt(const string& password, const string& data, vector<u8>& dest)
{
  cerr << "[*] Encrypting data..." << endl;
  char purpose = 0;
  SecByteBlock derived(32);

  PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
  pbkdf.DeriveKey(derived, sizeof(derived), purpose, (byte*) password.data(), password.size(), NULL, 0, 1024,
                  0.0f);
  vector<u8> data_vec(data.begin(), data.end());

  try {
    CBC_Mode<AES>::Encryption e;
    e.SetKeyWithIV(derived.data(), 16, derived.data() + 16, 16);
    //       string s(plaintext.begin(), plaintext.end())
    StringSource ss(string(data_vec.begin(), data_vec.end()), true,
                    new StreamTransformationFilter(e, new VectorSink(dest)));
  } catch (const Exception& e) {
    cerr << e.what() << endl;
    exit(1);
  }
}

void dump_network(bpnn<float>& nn, const string& encoded, const string& output_file)
{
  Json::StreamWriterBuilder builder;
  const unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  Json::Value network;
  Json::Value layers(Json::arrayValue);
  for (auto& layer : nn.net()) {
    Json::Value l(Json::arrayValue);
    for (auto& neuron : layer) {
      Json::Value n;
      Json::Value w(Json::arrayValue);
      vector<float> weights = neuron.weights();
      for (auto weight : weights)
        w.append(weight);
      n["weights"] = w;
      l.append(n);
    }
    layers.append(l);
  }

  network["layers"] = layers;
  network["outputs"] = (unsigned) encoded.length();
  network["activation"] = "tanh";
  network["derivative"] = "sech";

  if (output_file != "") {
    ofstream ofs(output_file);
    writer->write(network, &ofs);
    ofs.close();
  } else {
    writer->write(network, &cout);
  }
}

void dump_mapping(const map<char, float> mapping)
{
  Json::Value mappings;
  Json::StreamWriterBuilder builder;
  const unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

  for (auto& t : mapping) {
    Json::Value mapping(Json::intValue);
    mapping = t.second;
    mappings[string(1, t.first)] = t.second;
  }

  ofstream ofs("mappings.json");
  writer->write(mappings, &ofs);
  ofs.close();
}

void dump_magic_inputs(const vector<float>& inputs)
{
  Json::StreamWriterBuilder builder;
  const unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  Json::Value magic_inputs(Json::arrayValue);
  ofstream ofs = ofstream("inputs.json");
  for (auto& input : inputs)
    magic_inputs.append(input);
  writer->write(magic_inputs, &ofs);
  ofs.close();
}

static void steg_data(const string& password, const string& input_file, const string& output_file,
                      __attribute__((unused)) bool disable_compression)
{
  string data;
  stringstream ss;
  vector<u8> compressed;
  vector<u8> encrypted;

  if (input_file != "") {
    if (file_exists(input_file)) {
      string pre = "[*] File size: ";
      string post = " bytes";
      cerr << pre << file_size(input_file.c_str()) << post << endl;
      data = read_file(input_file);
    } else {
      string pre = "ERROR: File '";
      string post = "' does not exist.\n";
      cerr << pre << input_file << post;
      exit(ERROR_IN_COMMAND_LINE);
    }
  } else {
    cerr << "ERROR: Need input file for network JSON" << endl;
    exit(ERROR_IN_COMMAND_LINE);
  }

  //   if (!disable_compression) {
  //     string compressing = "[*] Compressing...";
  //     cerr << compressing << endl;
  //     lzma::compress(data, compressed);
  //     cout << compressed << endl;
  //   }

  if (password != "") {
    encrypt(password, data, encrypted);
  }

  string encoding = "[*] Encoding network...";

  // B64 encode message
  b64 base64;
  cerr << encoding << endl;
  string encoded =
      password != "" ? base64.encode(encrypted) : base64.encode(vector<u8>(data.begin(), data.end()));
  //           : base64.encode((disable_compression ? vector<u8>(data.begin(), data.end()) : compressed));
  string alphabet = base64.idx();
  random_shuffle(alphabet.begin(), alphabet.end());

  // Map B64 characters to floats
  map<char, float> mapping;
  size_t i = 0;
  for (char c : alphabet)
    mapping[c] = i++;
  dump_mapping(mapping);

  // Create expected data for neural net
  vector<float> expected;
  for (char c : encoded)
    expected.push_back(mapping[c] / 100);

  // Shape of the neural net
  vector<size_t> shape = {16, 10, 24, encoded.length()};

  // Create magic inputs
  static default_random_engine gen;
  static uniform_real_distribution<float> dis2(0, 1);
  // FIXME: Why are random values all zero?
  vector<float> inputs(shape[0], dis2(gen));
  dump_magic_inputs(inputs);

  // Create sample data
  vector<vector<float>> samples = {inputs};
  vector<vector<float>> sample_expected = {expected};

  // Train magic input sample to expected data
  bpnn<float> nn(shape);
  nn.train(samples, sample_expected, 10000);
  dump_network(nn, encoded, output_file);
}

bpnn<float> decode_network(const string& data)
{
  Json::Value network;
  Json::Reader reader;
  size_t num_outputs = 0;
  vector<vector<perceptron<float>>> net;
  if (reader.parse(data, network)) {
    num_outputs = network["outputs"].asLargestInt();
    for (auto& layer : network["layers"]) {
      vector<perceptron<float>> l;
      for (auto& neuron : layer) {
        vector<float> w;
        for (auto& weight : neuron["weights"])
          w.push_back(weight.asFloat());
        perceptron<float> p(w.size() - 1);
        p.weights(w);
        l.push_back(p);
      }
      net.push_back(l);
    }
  } else {
    cerr << "ERROR: Invalid network JSON file" << endl;
    exit(ERROR_INVALID_JSON);
  }

  // build network
  vector<size_t> shape;
  for (size_t i = 0; i < net.size(); i++)
    shape.push_back(net[i].size());
  shape.push_back(num_outputs);
  bpnn<float> nn(shape);
  nn.net(net);
  return nn;
}

vector<float> read_inputs(const string& magic_inputs_file)
{
  Json::Reader reader;
  Json::Value magic_inputs(Json::arrayValue);
  string magic_json = "";
  vector<float> inputs;

  if (magic_inputs_file != "") {
    if (file_exists(magic_inputs_file)) {
      magic_json = read_file(magic_inputs_file);
    }
  } else {
    cerr << "ERROR: Magic inputs file didn't exist" << endl;
    exit(ERROR_IN_COMMAND_LINE);
  }

  if (reader.parse(magic_json, magic_inputs)) {
    for (auto& in : magic_inputs)
      inputs.push_back(in.asFloat());
  }

  return inputs;
}

map<float, char> read_mapping(const string& mapping_file)
{
  Json::Value mappings;
  ifstream ifs;
  ifs.open(mapping_file);
  Json::CharReaderBuilder builder;
  JSONCPP_STRING errs;
  if (!parseFromStream(builder, ifs, &mappings, &errs)) {
    cerr << "ERROR: Invalid JSON - " << errs << endl;
    exit(ERROR_INVALID_JSON);
  }

  map<float, char> mapping;
  for (auto& member : mappings.getMemberNames()) {
    mapping[mappings[member].asFloat()] = member[0];
  }
  return mapping;
}

void decrypt(const string& data, vector<u8>& dest, const string& password)
{
  string decoding = "[*] Decrypting data...";
  cerr << decoding << endl;
  char purpose = 0; // unused by Crypto++

  // 32 bytes of derived material. Used to key the cipher.
  // 16 bytes are for the key, and 16 bytes are for the iv.
  SecByteBlock derived(32);

  PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
  pbkdf.DeriveKey(derived, sizeof(derived), purpose, (byte*) password.data(), password.size(), NULL, 0, 1024,
                  0.0f);

  try {
    CBC_Mode<AES>::Decryption d;
    d.SetKeyWithIV(derived.data(), 16, derived.data() + 16, 16);
    StringSource ss(data, true, new StreamTransformationFilter(d, new VectorSink(dest)));
  } catch (const Exception& e) {
    cerr << e.what() << endl;
    exit(1);
  }
}

static void unsteg_data(const string& password, const string& input_file,
                        const string& magic_inputs_file = "inputs.json",
                        const string& mapping_file = "mappings.json", const string& output_file = "unstegged",
                        __attribute__((unused)) bool disable_compression = false)
{
  string data = "";

  if (input_file != "") {
    if (file_exists(input_file)) {
      data = read_file(input_file);
    } else {
      string pre = "ERROR: File '";
      string post = "' does not exist.\n";
      cerr << pre << input_file << post;
      exit(ERROR_IN_COMMAND_LINE);
    }
  } else {
    cerr << "ERROR: Network topology file not provided" << endl;
    exit(ERROR_IN_COMMAND_LINE);
  }

  if (magic_inputs_file == "") {
    cerr << "ERROR: Magic inputs file not provided" << endl;
    exit(ERROR_IN_COMMAND_LINE);
  }

  // Decode and build network
  string decoding = "[*] Decoding network JSON...";
  cerr << decoding << endl;
  bpnn<float> nn = decode_network(data);

  // Read magic inputs
  vector<float> inputs = read_inputs(magic_inputs_file);

  // Feed inputs through network
  vector<float> outputs = nn.forward(inputs);

  // Map output to characters (how do we provide to CLI?)
  map<float, char> mapping = read_mapping(mapping_file);

  // Decode network outputs
  stringstream ss;
  for (float f : outputs) {
    int round = f * 100 + .5;
    for (auto& it : mapping)
      if (it.first == round)
        ss << it.second;
  }

  // B64 decode
  b64 base64;
  string tmp = ss.str();
  string decoded = base64.decode(ss.str());

  // Decrypt
  vector<u8> decrypted;
  if (password != "")
    decrypt(decoded, decrypted, password);

  // decompress

  //   int decompressed_size = 0;
  //   vector<u8> decompressed(data.size() * 2);
  //   //   decrypted.push_back(0);
  //   if (!disable_compression) {
  //     string decompressing = "[*] Decompressing data...";
  //     cerr << decompressing << endl;
  //     decompressed_size = lzma::decompress(decrypted, decompressed);
  //
  //     if (decompressed_size < 1) {
  //       cerr << "ERROR: Failure to decompress data" << endl;
  //       exit(ERROR_UNHANDLED_EXCEPTION);
  //     }
  //   }
  //
  //   cout << decompressed_size << endl;

  if (output_file != "") {
    ofstream ofs(output_file, ios_base::out | ios_base::binary);
    //     if (!disable_compression)
    //       ofs.write(reinterpret_cast<const char*>(decompressed.data()), decompressed_size);
    //     else
    ofs.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
    ofs.close();
  } else {
    string header = "<<< BEGIN RECOVERED MESSAGE >>>";
    string footer = "<<< END RECOVERED MESSAGE >>>";
    cerr << endl << header << endl << endl;
    //     if (!disable_compression)
    //       cout.write(reinterpret_cast<const char*>(decompressed.data()), decompressed_size);
    //     else
    cout.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
    cerr << endl << footer << endl;
  }
}

int main(int argc, char** argv)
{
  bool unsteg = "";
  string password = "";
  string output_file = "";
  string input_file = "";
  string magic_inputs_file = "";
  string mapping_file = "";
  bool disable_compression = false;

  try {
    string options = "mlsteg options";
    string help_switches = "help,h", help_message = "print usage";
    string unsteg_switches = "unsteg,u", unsteg_message = "unsteg message";
    string input_switches = "input,i", input_message = "input file";
    string magic_inputs_switches = "magic,m", magic_input_message = "magic inputs file";
    string mapping_file_switches = "map", mapping_file_message = "mapping";
    string output_switches = "output,o", output_message = "output file";
    string pass_switches = "password,p", pass_message = "encryption password";
    string disable_compression_switches = "disable-compression",
           disable_compression_message = "disable compression";

    po::options_description desc(options);
    // clang-format off
    desc.add_options()(help_switches.c_str(), help_message.c_str())(
        unsteg_switches.c_str(), po::bool_switch(&unsteg), unsteg_message.c_str())(
        input_switches.c_str(), po::value(&input_file), input_message.c_str())(
        magic_inputs_switches.c_str(), po::value(&magic_inputs_file), magic_input_message.c_str())(
        mapping_file_switches.c_str(), po::value(&mapping_file), mapping_file_message.c_str())(
        output_switches.c_str(), po::value(&output_file), output_message.c_str())(
        pass_switches.c_str(), po::value(&password), pass_message.c_str())(
        disable_compression_switches.c_str(), po::bool_switch(&disable_compression), disable_compression_message.c_str());
    // clang-format on

    po::variables_map vm;

    try {
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help") || vm.count("h") || argc == 1) {
        help(desc);
        return SUCCESS;
      }

      po::notify(vm);

      if (unsteg)
        unsteg_data(password, input_file, magic_inputs_file, mapping_file, output_file, disable_compression);
      else
        steg_data(password, input_file, output_file, disable_compression);
    } catch (po::error& e) {
      string pre = "ERROR: ";
      cerr << pre << e.what() << endl << endl;
      cerr << desc << endl;
      return ERROR_IN_COMMAND_LINE;
    }
  } catch (exception& e) {
    string pre = "ERROR: unhandled exception reached the top of main: ";
    string post = ", application will now exit";
    cerr << pre << e.what() << post << endl;
    return ERROR_UNHANDLED_EXCEPTION;
  }

  return SUCCESS;
}

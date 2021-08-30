#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <map>

#include <boost/program_options.hpp>

#include <crypto++/cryptlib.h>
#include <crypto++/rijndael.h>
#include <crypto++/modes.h>
#include <crypto++/files.h>
#include <crypto++/osrng.h>
#include <crypto++/hex.h>

#include "base64.h"
#include "bpnn.h"
#include "util.h"
#include "types.h"
#include "compression.h"


using namespace std;
using namespace CryptoPP;

namespace po = boost::program_options;

enum { SUCCESS, ERROR_IN_COMMAND_LINE, ERROR_UNHANDLED_EXCEPTION };

static void help(const po::options_description& desc)
{
    string msg = "mlsteg - hide messages in neural network weights";
    cout << msg << endl << desc << endl;
}

static void steg_data(const string& password, const string& input_file,
                      const string& output_file, bool disable_compression)
{
    string data;
    stringstream ss;
    string compressed;
    string encrypted;
    string dna;

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
        string header = "<<< BEGIN STEGGED NETWORK JSON (Press CTRL+D when done) >>>\n\n";
        string footer = "<<< END STEGGED NETWORK JSON >>>\n\n";
        cerr << header;
        for (string line; getline(cin, line);)
            ss << line << endl;
        cerr << endl << footer;
        cerr << endl;
    }

    // If read from stdin
    if (data == "")
        data = ss.str();

    string input_data(data.begin(), data.end());

    if (!disable_compression) {
        string compressing = "[*] Compressing...";
        cerr << compressing << endl;
        lzma::compress(input_data, compressed);
    }

    if (password != "") {
            AutoSeededRandomPool prng;
            HexEncoder encoder(new FileSink(cout));

            SecByteBlock key(AES::DEFAULT_KEYLENGTH);
            SecByteBlock iv(AES::BLOCKSIZE);

            prng.GenerateBlock(key, key.size());
            prng.GenerateBlock(iv, iv.size());
            string cipher, recovered;

            cout << "plain text: " << (disable_compression ? input_data : compressed) << endl;

            /*********************************\
            \*********************************/

            try {
                CBC_Mode< AES >::Encryption e;
                e.SetKeyWithIV(key, key.size(), iv);

                StringSource s(disable_compression == true ? input_data : compressed, true, 
                    new StreamTransformationFilter(e,
                        new StringSink(cipher)
                    ) // StreamTransformationFilter
                ); // StringSource
            } catch(const Exception& e) {
                cerr << e.what() << endl;
                exit(1);
            }
            
            cout << "key: ";
            encoder.Put(key, key.size());
            encoder.MessageEnd();
            cout << endl;

            cout << "iv: ";
            encoder.Put(iv, iv.size());
            encoder.MessageEnd();
            cout << endl;

            cout << "cipher text: ";
            encoder.Put((const byte*)&cipher[0], cipher.size());
            encoder.MessageEnd();
            cout << endl;
        //encrypt(encrypted = disable_compression ? input_data : compressed, password);
    }

    string encoding = "[*] Encoding network...";
    b64 base64;
    cerr << encoding << endl;
    // Map b64 characters to floats
    // base64 encode message
    // create expected data
    // create magic inputs
    // train on magic inputs to expected data
    // dump network to JSON file
    

    if (output_file != "") {
        ofstream ofs(output_file);
        ofs << dna;
        ofs.close();
    } else {
        cout << endl << dna << endl;
    }
}

static void unsteg_data(const string& password, const string& input_file,
                        const string& output_file, bool disable_compression)
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
        string header = "<<< BEGIN NETWORK JSON (Press CTRL+D when done) >>>\n\n";
        string footer = "<<< END NETWORK JSON >>>\n\n";
        cout << header;
        for (string line; getline(cin, line);)
            data += line + "\n";
        cout << endl << footer;
    }

    string decoding = "[*] Decoding network JSON...";
    cerr << decoding << endl;
    // parse json
    // build network
    // feed inputs through network
    // map output to characters
    
    // base64 decode
    // decrypt
    // decompress
    string decoded;//= dna64::decode(dna);
    vector<u8> decrypted(decoded.begin(), decoded.end());

    if (password != "") {
        //decrypt(decrypted, password);
    }

    size_t decompressed_size = 0;
    string decompressed;
    if (!disable_compression) {
        string decompressing = "[*] Decompressing data...";
        cerr << decompressing << endl;
        //decompressed = vector<u8>(decrypted.size() * 4);
        decompressed_size = lzma::decompress(decrypted, decompressed);
    }

    if (output_file != "") {
        ofstream ofs(output_file, ios_base::out | ios_base::binary);
        if (!disable_compression)
            ofs.write(reinterpret_cast<const char*>(decompressed.data()), decompressed_size);
        else
            ofs.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
        ofs.close();
    } else {
        string header = "<<< BEGIN RECOVERED MESSAGE >>>";
        string footer = "<<< END RECOVERED MESSAGE >>>";
        cerr << endl << header << endl << endl;
        if (!disable_compression)
            cout.write(reinterpret_cast<const char*>(decompressed.data()), decompressed_size);
        else
            cout.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
        cerr << endl << footer << endl;
    }
}

int main(int argc, char** argv)
{
    static default_random_engine gen;
    static uniform_int_distribution<size_t> dis(0, 99);
    b64 base64 = b64();
    string alphabet = base64.idx();
    random_shuffle(alphabet.begin(), alphabet.end());
    map<char, float> mapping;
    size_t i = 0;
    for(char c : alphabet)
        mapping[c] = i++;
    string msg = "ohaithisisamessagewithalotofdifferentcharacterzandtransgressionsandthings";
    string message = base64.encode(msg.c_str());
    // encode
    vector<float> expected;
    for (char c : message)
        expected.push_back(mapping[c] / 100);
    vector<size_t> shape = {16, 10, 24, message.length()};
    bpnn<float> nn(shape);
    static uniform_real_distribution<float> dis2(0, 1);
    vector<float> inputs(shape[0], 0);
    vector<vector<float>> samples = { inputs };
    vector<vector<float>> sample_expected  = { expected };
    nn.train(samples, sample_expected, 10000);
    vector<float> outputs = nn.forward(inputs);
    string decoded;
    for (float output: outputs) {
        int round = output * 100 + .5;
        for (auto& it : mapping) {
            if (it.second == round) {
                decoded += it.first;
            }
        }
    }
    cout << base64.decode(decoded);
    
    
    
    
    bool unsteg = "";
    string password = "";
    string output_file = "";
    string input_file = "";
    bool disable_compression = false;

    try {
        string options = "mlsteg options";
        string help_switches = "help,h", help_message = "print usage";
        string unsteg_switches = "unsteg,u", unsteg_message = "unsteg message";
        string input_switches = "input,i", input_message = "input file";
        string output_switches = "output,o", output_message = "output file";
        string pass_switches = "password,p", pass_message = "encryption password";
        string disable_compression_switches = "disable-compression",
               disable_compression_message = "disable compression";

        po::options_description desc(options);
        // clang-format off
        desc.add_options()(help_switches.c_str(), help_message.c_str())(
            unsteg_switches.c_str(), po::bool_switch(&unsteg),
            unsteg_message.c_str())(input_switches.c_str(), po::value(&input_file), input_message.c_str())(
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
                unsteg_data(password, input_file, output_file, disable_compression);
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

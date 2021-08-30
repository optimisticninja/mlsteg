# mlsteg

**Still fixing compression bug**

Hide data in neural networks.

## How it works

For now, a static topology of { 16, 10, 24, compressed message size } is used to train
a neural network to magic inputs that output floats mapped to a serialized character 
format (base64 in this case).

The network will output the following after the stegging process:

1. Network topology/information
2. Character mappings
3. Magic inputs

## Dependencies

* CMake
* Boost (program_options)
* Crypto++
* libjsoncpp

```bash
sudo apt install cmake libboost-dev libcrypto++-dev libjsoncpp-dev
```

## Examples

**network.json**
```json
{
	"layers" : 
	[
		[
			{
				"weights" : 
				[
					0.24869127571582794,
					0.86126887798309326,
					0.44930148124694824,
					0.51862269639968872,
					0.5948939323425293,
					0.48572888970375061,
					0.75408476591110229,
					0.0054831434972584248,
					0.26049011945724487,
					0.16634514927864075,
					0.86984294652938843,
					0.55356234312057495,
					0.8257792592048645,
					0.97545087337493896,
					0.50649058818817139,
					0.69015979766845703,
					0.61930680274963379
				]
			},
			...
        ],
        ...
	]
	"outputs": 299,
	"activation": "tanh",
	"derivation": "sech"
}
```

**mapping.json**
```json
{
	"+" : 24,
	"/" : 43,
	"0" : 23,
	"1" : 1,
	"2" : 50,
	"3" : 60,
	"4" : 40,
	"5" : 41,
	"6" : 47,
	"7" : 9,
	"8" : 57,
	"9" : 31,
	"A" : 19,
	...
}
```

**inputs.json**
```json
[
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06,
	7.825903594493866e-06
]
```

## Usage

```
mlsteg - hide messages in neural network weights
mlsteg options:
  -h [ --help ]          print usage
  -u [ --unsteg ]        unsteg message
  -i [ --input ] arg     input file
  -m [ --magic ] arg     magic inputs file
  --map arg              mapping
  -o [ --output ] arg    output file
  -p [ --password ] arg  encryption password
  --disable-compression  disable compression
```

### Stegging

```bash
$ ./mlsteg -i compile_commands.json -o test.json -p test
[*] File size: 564 bytes
[*] Encoding network...
>iter=9999, lrate=0.010, error=0.0008
```

### Unstegging

```bash
$ ./mlsteg -u -i test.json -m inputs.json --map mappings.json -p test 
[*] Decoding network JSON...
[*] Decrypting data...

<<< BEGIN RECOVERED MESSAGE >>>

[
{
  "directory": "/home/jonesy/git/nn/build/src",
  "command": "/usr/bin/c++  -DBOOST_ALL_NO_LIB -DBOOST_PROGRAM_OPTIONS_DYN_LINK   -Wall -Werror -Wextra -g   -o CMakeFiles/mlsteg.dir/main.cc.o -c /home/jonesy/git/nn/src/main.cc",
  "file": "/home/jonesy/git/nn/src/main.cc"
},
{
  "directory": "/home/jonesy/git/nn/build/src",
  "command": "/usr/bin/c++  -DBOOST_ALL_NO_LIB -DBOOST_PROGRAM_OPTIONS_DYN_LINK   -Wall -Werror -Wextra -g   -o CMakeFiles/mlsteg.dir/base64.cc.o -c /home/jonesy/git/nn/src/base64.cc",
  "file": "/home/jonesy/git/nn/src/base64.cc"
}
]
<<< END RECOVERED MESSAGE >>>
```

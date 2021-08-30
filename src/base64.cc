#include <string>
#include <cstring>

#include "base64.h"


string b64::encode(const char* bytes)
{
    string encoded;
    int i = 0, j = 0;
    unsigned char chunk3b[3], chunk4b[4];
    size_t bytes_len = strlen(bytes);

    while (bytes_len--) {
        chunk3b[i++] = *(bytes++);
        
        if (i == 3) {
            chunk4b[0] = (chunk3b[0] & 0xfc) >> 2;
            chunk4b[1] = ((chunk3b[0] & 0x03) << 4) + ((chunk3b[1] & 0xf0) >> 4);
            chunk4b[2] = ((chunk3b[1] & 0x0f) << 2) + ((chunk3b[2] & 0xc0) >> 6);
            chunk4b[3] = chunk3b[2] & 0x3f;
            for (i = 0; i < 4; i++) encoded += b64_idx[chunk4b[i]];
            i = 0;
        }
    }

    if (i) {
        memset(&chunk3b[i], '\0', 3 - i);
        chunk4b[0] = (chunk3b[0] & 0xfc) >> 2;
        chunk4b[1] = ((chunk3b[0] & 0x03) << 4) + ((chunk3b[1] & 0xf0) >> 4);
        chunk4b[2] = ((chunk3b[1] & 0x0f) << 2) + ((chunk3b[2] & 0xc0) >> 6);
        chunk4b[3] = chunk3b[2] & 0x3f;
        for (j = 0; j < i + 1; j++) encoded += b64_idx[chunk4b[j]];
    }

    return encoded;
}

string b64::decode(const string& encoded)
{
    int encoded_len = encoded.size();
    int encoded_idx = 0;
    int i = 0, j = 0;
    unsigned char chunk3b[3], chunk4b[4];
    string decoded;

    while (encoded_len-- && encoded[encoded_idx] != '=' && is_b64(encoded[encoded_idx])) {
        chunk4b[i++] = encoded[encoded_idx++];
        if (i == 4) {
            for (i = 0; i <4; i++) chunk4b[i] = b64_idx.find(chunk4b[i]);
            chunk3b[0] = (chunk4b[0] << 2) + ((chunk4b[1] & 0x30) >> 4);
            chunk3b[1] = ((chunk4b[1] & 0xf) << 4) + ((chunk4b[2] & 0x3c) >> 2);
            chunk3b[2] = ((chunk4b[2] & 0x3) << 6) + chunk4b[3];
            for (i = 0; i < 3; i++) decoded += chunk3b[i];
            i = 0;
        }
    }

    if (i) {
        memset(&chunk4b[i], 0, 4 - i);
        for (j = 0; j < 4; j++) chunk4b[j] = b64_idx.find(chunk4b[j]);
        chunk3b[0] = (chunk4b[0] << 2) + ((chunk4b[1] & 0x30) >> 4);
        chunk3b[1] = ((chunk4b[1] & 0xf) << 4) + ((chunk4b[2] & 0x3c) >> 2);
        chunk3b[2] = ((chunk4b[2] & 0x3) << 6) + chunk4b[3];
        for (j = 0; (j < i - 1); j++) decoded += chunk3b[j];
    }

    return decoded;
}

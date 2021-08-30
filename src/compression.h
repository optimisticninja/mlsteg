#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include "types.h"

using namespace std;

#include <cassert>
#include <zlib.h>

namespace lzma
{
    void compress(const string& src, string& dest)
    {
        vector<u8> buffer;
        const size_t BUFSIZE = 128 * 1024;
        u8 temp_buffer[BUFSIZE];

        z_stream strm;
        strm.zalloc = 0;
        strm.zfree = 0;
        strm.next_in = (u8*) src.data();
        strm.avail_in = src.size();
        strm.next_out = temp_buffer;
        strm.avail_out = BUFSIZE;

        deflateInit(&strm, Z_BEST_COMPRESSION);

        while (strm.avail_in != 0) {
            int res = deflate(&strm, Z_NO_FLUSH);
            assert(res == Z_OK);
            if (strm.avail_out == 0 && res == Z_OK) {
                buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
                strm.next_out = temp_buffer;
                strm.avail_out = BUFSIZE;
            }
        }

        int deflate_res = Z_OK;
        while (deflate_res == Z_OK) {
            if (strm.avail_out == 0) {
                buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
                strm.next_out = temp_buffer;
                strm.avail_out = BUFSIZE;
            }
            deflate_res = deflate(&strm, Z_FINISH);
        }

        assert(deflate_res == Z_STREAM_END);
        buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
        deflateEnd(&strm);
        dest = string(buffer.begin(), buffer.end());
    }

    size_t decompress(const vector<u8>& src, string& dest)
    {
        z_stream strm;
        strm.total_in = strm.avail_in = src.size();
        strm.next_in = (u8*) src.data();
        strm.next_out = (u8*) dest.data();

        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.total_out = deflateBound(&strm, strm.total_in);
        strm.avail_out = dest.size();

        int err = -1;
        int ret = -1;

        err = inflateInit2(
            &strm, (15 + 32)); // 15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
        if (err == Z_OK) {
            err = inflate(&strm, Z_FINISH);
            if (err == Z_STREAM_END) {
                ret = strm.total_out;
            } else {
                inflateEnd(&strm);
                return err;
            }
        } else {
            inflateEnd(&strm);
            return err;
        }

        inflateEnd(&strm);
        return ret;
    }
} // namespace lzma

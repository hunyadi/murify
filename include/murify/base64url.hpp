/**
 * murify: Efficient in-memory compression for URLs
 * @see https://github.com/hunyadi/murify
 *
 * Copyright (c) 2024 Levente Hunyadi
 *
 * This work is licensed under the terms of the MIT license.
 * For a copy, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <string>
#include <string_view>
#include <cstddef>

namespace murify
{
    struct base64
    {
        static bool encode(const std::basic_string_view<std::byte>& input, std::string& output)
        {
            static constexpr char encoding_table[] = {
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_' };

            std::size_t in_len = input.size();
            std::size_t triplets = in_len / 3;
            std::size_t spare = in_len % 3;
            std::size_t out_len = (4 * triplets) + (spare > 0 ? spare + 1 : 0);

            output.clear();
            output.resize(out_len);

            char* p = output.data();
            unsigned a;
            unsigned b;
            unsigned c;

            std::size_t i = 0;
            for (std::size_t k = 0; k < triplets; i += 3, ++k) {
                a = static_cast<unsigned>(input[i]);
                b = static_cast<unsigned>(input[i + 1]);
                c = static_cast<unsigned>(input[i + 2]);
                *p++ = encoding_table[(a >> 2) & 0x3f];
                *p++ = encoding_table[((a & 0x3) << 4) | ((b >> 4) & 0xf)];
                *p++ = encoding_table[((b & 0xf) << 2) | ((c >> 6) & 0x3)];
                *p++ = encoding_table[c & 0x3f];
            }
            switch (spare) {
            case 1:
                a = static_cast<unsigned>(input[i]);
                *p++ = encoding_table[(a >> 2) & 0x3f];
                *p++ = encoding_table[(a & 0x3) << 4];
                break;
            case 2:
                a = static_cast<unsigned>(input[i]);
                b = static_cast<unsigned>(input[i + 1]);
                *p++ = encoding_table[(a >> 2) & 0x3f];
                *p++ = encoding_table[((a & 0x3) << 4) | ((b >> 4) & 0xf)];
                *p++ = encoding_table[((b & 0xf) << 2)];
                break;
            default:  // case 0:
                break;
            }

            return true;
        }

        static bool encode(const std::basic_string<std::byte>& input, std::string& output)
        {
            return encode(std::basic_string_view<std::byte>(input.data(), input.size()), output);
        }

        static std::string encode(const std::basic_string_view<std::byte>& input)
        {
            std::string output;
            encode(input, output);
            return output;
        }

        static std::string encode(const std::basic_string<std::byte>& input)
        {
            return encode(std::basic_string_view<std::byte>(input.data(), input.size()));
        }

        static bool decode(const std::string_view& input, std::basic_string<std::byte>& output)
        {
            static constexpr unsigned char decoding_table[] = {
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 52, 53, 54, 55, 56, 57,
                58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64,  0,  1,  2,  3,  4,  5,  6,
                 7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                25, 64, 64, 64, 64, 63, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
                37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64 };

            std::size_t quadruplets = input.size() / 4;
            std::size_t spare = 0;
            if (input.size() % 4 == 3) {
                spare = 2;
            } else if (input.size() % 4 == 2) {
                spare = 1;
            } else if (input.size() % 4 == 1) {
                return false;
            }

            output.resize(3 * quadruplets + spare);
            std::byte* p = output.data();

            std::size_t i = 0;
            for (std::size_t k = 0; k < quadruplets; i += 4, ++k) {
                unsigned int a = decoding_table[static_cast<int>(input[i])];
                unsigned int b = decoding_table[static_cast<int>(input[i + 1])];
                unsigned int c = decoding_table[static_cast<int>(input[i + 2])];
                unsigned int d = decoding_table[static_cast<int>(input[i + 3])];
                if (((a | b | c | d) & 64) != 0) {
                    return false;
                }

                unsigned int triplet = (a << 3 * 6) | (b << 2 * 6) | (c << 6) | d;
                *p++ = static_cast<std::byte>((triplet >> 2 * 8) & 0xff);
                *p++ = static_cast<std::byte>((triplet >> 1 * 8) & 0xff);
                *p++ = static_cast<std::byte>(triplet & 0xff);
            }

            switch (spare) {
            case 2:
            {
                unsigned int a = decoding_table[static_cast<int>(input[i])];
                unsigned int b = decoding_table[static_cast<int>(input[i + 1])];
                unsigned int c = decoding_table[static_cast<int>(input[i + 2])];
                if (((a | b | c) & 64) != 0) {
                    return false;
                }

                unsigned int triplet = (a << 2 * 6) | (b << 6) | c;
                *p++ = static_cast<std::byte>((triplet >> 10) & 0xff);
                *p++ = static_cast<std::byte>((triplet >> 2) & 0xff);
            }
            break;
            case 1:
            {
                unsigned int a = decoding_table[static_cast<int>(input[i])];
                unsigned int b = decoding_table[static_cast<int>(input[i + 1])];
                if (((a | b) & 64) != 0) {
                    return false;
                }

                unsigned int triplet = (a << 6) | b;
                *p++ = static_cast<std::byte>((triplet >> 4) & 0xff);
            }
            break;
            default:  // case 0:
            {}
            break;
            }

            return true;
        }

        static bool decode(const std::string& input, std::basic_string<std::byte>& output)
        {
            return decode(std::string_view(input.data(), input.size()), output);
        }
    };
}

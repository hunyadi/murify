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
            std::size_t out_len = 4 * ((in_len + 2) / 3);

            output.clear();
            output.reserve(out_len);

            std::size_t i = 0;
            if (in_len > 2) {
                for (; i < in_len - 2; i += 3) {
                    output.push_back(encoding_table[
                        (static_cast<unsigned>(input[i]) >> 2) & 0x3f
                    ]);
                    output.push_back(encoding_table[
                        ((static_cast<unsigned>(input[i]) & 0x3) << 4) |
                            ((static_cast<unsigned>(input[i + 1]) >> 4) & 0xf)
                    ]);
                    output.push_back(encoding_table[
                        ((static_cast<unsigned>(input[i + 1]) & 0xf) << 2) |
                            ((static_cast<unsigned>(input[i + 2]) >> 6) & 0x3)
                    ]);
                    output.push_back(encoding_table[
                        static_cast<unsigned>(input[i + 2]) & 0x3f
                    ]);
                }
            }
            if (i < in_len) {
                output.push_back(encoding_table[
                    (static_cast<unsigned>(input[i]) >> 2) & 0x3f
                ]);
                if (i == (in_len - 1)) {
                    output.push_back(encoding_table[
                        ((static_cast<unsigned>(input[i]) & 0x3) << 4)
                    ]);
                } else {
                    output.push_back(encoding_table[
                        ((static_cast<unsigned>(input[i]) & 0x3) << 4) |
                            ((static_cast<unsigned>(input[i + 1]) >> 4) & 0xf)
                    ]);
                    output.push_back(encoding_table[
                        ((static_cast<unsigned>(input[i + 1]) & 0xf) << 2)
                    ]);
                }
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
                 7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
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
            output.reserve(3 * quadruplets + spare);

            std::size_t i = 0;
            for (; i < 4 * quadruplets; i += 4) {
                unsigned int a = decoding_table[static_cast<int>(input[i])];
                unsigned int b = decoding_table[static_cast<int>(input[i + 1])];
                unsigned int c = decoding_table[static_cast<int>(input[i + 2])];
                unsigned int d = decoding_table[static_cast<int>(input[i + 3])];
                if (((a | b | c | d) & 64) != 0) {
                    return false;
                }

                unsigned int triplet = (a << 3 * 6) + (b << 2 * 6) + (c << 6) + d;

                output.push_back(static_cast<std::byte>((triplet >> 2 * 8) & 0xff));
                output.push_back(static_cast<std::byte>((triplet >> 1 * 8) & 0xff));
                output.push_back(static_cast<std::byte>(triplet & 0xff));
            }

            if (input.size() % 4 == 3) {
                unsigned int a = decoding_table[static_cast<int>(input[i])];
                unsigned int b = decoding_table[static_cast<int>(input[i + 1])];
                unsigned int c = decoding_table[static_cast<int>(input[i + 2])];
                if (((a | b | c) & 64) != 0) {
                    return false;
                }

                unsigned int triplet = (a << 2 * 6) + (b << 6) + c;

                output.push_back(static_cast<std::byte>((triplet >> 10) & 0xff));
                output.push_back(static_cast<std::byte>((triplet >> 2) & 0xff));
            } else if (input.size() % 4 == 2) {
                unsigned int a = decoding_table[static_cast<int>(input[i])];
                unsigned int b = decoding_table[static_cast<int>(input[i + 1])];
                if (((a | b) & 64) != 0) {
                    return false;
                }

                unsigned int triplet = (a << 6) + b;

                output.push_back(static_cast<std::byte>((triplet >> 4) & 0xff));
            }

            return true;
        }

        static bool decode(const std::string& input, std::basic_string<std::byte>& output)
        {
            return decode(std::string_view(input.data(), input.size()), output);
        }

        static std::string_view byte_to_string(const std::basic_string_view<std::byte>& in)
        {
            return std::string_view(reinterpret_cast<const char*>(in.data()), in.size());
        }

        static std::string_view byte_to_string(const std::basic_string<std::byte>& in)
        {
            return byte_to_string(std::basic_string_view<std::byte>(in.data(), in.size()));
        }

        static std::basic_string_view<std::byte> string_to_byte(const std::string_view& in)
        {
            return std::basic_string_view<std::byte>(reinterpret_cast<const std::byte*>(in.data()), in.size());
        }

        static std::basic_string_view<std::byte> string_to_byte(const std::string& in)
        {
            return string_to_byte(std::string_view(in.data(), in.size()));
        }
    };
}

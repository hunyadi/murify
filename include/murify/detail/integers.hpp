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
#include <cstdint>

namespace murify
{
    namespace detail
    {
        inline unsigned int get_integer_width(std::uint64_t value)
        {
            if (value <= 0xffull) {
                return 1;
            } else if (value <= 0xffffull) {
                return 2;
            } else if (value <= 0xffffffull) {
                return 3;
            } else if (value <= 0xffffffffull) {
                return 4;
            } else if (value <= 0xffffffffffull) {
                return 5;
            } else if (value <= 0xffffffffffffull) {
                return 6;
            } else if (value <= 0xffffffffffffffull) {
                return 7;
            } else {
                return 8;
            }
        }

        inline unsigned int get_integer_width(std::uint32_t value)
        {
            if (value <= 0xff) {
                return 1;
            } else if (value <= 0xffff) {
                return 2;
            } else if (value <= 0xffffff) {
                return 3;
            } else {
                return 4;
            }
        }

        inline unsigned long long read_integer(const std::basic_string_view<std::byte>& data)
        {
            using ull = unsigned long long;
            switch (data.size())
            {
            case 1:
                return static_cast<ull>(data[0]);
            case 2:
                return
                    static_cast<ull>(data[0]) << 8 |
                    static_cast<ull>(data[1]) << 0;
            case 3:
                return
                    static_cast<ull>(data[0]) << 16 |
                    static_cast<ull>(data[1]) << 8 |
                    static_cast<ull>(data[2]) << 0;
            case 4:
                return
                    static_cast<ull>(data[0]) << 24 |
                    static_cast<ull>(data[1]) << 16 |
                    static_cast<ull>(data[2]) << 8 |
                    static_cast<ull>(data[3]) << 0;
            case 5:
                return
                    static_cast<ull>(data[0]) << 32 |
                    static_cast<ull>(data[1]) << 24 |
                    static_cast<ull>(data[2]) << 16 |
                    static_cast<ull>(data[3]) << 8 |
                    static_cast<ull>(data[4]) << 0;
            case 6:
                return
                    static_cast<ull>(data[0]) << 40 |
                    static_cast<ull>(data[1]) << 32 |
                    static_cast<ull>(data[2]) << 24 |
                    static_cast<ull>(data[3]) << 16 |
                    static_cast<ull>(data[4]) << 8 |
                    static_cast<ull>(data[5]) << 0;
            case 7:
                return
                    static_cast<ull>(data[0]) << 48 |
                    static_cast<ull>(data[1]) << 40 |
                    static_cast<ull>(data[2]) << 32 |
                    static_cast<ull>(data[3]) << 24 |
                    static_cast<ull>(data[4]) << 16 |
                    static_cast<ull>(data[5]) << 8 |
                    static_cast<ull>(data[6]) << 0;
            case 8:
            default:
                return
                    static_cast<ull>(data[0]) << 56 |
                    static_cast<ull>(data[1]) << 48 |
                    static_cast<ull>(data[2]) << 40 |
                    static_cast<ull>(data[3]) << 32 |
                    static_cast<ull>(data[4]) << 24 |
                    static_cast<ull>(data[5]) << 16 |
                    static_cast<ull>(data[6]) << 8 |
                    static_cast<ull>(data[7]) << 0;
            }
        }

        inline void write_integer(std::basic_string<std::byte>& data, unsigned int width, std::uint64_t value)
        {
            switch (width)
            {
            case 8:
                data.push_back(static_cast<std::byte>((value >> 56) & 0xff));
            case 7:
                data.push_back(static_cast<std::byte>((value >> 48) & 0xff));
            case 6:
                data.push_back(static_cast<std::byte>((value >> 40) & 0xff));
            case 5:
                data.push_back(static_cast<std::byte>((value >> 32) & 0xff));
            case 4:
                data.push_back(static_cast<std::byte>((value >> 24) & 0xff));
            case 3:
                data.push_back(static_cast<std::byte>((value >> 16) & 0xff));
            case 2:
                data.push_back(static_cast<std::byte>((value >> 8) & 0xff));
            case 1:
                data.push_back(static_cast<std::byte>(value & 0xff));
            default:
                break;
            }
        }

        inline void write_integer(std::basic_string<std::byte>& data, unsigned int width, std::uint32_t value)
        {
            switch (width)
            {
            case 4:
                data.push_back(static_cast<std::byte>((value >> 24) & 0xff));
            case 3:
                data.push_back(static_cast<std::byte>((value >> 16) & 0xff));
            case 2:
                data.push_back(static_cast<std::byte>((value >> 8) & 0xff));
            case 1:
                data.push_back(static_cast<std::byte>(value & 0xff));
            default:
                break;
            }
        }
    }
}

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

namespace murify
{
    namespace detail
    {
        enum class Embedding : unsigned int
        {
            none = 0b01,
            integer = 0b00,
            interned_string = 0b10,
            string_length = 0b11
        };

        enum class Coding : unsigned int
        {
            width = 0b00,
            indexed = 0b10,
            base64 = 0b11,
            encapsulated = 0b01
        };

        enum class DataType : unsigned int
        {
            integer = 0,
            string = 1
        };

        enum class Encapsulation : unsigned int
        {
            uuid = 0,
            jwt = 1
        };

        struct embedded_value_t
        {
            Embedding embedding : 2;
            unsigned value : 6;
        };

        struct prefixed_value_t
        {
            Embedding embedding : 2;
            Coding coding : 2;
            DataType data_type : 1;
            unsigned width : 3;
        };

        struct encapsulated_value_t
        {
            Embedding embedding : 2;
            Coding coding : 2;
            Encapsulation identifier : 4;
        };

        union control_byte {
            std::byte value;
            embedded_value_t embedded_value;
            prefixed_value_t prefixed_value;
            encapsulated_value_t encapsulated_value;
        };

        inline unsigned int get_integer_width(uint64_t value)
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

        inline unsigned int get_integer_width(uint32_t value)
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

        inline void write_integer(std::basic_string<std::byte>& data, unsigned int width, uint64_t value)
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

        inline void write_integer(std::basic_string<std::byte>& data, unsigned int width, uint32_t value)
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

        inline void copy(std::basic_string<std::byte>& target, const std::string_view& source)
        {
            std::size_t s = target.size();
            target.resize(s + source.size());
            std::memcpy(target.data() + s, source.data(), source.size());
        }

        inline std::string take(const std::basic_string_view<std::byte>& source)
        {
            std::string target;
            target.resize(source.size());
            std::memcpy(target.data(), source.data(), source.size());
            return target;
        }

        /**
         * Splits a string into parts along occurrences of the separator character.
         *
         * @param in The string to split.
         * @param sep The character that is separating the parts.
         */
        inline std::vector<std::string_view> split(const std::string_view& in, char sep)
        {
            std::vector<std::string_view> r;
            for (std::size_t p = 0; ; ++p) {
                std::size_t q = p;
                p = in.find(sep, q);
                r.push_back(in.substr(q, p - q));
                if (p == std::string_view::npos) {
                    return r;
                }
            }
        }

        /**
         * Splits a string into parts, including the separator characters.
         *
         * @param in The string to split.
         * @param chars A string of separator characters, which are included in the result.
         */
        inline std::vector<std::string_view> tokenize(const std::string_view& in, const char* chars)
        {
            std::vector<std::string_view> r;
            for (std::size_t p = 0; ; ++p) {
                std::size_t q = p;
                p = in.find_first_of(chars, q);
                auto piece = in.substr(q, p - q);
                if (!piece.empty()) {
                    r.push_back(piece);
                }
                if (p == std::string_view::npos) {
                    return r;
                }
                r.push_back(in.substr(p, 1));
            }
        }

        /**
         * Joins parts into a string.
         *
         * @param parts The parts to join.
         */
        inline std::string join(const std::vector<std::string>& parts)
        {
            std::string s;
            for (auto it = parts.begin(); it != parts.end(); ++it) {
                s += *it;
            }
            return s;
        }

        /**
         * Joins parts into a string.
         *
         * @param parts The parts to join.
         * @param sep The character that is separating the parts.
         */
        inline std::string join(const std::vector<std::string>& parts, char sep)
        {
            std::string s;
            for (auto it = parts.begin(); it != parts.end(); ++it) {
                s += *it;
                if (it != parts.end() - 1) {
                    s += sep;
                }
            }
            return s;
        }
    }
}

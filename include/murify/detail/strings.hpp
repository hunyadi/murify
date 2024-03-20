#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace murify
{
    namespace detail
    {
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

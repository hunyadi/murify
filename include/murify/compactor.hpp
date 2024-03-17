#pragma once
#include "detail/detail.hpp"
#include "base64url.hpp"
#include "interned_string.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <charconv>
#include <algorithm>
#include <stdexcept>

namespace murify
{
    struct BaseTokenizer
    {
        static std::vector<std::string_view> split(const std::string_view& str);
        static std::string join(const std::vector<std::string>& parts);
    };

    /** URL compressor.
     *
     * ```
     * 0 0  n n  n  n n n  --> embedded integer with value n
     * 0 1  0 0  0  w w w  --> integer expressed in width w
     * 0 1  0 0  1  w w w  --> string of size expressed in width w, followed by characters
     * 0 1  1 0  0  - - -  --> [unused]
     * 0 1  1 0  1  w w w  --> interned string index expressed in width w
     * 0 1  0 1  c  c c c  --> encapsulated data (e.g. UUID)
     * 0 1  1 1  s  s s s  --> base64-encoded string of size s
     * 1 0  i i  i  i i i  --> embedded interned string with index i
     * 1 1  s s  s  s s s  --> string of embedded size s, followed by characters
     * ```
    */
    template<typename Tokenizer>
    struct Compactor
    {
        std::basic_string<std::byte> compact(const std::string_view& str);

        std::basic_string<std::byte> compact(const std::string& str)
        {
            return compact(std::string_view(str.data(), str.size()));
        }

        std::string expand(const std::basic_string_view<std::byte>& enc);

        std::string expand(const std::basic_string<std::byte>& enc)
        {
            return expand(std::basic_string_view<std::byte>(enc.data(), enc.size()));
        }

    protected:
        void compact_interned(std::basic_string<std::byte>& out, const std::string_view& part);
        void compact_string(std::basic_string<std::byte>& out, const std::string_view& part);
        bool compact_jwt(std::basic_string<std::byte>& out, const std::string_view& part);
        std::size_t expand_single(std::string& out, const std::basic_string_view<std::byte>& enc);
        std::size_t expand_jwt(std::string& out, const std::basic_string_view<std::byte>& enc);

    private:
        interned_store string_store;
    };

    struct PathTokenizer : BaseTokenizer
    {
        static std::vector<std::string_view> split(const std::string_view& str);
        static std::string join(const std::vector<std::string>& parts);
    };

    struct PathCompactor : Compactor<PathTokenizer>
    {};

    struct QueryTokenizer : BaseTokenizer
    {
        static std::vector<std::string_view> split(const std::string_view& str);
        static std::string join(const std::vector<std::string>& parts);
    };

    struct QueryCompactor : Compactor<QueryTokenizer>
    {};

    template<typename Tokenizer>
    std::basic_string<std::byte> Compactor<Tokenizer>::compact(const std::string_view& str)
    {
        using detail::Embedding, detail::Coding, detail::DataType, detail::Encapsulation;
        using detail::control_byte;
        using detail::copy, detail::get_integer_width, detail::write_integer;

        if (str.empty()) {
            return std::basic_string<std::byte>();
        }

        auto parts = Tokenizer::split(str);

        std::basic_string<std::byte> out;
        out.push_back(static_cast<std::byte>(parts.size()));

        for (auto it = parts.begin(); it != parts.end(); ++it) {
            auto part = *it;

            // empty string is always interned
            if (part.empty()) {
                compact_interned(out, part);
                continue;
            }

            // string of decimal digits
            unsigned long long number;
            std::from_chars_result result = std::from_chars(part.data(), part.data() + part.size(), number);
            if (result.ec == std::errc{} && result.ptr == part.data() + part.size()) {
                if (number < 64) {
                    // embedded integer
                    control_byte control;
                    control.embedded_value.embedding = Embedding::integer;
                    control.embedded_value.value = number;
                    out.push_back(control.value);
                } else {
                    // integer with explicitly specified width and value
                    unsigned width = get_integer_width(number);

                    control_byte control;
                    control.prefixed_value.embedding = Embedding::none;
                    control.prefixed_value.coding = Coding::width;
                    control.prefixed_value.data_type = DataType::integer;
                    control.prefixed_value.width = width - 1;
                    out.push_back(control.value);

                    write_integer(out, width, number);
                }
                continue;
            }

            // intern-able string
            if (part.size() < 24 && std::all_of(part.begin(), part.end(), [](char c) { return std::islower(c) || c == '_'; })) {
                compact_interned(out, part);
                continue;
            }

            // JWT
            if (part.size() >= 2 && part[0] == 'e' && part[1] == 'y') {
                if (compact_jwt(out, part)) {
                    continue;
                }
            }

            // non-intern-able string
            compact_string(out, part);
        }

        return out;
    }

    template<typename Tokenizer>
    void Compactor<Tokenizer>::compact_string(std::basic_string<std::byte>& out, const std::string_view& part)
    {
        using detail::Embedding, detail::Coding, detail::DataType, detail::Encapsulation;
        using detail::control_byte;
        using detail::copy, detail::get_integer_width, detail::write_integer;

        unsigned int length = static_cast<unsigned int>(part.size());
        if (length < 64) {
            // short string with embedded length
            control_byte control;
            control.embedded_value.embedding = Embedding::string_length;
            control.embedded_value.value = length;
            out.push_back(control.value);
        } else {
            // long string with explicitly specified length
            auto width = get_integer_width(length);

            control_byte control;
            control.prefixed_value.embedding = Embedding::none;
            control.prefixed_value.coding = Coding::width;
            control.prefixed_value.data_type = DataType::string;
            control.prefixed_value.width = width - 1;
            out.push_back(control.value);

            write_integer(out, width, length);
        }

        // characters of string
        copy(out, part);
    }

    template<typename Tokenizer>
    void Compactor<Tokenizer>::compact_interned(std::basic_string<std::byte>& out, const std::string_view& part)
    {
        using detail::Embedding, detail::Coding, detail::DataType;
        using detail::control_byte;
        using detail::copy, detail::get_integer_width, detail::write_integer;

        interned_string s = string_store.intern(part);
        uint32_t index = s.index();
        if (index < 64) {
            // interned string with embedded index
            control_byte control;
            control.embedded_value.embedding = Embedding::interned_string;
            control.embedded_value.value = s.index();
            out.push_back(control.value);
        } else {
            // interned string with explicitly specified width and index
            unsigned width = get_integer_width(index);

            control_byte control;
            control.prefixed_value.embedding = Embedding::none;
            control.prefixed_value.coding = Coding::indexed;
            control.prefixed_value.data_type = DataType::string;
            control.prefixed_value.width = width - 1;
            out.push_back(control.value);

            write_integer(out, width, index);
        }
    }

    template<typename Tokenizer>
    bool Compactor<Tokenizer>::compact_jwt(std::basic_string<std::byte>& out, const std::string_view& part)
    {
        using detail::Embedding, detail::Coding, detail::Encapsulation;
        using detail::control_byte;
        using detail::split;

        auto jwt_parts = split(part, '.');
        if (jwt_parts.size() != 3) {
            return false;
        }

        std::basic_string<std::byte> header;
        if (!base64::decode(jwt_parts[0], header)) {
            return false;
        }
        std::basic_string<std::byte> payload;
        if (!base64::decode(jwt_parts[1], payload)) {
            return false;
        }
        std::basic_string<std::byte> signature;
        if (!base64::decode(jwt_parts[2], signature)) {
            return false;
        }

        control_byte control;
        control.encapsulated_value.embedding = Embedding::none;
        control.encapsulated_value.coding = Coding::encapsulated;
        control.encapsulated_value.identifier = Encapsulation::jwt;
        out.push_back(control.value);

        compact_interned(out, base64::byte_to_string(header));
        compact_string(out, base64::byte_to_string(payload));
        compact_string(out, base64::byte_to_string(signature));
        return true;
    }

    template<typename Tokenizer>
    std::string Compactor<Tokenizer>::expand(const std::basic_string_view<std::byte>& enc)
    {
        if (enc.empty()) {
            return std::string();
        }

        std::vector<std::string> parts;

        std::size_t index = 0;
        auto count = static_cast<std::size_t>(enc[index]);
        parts.reserve(count);
        ++index;

        for (std::size_t i = 0; i < count; ++i) {
            std::string part;
            index += expand_single(part, enc.substr(index));
            parts.push_back(part);
        }
        return Tokenizer::join(parts);
    }

    template<typename Tokenizer>
    std::size_t Compactor<Tokenizer>::expand_single(std::string& out, const std::basic_string_view<std::byte>& enc)
    {
        using detail::Embedding, detail::Coding, detail::DataType, detail::Encapsulation;
        using detail::control_byte;
        using detail::read_integer, detail::take;

        std::size_t index = 0;
        control_byte control;
        control.value = enc[index];
        ++index;

        std::size_t length;
        unsigned int width;
        unsigned long long value;

        switch (control.embedded_value.embedding) {
        case Embedding::integer:
            // embedded integer
            out = std::to_string(control.embedded_value.value);
            break;
        case Embedding::interned_string:
            // interned string with embedded index
            out = interned_string(control.embedded_value.value).data(string_store);
            break;
        case Embedding::string_length:
            // string with embedded length
            length = control.embedded_value.value;
            out = take(enc.substr(index, length));
            index += length;
            break;
        case Embedding::none:
            switch (control.prefixed_value.coding) {
            case Coding::width:
                width = control.prefixed_value.width + 1;
                switch (control.prefixed_value.data_type) {
                case DataType::integer:
                    // integer with externally specified value
                    value = read_integer(enc.substr(index, width));
                    index += width;
                    out = std::to_string(value);
                    break;
                case DataType::string:
                    // string with externally specified length
                    length = read_integer(enc.substr(index, width));
                    index += width;
                    out = take(enc.substr(index, length));
                    index += length;
                    break;
                }
                break;
            case Coding::indexed:
                width = control.prefixed_value.width + 1;
                switch (control.prefixed_value.data_type) {
                case DataType::integer:
                    throw std::runtime_error("indexed coding of integer type is unused");
                case DataType::string:
                    // interned string with externally specified index
                    uint32_t string_index = static_cast<uint32_t>(read_integer(enc.substr(index, width)));
                    index += width;
                    out = interned_string(string_index).data(string_store);
                    break;
                }
                break;
            case Coding::base64:
                throw std::runtime_error("base64 encoding not implemented");
            case Coding::encapsulated:
                switch (control.encapsulated_value.identifier)
                {
                case Encapsulation::jwt:
                    index += expand_jwt(out, enc.substr(index));
                    break;
                default:
                    throw std::runtime_error("encapsulated encoding not implemented");
                }
            }
            break;
        }

        return index;
    }

    template<typename Tokenizer>
    std::size_t Compactor<Tokenizer>::expand_jwt(std::string& out, const std::basic_string_view<std::byte>& enc)
    {
        std::string header;
        std::size_t index = expand_single(header, enc);
        std::string payload;
        index += expand_single(payload, enc.substr(index));
        std::string signature;
        index += expand_single(signature, enc.substr(index));

        out.append(base64::encode(base64::string_to_byte(header)));
        out += '.';
        out.append(base64::encode(base64::string_to_byte(payload)));
        out += '.';
        out.append(base64::encode(base64::string_to_byte(signature)));

        return index;
    }

    std::vector<std::string_view> PathTokenizer::split(const std::string_view& str)
    {
        return detail::split(str, '/');
    }

    std::string PathTokenizer::join(const std::vector<std::string>& parts)
    {
        return detail::join(parts, '/');
    }

    std::vector<std::string_view> QueryTokenizer::split(const std::string_view& str)
    {
        auto key_value_pairs = detail::split(str, '&');
        std::vector<std::string_view> parts;
        for (auto&& key_value : key_value_pairs) {
            std::size_t index = key_value.find('=');
            if (index != std::string::npos) {
                parts.emplace_back(key_value.data(), index);
                parts.emplace_back("=", 1);
                parts.emplace_back(key_value.data() + index + 1, key_value.size() - index - 1);
            } else {
                parts.emplace_back(key_value.data(), key_value.size());
                parts.emplace_back("", 0);
                parts.emplace_back("", 0);
            }
        }
        return parts;
    }

    std::string QueryTokenizer::join(const std::vector<std::string>& parts)
    {
        std::vector<std::string> pieces;
        for (auto it = parts.begin(); it != parts.end(); ) {
            auto&& key = *it;
            ++it;
            auto&& sep = *it;
            ++it;
            auto&& value = *it;
            ++it;
            std::string str;
            str.reserve(key.size() + sep.size() + value.size());
            str.append(key);
            str.append(sep);
            str.append(value);
            pieces.push_back(str);
        }
        return detail::join(pieces, '&');
    }
}

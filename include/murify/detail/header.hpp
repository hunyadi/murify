#pragma once
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
    }
}

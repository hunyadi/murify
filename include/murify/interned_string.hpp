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
#include <cstring>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

namespace murify
{
    struct interned_store;

    /**
     * Encapsulates a string stored in an indexed array of strings, and referenced with its ordinal.
     */
    struct interned_string
    {
        interned_string() = default;

        /** Constructs an interned string from an ordinal assigned in an indexed array of strings. */
        explicit interned_string(uint32_t index)
            : _index(index)
        {}

        /** The ordinal assigned to this interned string. */
        uint32_t index() const
        {
            return _index;
        }

        /** The characters stored in the indexed array. */
        const char* data(const interned_store& store) const;

        /** String length. */
        std::size_t size(const interned_store& store) const;

        /** A string view over the characters stored in the indexed array. */
        std::string_view str(const interned_store& store) const;

    private:
        uint32_t _index = 0;
    };

    /**
     * Maps strings into ordinals of an indexed array of strings.
     */
    struct interned_store
    {
        interned_store() = default;

        interned_store(const interned_store&) = delete;

        ~interned_store()
        {
            clear();
        }

        /** The characters of a string stored in the indexed array. */
        const char* data(const interned_string& s) const
        {
            return _data[s.index()] + sizeof(std::size_t);
        }

        /** String length. */
        std::size_t size(const interned_string& s) const
        {
            return *reinterpret_cast<const std::size_t*>(_data[s.index()]);
        }

        /** A string view over the characters stored in the indexed array. */
        std::string_view str(const interned_string& s) const
        {
            return std::string_view(data(s), size(s));
        }

        struct const_iterator
        {
            const_iterator(const interned_store& ref, uint32_t index)
                : _ref(ref), _index(index)
            {}

            bool operator==(const const_iterator& op) const
            {
                return &_ref == &op._ref && _index == op._index;
            }

            bool operator!=(const const_iterator& op) const
            {
                return &_ref != &op._ref || _index != op._index;
            }

            std::string_view operator*() const
            {
                return interned_string(_index).str(_ref);
            }

            const_iterator& operator++()
            {
                ++_index;
                return *this;
            }

            const_iterator operator++(int)
            {
                const_iterator result = *this;
                ++_index;
                return result;
            }

        private:
            const interned_store& _ref;
            uint32_t _index;
        };

        /** Number of strings stored in the indexed array. */
        std::size_t count() const
        {
            return _table.size();
        }

        const_iterator begin() const
        {
            return const_iterator(*this, 0);
        }

        const_iterator end() const
        {
            return const_iterator(*this, static_cast<uint32_t>(count()));
        }

        /** Deallocates and removes all strings in the indexed array. */
        void clear()
        {
            _table.clear();
            for (auto it = _data.begin(); it != _data.end(); ++it)
            {
                delete (*it);
            }
            _data.clear();
        }

        interned_string intern(const char* beg, const char* end)
        {
            return intern(std::string_view(beg, end - beg));
        }

        interned_string intern(const char* beg, std::size_t siz)
        {
            return intern(std::string_view(beg, siz));
        }

        interned_string intern(const std::string& str)
        {
            return intern(std::string_view(str.data(), str.size()));
        }

        /** Adds a new string to the indexed array and assigns an ordinal to the interned string. */
        interned_string intern(const std::string_view& str)
        {
            uint32_t index;
            auto it = _table.find(str);
            if (it != _table.end())
            {
                index = it->second;
            } else
            {
                char* ptr = new char[sizeof(std::size_t) + str.size() + 1];
                *reinterpret_cast<std::size_t*>(ptr) = str.size();
                char* s = ptr + sizeof(std::size_t);
                std::memcpy(s, str.data(), str.size());
                s[str.size()] = 0;
                index = static_cast<uint32_t>(_data.size());
                _data.push_back(ptr);
                _table.insert(std::make_pair(std::string_view(s, str.size()), index));
            }
            return interned_string(index);
        }

    private:
        std::unordered_map<std::string_view, uint32_t> _table;
        std::vector<const char*> _data;
    };

    inline const char* interned_string::data(const interned_store& store) const
    {
        return store.data(*this);
    }

    inline std::size_t interned_string::size(const interned_store& store) const
    {
        return store.size(*this);
    }

    inline std::string_view interned_string::str(const interned_store& store) const
    {
        return store.str(*this);
    }
}

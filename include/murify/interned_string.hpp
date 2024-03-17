#pragma once
#include <cstring>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

namespace murify
{
    struct interned_store;

    struct interned_string
    {
        interned_string() = default;

        explicit interned_string(uint32_t index)
            : _index(index)
        {}

        uint32_t index() const
        {
            return _index;
        }

        const char* data(const interned_store& store) const;

        std::size_t size(const interned_store& store) const;

        std::string_view str(const interned_store& store) const;

    private:
        uint32_t _index = 0;
    };

    struct interned_store
    {
        interned_store() = default;

        interned_store(const interned_store&) = delete;

        ~interned_store()
        {
            clear();
        }

        const char* data(const interned_string& s) const
        {
            return _data[s.index()] + sizeof(std::size_t);
        }

        std::size_t size(const interned_string& s) const
        {
            return *reinterpret_cast<const std::size_t*>(_data[s.index()]);
        }

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

        void clear()
        {
            _table.clear();
            for (auto it = _data.begin(); it != _data.end(); ++it)
            {
                delete (*it);
            }
            _data.clear();
        }

        interned_string intern(const std::string& str)
        {
            return intern(std::string_view(str.data(), str.size()));
        }

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

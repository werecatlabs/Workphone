#pragma once
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <string>
#include <string_view>
#include <ostream>
#include <initializer_list>
#include <utility>

namespace workphone
{
    /** Public std-string-like ownership with allocation-local SourceCpu charges.
     * Copies and owning imports retain spare capacity, including the terminator.
     * Inline storage lives in the enclosing object. clear() retains capacity;
     * releaseStorage() physically retires it. Foreign output is explicit. */
    class SourceBudgetStdString : public SourceBudgetObject
    {
        using Storage = std::basic_string<char, std::char_traits<char>, SourceBudgetAllocator<char>>;
        Storage m_value;

        template<class A> void import(const std::basic_string<char, std::char_traits<char>, A> &other)
        {
            m_value.reserve(other.capacity());
            m_value.assign(other.data(), other.size());
        }
    public:
        using value_type = char;
        using size_type = size_t;
        using iterator = Storage::iterator;
        using const_iterator = Storage::const_iterator;
        using reverse_iterator = Storage::reverse_iterator;
        using const_reverse_iterator = Storage::const_reverse_iterator;
        using allocator_type = SourceBudgetAllocator<char>;
        static constexpr auto npos = Storage::npos;

        SourceBudgetStdString() = default;
        SourceBudgetStdString(const char *text) : m_value(text) {}
        SourceBudgetStdString(const char *text, size_t count) : m_value(text, count) {}
        SourceBudgetStdString(size_t count, char value) : m_value(count, value) {}
        SourceBudgetStdString(std::initializer_list<char> values) : m_value(values) {}
        SourceBudgetStdString(std::string_view text) : m_value(text.empty() ? "" : text.data(), text.size()) {}
        template<class A> SourceBudgetStdString(const std::basic_string<char, std::char_traits<char>, A> &other)
        { import(other); }
        template<class It> SourceBudgetStdString(It first, It last) : m_value(first, last) {}
        SourceBudgetStdString(const SourceBudgetStdString &other) { import(other.m_value); }
        SourceBudgetStdString(SourceBudgetStdString &&) noexcept = default;
        SourceBudgetStdString &operator=(SourceBudgetStdString &&) noexcept = default;
        SourceBudgetStdString &operator=(const SourceBudgetStdString &other)
        {
            if(this != &other) { SourceBudgetStdString candidate(other); swap(candidate); }
            return *this;
        }
        template<class A> SourceBudgetStdString &operator=(const std::basic_string<char, std::char_traits<char>, A> &other)
        { SourceBudgetStdString candidate(other); swap(candidate); return *this; }
        SourceBudgetStdString &operator=(std::string_view text)
        { SourceBudgetStdString candidate(text); swap(candidate); return *this; }
        SourceBudgetStdString &operator=(const char *text)
        { SourceBudgetStdString candidate(text); swap(candidate); return *this; }
        SourceBudgetStdString &operator=(std::initializer_list<char> values)
        { SourceBudgetStdString candidate(values); swap(candidate); return *this; }
        SourceBudgetStdString &operator=(char value)
        { SourceBudgetStdString candidate(1,value); swap(candidate); return *this; }

        std::string_view view() const noexcept { return {m_value.data(), m_value.size()}; }
        operator std::string_view() const noexcept { return view(); }
        explicit operator std::string() const { return {m_value.data(), m_value.size()}; }
        const char *c_str() const noexcept { return m_value.c_str(); }
        char *data() noexcept { return m_value.data(); }
        const char *data() const noexcept { return m_value.data(); }
        size_t size() const noexcept { return m_value.size(); }
        size_t length() const noexcept { return m_value.length(); }
        size_t capacity() const noexcept { return m_value.capacity(); }
        size_t max_size() const noexcept { return m_value.max_size(); }
        bool empty() const noexcept { return m_value.empty(); }
        allocator_type get_allocator() const noexcept { return m_value.get_allocator(); }
        char &operator[](size_t index) { return m_value[index]; }
        const char &operator[](size_t index) const { return m_value[index]; }
        char &at(size_t index) { return m_value.at(index); }
        const char &at(size_t index) const { return m_value.at(index); }
        char &front() { return m_value.front(); }
        const char &front() const { return m_value.front(); }
        char &back() { return m_value.back(); }
        const char &back() const { return m_value.back(); }
        iterator begin() noexcept { return m_value.begin(); }
        iterator end() noexcept { return m_value.end(); }
        const_iterator begin() const noexcept { return m_value.begin(); }
        const_iterator end() const noexcept { return m_value.end(); }
        const_iterator cbegin() const noexcept { return m_value.cbegin(); }
        const_iterator cend() const noexcept { return m_value.cend(); }
        reverse_iterator rbegin() noexcept { return m_value.rbegin(); }
        reverse_iterator rend() noexcept { return m_value.rend(); }
        const_reverse_iterator rbegin() const noexcept { return m_value.rbegin(); }
        const_reverse_iterator rend() const noexcept { return m_value.rend(); }
        void reserve(size_t count) { m_value.reserve(count); }
        void resize(size_t count, char value = char()) { m_value.resize(count, value); }
        void shrink_to_fit() { m_value.shrink_to_fit(); }
        void clear() noexcept { m_value.clear(); }
        void releaseStorage() noexcept { Storage{}.swap(m_value); }
        void swap(SourceBudgetStdString &other) noexcept { m_value.swap(other.m_value); }
        void push_back(char value) { m_value.push_back(value); }
        void pop_back() { m_value.pop_back(); }
        SourceBudgetStdString &assign(const char *text, size_t count)
        { m_value.assign(text, count); return *this; }
        SourceBudgetStdString &assign(size_t count, char value)
        { m_value.assign(count, value); return *this; }
        SourceBudgetStdString &assign(std::string_view text)
        { return assign(text.empty() ? "" : text.data(), text.size()); }
        SourceBudgetStdString &append(std::string_view text)
        { if(!text.empty()) m_value.append(text.data(), text.size()); return *this; }
        SourceBudgetStdString &append(const char *text, size_t count)
        { m_value.append(text, count); return *this; }
        SourceBudgetStdString &append(size_t count, char value)
        { m_value.append(count, value); return *this; }
        SourceBudgetStdString &operator+=(std::string_view text) { return append(text); }
        SourceBudgetStdString &operator+=(char value) { push_back(value); return *this; }
        SourceBudgetStdString &erase(size_t index = 0, size_t count = npos)
        { m_value.erase(index, count); return *this; }
        size_t find(std::string_view text, size_t offset = 0) const noexcept { return view().find(text, offset); }
        size_t find(char value, size_t offset = 0) const noexcept { return view().find(value, offset); }
        size_t rfind(std::string_view text, size_t offset = npos) const noexcept { return view().rfind(text, offset); }
        SourceBudgetStdString substr(size_t offset = 0, size_t count = npos) const { return view().substr(offset, count); }
        int compare(std::string_view other) const noexcept { return view().compare(other); }

        friend bool operator==(const SourceBudgetStdString &a, const SourceBudgetStdString &b) noexcept { return a.view() == b.view(); }
        friend bool operator==(const SourceBudgetStdString &a, std::string_view b) noexcept { return a.view() == b; }
        friend bool operator==(std::string_view a, const SourceBudgetStdString &b) noexcept { return a == b.view(); }
        friend bool operator==(const SourceBudgetStdString &a, const char *b) noexcept { return a.view() == b; }
        friend bool operator==(const char *a, const SourceBudgetStdString &b) noexcept { return a == b.view(); }
        template<class A> friend bool operator==(const SourceBudgetStdString &a, const std::basic_string<char, std::char_traits<char>, A> &b) noexcept
        { return a.view() == std::string_view(b.data(), b.size()); }
        template<class A> friend bool operator==(const std::basic_string<char, std::char_traits<char>, A> &a, const SourceBudgetStdString &b) noexcept { return b == a; }
        friend bool operator!=(const SourceBudgetStdString &a, const SourceBudgetStdString &b) noexcept { return !(a == b); }
        friend bool operator!=(const SourceBudgetStdString &a, std::string_view b) noexcept { return !(a == b); }
        friend bool operator!=(std::string_view a, const SourceBudgetStdString &b) noexcept { return !(a == b); }
        friend bool operator!=(const SourceBudgetStdString &a, const char *b) noexcept { return !(a == b); }
        friend bool operator!=(const char *a, const SourceBudgetStdString &b) noexcept { return !(a == b); }
        template<class A> friend bool operator!=(const SourceBudgetStdString &a, const std::basic_string<char, std::char_traits<char>, A> &b) noexcept { return !(a == b); }
        template<class A> friend bool operator!=(const std::basic_string<char, std::char_traits<char>, A> &a, const SourceBudgetStdString &b) noexcept { return !(a == b); }
        friend bool operator<(const SourceBudgetStdString &a, const SourceBudgetStdString &b) noexcept { return a.view() < b.view(); }
        friend SourceBudgetStdString operator+(SourceBudgetStdString a, const SourceBudgetStdString &b) { a += b.view(); return a; }
        friend SourceBudgetStdString operator+(SourceBudgetStdString a, std::string_view b) { a += b; return a; }
        friend SourceBudgetStdString operator+(std::string_view a, const SourceBudgetStdString &b) { SourceBudgetStdString result(a); result += b.view(); return result; }
        friend SourceBudgetStdString operator+(SourceBudgetStdString a, const char *b) { a += std::string_view(b); return a; }
        friend SourceBudgetStdString operator+(const char *a, const SourceBudgetStdString &b) { return std::string_view(a) + b; }
        template<class A> friend SourceBudgetStdString operator+(SourceBudgetStdString a, const std::basic_string<char, std::char_traits<char>, A> &b)
        { a += std::string_view(b.data(), b.size()); return a; }
        template<class A> friend SourceBudgetStdString operator+(const std::basic_string<char, std::char_traits<char>, A> &a, const SourceBudgetStdString &b)
        { return std::string_view(a.data(), a.size()) + b; }
        friend std::ostream &operator<<(std::ostream &stream, const SourceBudgetStdString &value)
        { return stream.write(value.data(), static_cast<std::streamsize>(value.size())); }
    };

    inline void swap(SourceBudgetStdString &a, SourceBudgetStdString &b) noexcept { a.swap(b); }
}

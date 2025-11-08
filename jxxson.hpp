
//
// MIT License
// 
// Copyright (c) 2025 Henry Du
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once

#include <format>
#include <ranges>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <forward_list>

namespace jxxson {
    enum class document_tree_node_type : std::uint8_t {
        null,
        boolean,
        integer,
        floating_point,
        string,
        array,
        object,
        root
    };

    static constexpr struct document_node_root_tag_type   { std::size_t padding; } document_node_root_tag{};
    static constexpr struct document_node_array_tag_type  { std::size_t padding; } document_node_array_tag{};
    static constexpr struct document_node_object_tag_type { std::size_t padding; } document_node_object_tag{};

    template <typename Integer = int, typename FloatingPoint = float, class CharT = char, class BufferAllocator = std::allocator<CharT>>
    class document_node_value {
    public:
        using string              = std::basic_string<CharT, std::char_traits<CharT>, BufferAllocator>;
        using string_view         = std::basic_string_view<CharT, std::char_traits<CharT>>;
        using int_type            = Integer;
        using float_type          = FloatingPoint;

        document_tree_node_type type = document_tree_node_type::null;
        string                  buffer;

        constexpr document_node_value(const document_node_value&) = default;
        constexpr document_node_value(document_node_value&&) = default;
        constexpr document_node_value& operator=(const document_node_value&) = default;
        constexpr document_node_value& operator=(document_node_value&&) = default;
        constexpr ~document_node_value() = default;

        constexpr document_node_value(const BufferAllocator& a = BufferAllocator{}) : type(document_tree_node_type::null), buffer(a) {}
        constexpr document_node_value(const bool           b, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::boolean),          buffer(reinterpret_cast<const char*>(&b), sizeof(b), a) {}
        constexpr document_node_value(const int_type       i, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::integer),          buffer(reinterpret_cast<const char*>(&i), sizeof(i), a) {}
        constexpr document_node_value(const float_type     f, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::floating_point),   buffer(reinterpret_cast<const char*>(&f), sizeof(f), a) {}
        constexpr document_node_value(const string_view    s, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::string),           buffer(s, a) {}
        constexpr document_node_value(const string&        s, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::string),           buffer(s, a) {}
        constexpr document_node_value(const CharT*         s, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::string),           buffer(s, a) {}

        constexpr document_node_value(const decltype(document_node_root_tag)   r, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::root),   buffer(reinterpret_cast<const char*>(&r), sizeof(r), a) {}
        constexpr document_node_value(const decltype(document_node_array_tag)  r, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::array),  buffer(reinterpret_cast<const char*>(&r), sizeof(r), a) {}
        constexpr document_node_value(const decltype(document_node_object_tag) r, const BufferAllocator a = BufferAllocator{}) : type(document_tree_node_type::object), buffer(reinterpret_cast<const char*>(&r), sizeof(r), a) {}

        template <typename Ty> constexpr Ty& as() {
            if constexpr (std::is_same_v<Ty, string>) { return buffer; }
            return *reinterpret_cast<Ty*>(buffer.data());
        }

        template <typename Ty> constexpr auto as() const {
            if constexpr (std::is_same_v<Ty, string>) { return string_view(buffer); }
            return *reinterpret_cast<const Ty*>(buffer.data());
        }

        constexpr bool parent_type()   const { return type == document_tree_node_type::object || type == document_tree_node_type::array || type == document_tree_node_type::root; }
        constexpr auto get_allocator() const { return buffer.get_allocator(); }

        // This is a value formatter.
        template <bool IsBegin, class OutputIt>
        constexpr OutputIt format_to(OutputIt out) {
            constexpr CharT  out_null [4]  = {CharT{'n'}, CharT{'u'}, CharT{'l'}, CharT{'l'}};
            constexpr CharT  out_true [4]  = {CharT{'t'}, CharT{'r'}, CharT{'u'}, CharT{'e'}};
            constexpr CharT  out_false[5]  = {CharT{'f'}, CharT{'a'}, CharT{'l'}, CharT{'s'}, CharT{'e'}};
            if constexpr (IsBegin) {
                switch (type) {
                case document_tree_node_type::null:    {          out = std::ranges::copy(out_null, out).out; break; } 
                case document_tree_node_type::boolean: {
                    if (as<bool>()) { out = std::ranges::copy(out_true, out).out; }
                    else { out = std::ranges::copy(out_false, out).out; }
                    break;
                }
                case document_tree_node_type::integer: {
                    if constexpr (std::is_same_v<CharT, wchar_t>) {
                        out = std::ranges::copy(std::format(L"{:d}", as<int_type>()), out).out; break;
                    } else { out = std::ranges::copy(std::format("{:d}", as<int_type>()), out).out; break; }
                }
                case document_tree_node_type::floating_point: {
                    if constexpr (std::is_same_v<CharT, wchar_t>) {
                        out = std::ranges::copy(std::format(L"{:f}", as<float_type>()), out).out; break;
                    } else { out = std::ranges::copy(std::format("{:f}", as<float_type>()), out).out; break; }
                }
                case document_tree_node_type::string:           { *out++ = CharT{'\"'}; out = std::ranges::copy(buffer, out).out; *out++ = CharT{'\"'}; break; }
                case document_tree_node_type::object: *out++ = CharT{'{'}; break;
                case document_tree_node_type::array:  *out++ = CharT{'['}; break;
                case document_tree_node_type::root: break;
                }
            } else {
                switch (type) {
                default: break;
                case document_tree_node_type::object: *out++ = CharT{'}'}; break;
                case document_tree_node_type::array:  *out++ = CharT{']'}; break; 
                }
            }
            return out;
        }
        
    };

    template <typename Integer = int, typename FloatingPoint = float, class CharT = char, class BufferAllocator = std::allocator<CharT>>
    class document_tree_node {
    public:
        using string              = std::basic_string<CharT, std::char_traits<CharT>, BufferAllocator>;
        using string_view         = std::basic_string_view<CharT, std::char_traits<CharT>>;
        using pointer             = document_tree_node*;
        using const_pointer       = const document_tree_node*;
        using reference           = document_tree_node&;
        using const_reference     = const document_tree_node&;
        using int_type            = Integer;
        using float_type          = FloatingPoint;
        using value_type          = document_node_value<int_type, float_type, CharT, BufferAllocator>;
    private:
        string            name_;
        value_type        value_;
        std::ptrdiff_t    pid_ = -1;
        bool              tombed_ = false;
    public:
        constexpr document_tree_node(pointer parent, pointer beg, string_view n, value_type v)
            : name_(n), value_(std::move(v)), pid_(parent != nullptr ? parent - beg : -1) {}

        constexpr document_tree_node(std::ptrdiff_t pid, string_view n, value_type v)
            : name_(n), value_(std::move(v)), pid_(pid) {}

        constexpr document_tree_node()                                       = default;
        constexpr document_tree_node(const document_tree_node&)              = default;
        constexpr document_tree_node(document_tree_node&&)                   = default;
        constexpr document_tree_node& operator=(const document_tree_node&)   = default;
        constexpr document_tree_node& operator=(document_tree_node&&)        = default;
        constexpr ~document_tree_node()                                      = default;

        constexpr bool              dying()           const { return tombed_; }
        constexpr void              dying(bool v)           { tombed_ = v; }
        constexpr string&           name()                  { return name_; }
        constexpr string_view       name()            const { return name_; }
        constexpr value_type&       value()                 { return value_; }
        constexpr value_type        value()           const { return value_; }
        constexpr auto              get_allocator()   const { return value_.get_allocator(); }
        constexpr std::ptrdiff_t&   parent_index()          { return pid_; }
        constexpr std::ptrdiff_t    parent_index()    const { return pid_; }
    };

    template <class JsonTree>
    class document_tree_node_const_iterator {
    public:
        using tree_type         = JsonTree;
        using string_view       = typename tree_type::string_view;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept  = std::contiguous_iterator_tag;

        using node_value          = typename tree_type::node_value;
        using value_type         = typename tree_type::value_type;
        using pointer            = typename tree_type::const_pointer;
        using reference          = typename tree_type::const_reference;
        using container_iterator = typename tree_type::container_const_iterator;
    protected:
        pointer       node_ptr_ = nullptr;
        tree_type*    tree_ptr_ = nullptr;
    public:
        ~document_tree_node_const_iterator() = default;
        constexpr document_tree_node_const_iterator(const document_tree_node_const_iterator&) = default;
        constexpr document_tree_node_const_iterator(document_tree_node_const_iterator&&) = default;
        constexpr document_tree_node_const_iterator() = default;
        
        constexpr document_tree_node_const_iterator(const tree_type* const tp, const pointer np)
            : node_ptr_(const_cast<pointer>(np)), tree_ptr_(const_cast<tree_type*>(tp)) {}

        constexpr document_tree_node_const_iterator& operator=(const document_tree_node_const_iterator&) = default;
        constexpr document_tree_node_const_iterator& operator=(document_tree_node_const_iterator&&)      = default;

        constexpr reference       operator*()               const { return *node_ptr_; }
        constexpr pointer         operator->()              const { return node_ptr_; }

        constexpr document_tree_node_const_iterator& operator++()                             { ++node_ptr_; return *this; }
        constexpr document_tree_node_const_iterator  operator++(int)                          { return document_tree_node_const_iterator( tree_ptr_, node_ptr_++); }
        constexpr document_tree_node_const_iterator& operator+=(const std::ptrdiff_t d)       { node_ptr_ += d; return *this; }
        constexpr document_tree_node_const_iterator  operator+ (const std::ptrdiff_t d) const { return document_tree_node_const_iterator(tree_ptr_, node_ptr_ + d); }

        constexpr document_tree_node_const_iterator& operator--()                             { --node_ptr_; return *this; }
        constexpr document_tree_node_const_iterator  operator--(int)                          { return document_tree_node_const_iterator(tree_ptr_, node_ptr_--); }
        constexpr document_tree_node_const_iterator& operator-=(const std::ptrdiff_t d)       { node_ptr_ -= d; return *this; }
        constexpr document_tree_node_const_iterator  operator- (const std::ptrdiff_t d) const { return document_tree_node_const_iterator(tree_ptr_, node_ptr_ - d); }
        
        constexpr bool operator==(const document_tree_node_const_iterator& right) const { return tree_ptr_ == right.tree_ptr_ && node_ptr_ == right.node_ptr_; }
        constexpr bool operator!=(const document_tree_node_const_iterator& right) const { return node_ptr_ != right.node_ptr_ || tree_ptr_ != right.tree_ptr_; }
        constexpr bool operator< (const document_tree_node_const_iterator& right) const { return tree_ptr_ == right.tree_ptr_ && node_ptr_ <  right.node_ptr_; }
        constexpr bool operator> (const document_tree_node_const_iterator& right) const { return tree_ptr_ == right.tree_ptr_ && node_ptr_ >  right.node_ptr_; }
        constexpr bool operator<=(const document_tree_node_const_iterator& right) const { return tree_ptr_ == right.tree_ptr_ && node_ptr_ <= right.node_ptr_; }
        constexpr bool operator>=(const document_tree_node_const_iterator& right) const { return tree_ptr_ == right.tree_ptr_ && node_ptr_ >= right.node_ptr_; }

        constexpr std::ptrdiff_t                      operator-(const document_tree_node_const_iterator& rhs) const { return node_ptr_ - rhs.node_ptr_; }
        constexpr friend document_tree_node_const_iterator operator+(const std::ptrdiff_t d, const document_tree_node_const_iterator& it) { return it + d; }

        constexpr document_tree_node_const_iterator parent()                        const {
            return node_ptr_->parent_index() != -1 ? document_tree_node_const_iterator(tree_ptr_, tree_ptr_->data() + node_ptr_->parent_index()) : *this;
        }
        
        constexpr document_tree_node_const_iterator begin()                   const { return tree_ptr_->search_child_begin(*this); }
        constexpr document_tree_node_const_iterator end()                     const { return tree_ptr_->search_child_end(*this); }
        constexpr std::size_t                       size()                    const { return end() - begin(); }
        constexpr auto                              find(string_view name) const { return tree_ptr_->access(*this, name); }
        constexpr auto                              find(std::size_t id)   const { return tree_ptr_->access(*this, id); }
    };

    template <class JsonTree>
    class document_tree_node_iterator : protected document_tree_node_const_iterator<JsonTree> {
        using base = document_tree_node_const_iterator<JsonTree>;
        constexpr document_tree_node_iterator(const base& right) : base(right) {}
    public:
        using tree_type         = typename base::tree_type;
        using difference_type   = typename base::difference_type;
        using iterator_category = typename base::iterator_category;
        using iterator_concept  = typename base::iterator_concept;
        
        using node_value          = typename tree_type::node_value;
        using string_view         = typename tree_type::string_view;
        using value_type          = typename tree_type::value_type;
        using pointer             = typename tree_type::pointer;
        using reference           = typename tree_type::reference;
        using container_iterator  = typename tree_type::container_iterator;
    protected:
        using base::node_ptr_;
        using base::tree_ptr_;
    public:
        ~document_tree_node_iterator() = default;
        constexpr document_tree_node_iterator(const document_tree_node_iterator&) = default;
        constexpr document_tree_node_iterator(document_tree_node_iterator&&)      = default;
        constexpr document_tree_node_iterator() = default;
        
        constexpr document_tree_node_iterator(const tree_type* const tp, const pointer np)            : base(tp, np) {}

        constexpr document_tree_node_iterator& operator=(const document_tree_node_iterator&) = default;
        constexpr document_tree_node_iterator& operator=(document_tree_node_iterator&&)      = default;

        constexpr reference       operator*()               const { return const_cast<reference>(base::operator*()); }
        constexpr pointer         operator->()              const { return const_cast<pointer>  (base::operator->()); }

        constexpr document_tree_node_iterator& operator++()                             { base::operator++(); return *this; }
        constexpr document_tree_node_iterator  operator++(int)                          { return base::operator++(0); }
        constexpr document_tree_node_iterator& operator+=(const std::ptrdiff_t d)       { base::operator+=(d); return *this; }
        constexpr document_tree_node_iterator  operator+ (const std::ptrdiff_t d) const { return base::operator+ (d); }

        constexpr document_tree_node_iterator& operator--()                             { base::operator--(); return *this;  }
        constexpr document_tree_node_iterator  operator--(int)                          { return base::operator--(0); }
        constexpr document_tree_node_iterator& operator-=(const std::ptrdiff_t d)       { base::operator-=(d); return *this; }
        constexpr document_tree_node_iterator  operator- (const std::ptrdiff_t d) const { return base::operator- (d); }

        constexpr bool operator==(const document_tree_node_iterator& right) const { return base::operator==(right); }
        constexpr bool operator!=(const document_tree_node_iterator& right) const { return base::operator!=(right); }
        constexpr bool operator< (const document_tree_node_iterator& right) const { return base::operator<(right); }
        constexpr bool operator> (const document_tree_node_iterator& right) const { return base::operator>(right); }
        constexpr bool operator<=(const document_tree_node_iterator& right) const { return base::operator<=(right); }
        constexpr bool operator>=(const document_tree_node_iterator& right) const { return base::operator>=(right); }
        
        constexpr std::ptrdiff_t                operator-(const document_tree_node_iterator& rhs) const { return base::operator-(rhs); }
        constexpr friend document_tree_node_iterator operator+(const std::ptrdiff_t d, const document_tree_node_iterator& it) { return it + d; }

        constexpr document_tree_node_iterator parent()     const { return base::parent(); }
        constexpr document_tree_node_iterator begin()      const { return base::begin();  }
        constexpr document_tree_node_iterator end()        const { return base::end();    }
        constexpr std::size_t                 size()       const { return end() - begin(); }

        constexpr document_tree_node_iterator emplace(string_view name, const node_value& value) {
            return tree_ptr_->emplace(*this, name, value);
        }

        constexpr auto   operator[](string_view name)       { return tree_ptr_->insert_or_access(*this, name); }
        constexpr auto   operator[](std::size_t id)         { return tree_ptr_->insert_or_access(*this, id); }
        constexpr auto   find(string_view name)             { return tree_ptr_->access(*this, name); }
        constexpr auto   find(std::size_t id)               { return tree_ptr_->access(*this, id); }
        constexpr auto   find(string_view name)       const { return tree_ptr_->access(*this, name); }
        constexpr auto   find(std::size_t id)         const { return tree_ptr_->access(*this, id); }

        constexpr document_tree_node_iterator remove() {
            tree_ptr_->remove(*this);
            return tree_ptr_->begin();
        }
    };
    
    template <typename Integer         = int,
              typename FloatingPoint   = float,
              class CharT              = char,
              class BufferAllocator    = std::allocator<CharT>,
              template <class Ty> class TreeAllocator = std::allocator
    >
    class document_tree_batch_inserter {
        struct inserter_node;
    public:
        using string_view        = std::basic_string_view<CharT, std::char_traits<CharT>>;
        using string             = std::basic_string<CharT, std::char_traits<CharT>, BufferAllocator>;
        using node_value         = document_node_value<Integer, FloatingPoint, CharT, BufferAllocator>;
        using float_type         = FloatingPoint;
        using int_type           = Integer;

        using container_allocator = TreeAllocator<inserter_node>;
        using container           = std::forward_list<inserter_node, container_allocator>;
        using container_iterator  = typename container::iterator;
        
        constexpr document_tree_batch_inserter(const document_tree_batch_inserter&) = default;
        constexpr document_tree_batch_inserter(document_tree_batch_inserter&&)      = default;
        constexpr document_tree_batch_inserter& operator=(const document_tree_batch_inserter&) = default;
        constexpr document_tree_batch_inserter& operator=(document_tree_batch_inserter&&)      = default;
        
        constexpr document_tree_batch_inserter(const BufferAllocator& buf_alloc = BufferAllocator{}, const container_allocator& tree_alloc = container_allocator{})
        : nodes_(tree_alloc), max_depth_(0) {
            tail_ = nodes_.emplace_after(nodes_.before_begin(), 0, nodes_.end(), string_view(), node_value(document_node_root_tag, buf_alloc), buf_alloc); ++size_;
        }

        constexpr container_iterator emplace(container_iterator parent, string_view name, node_value&& value, const BufferAllocator& ba = BufferAllocator{}) {
            std::size_t d = 1; for (auto it = parent; it != root(); it = it->parent) { ++d; }
            tail_ = nodes_.emplace_after(tail_, d, parent, name, std::move(value), ba);
            max_depth_ = std::max(max_depth_, d); ++size_;
            return tail_;
        }

        constexpr container_iterator root()        { return nodes_.begin(); }
        constexpr std::size_t        depth() const { return max_depth_; }
        constexpr std::size_t        size()  const { return size_; }

        constexpr container_iterator begin()   { return nodes_.begin(); }
        constexpr container_iterator end()     { return nodes_.end(); }
    private:
        struct inserter_node {
            std::size_t        depth;
            container_iterator parent;
            string             name;
            node_value         value;
            std::size_t        index = 0;
            constexpr inserter_node(std::size_t d, container_iterator parent, string_view name, node_value&& value, const BufferAllocator& ba = BufferAllocator{})
            : depth(d), parent(parent), name(name, ba), value(std::move(value)) {}
        };
        container          nodes_;
        container_iterator tail_;
        std::size_t        max_depth_;
        std::size_t        size_ = 0;
    };

    template <typename Integer         = int,
              typename FloatingPoint   = float,
              class    CharT           = char,
              class    BufferAllocator = std::allocator<CharT>,
              class    TreeAllocator   = std::allocator<document_tree_node<Integer, FloatingPoint, CharT, BufferAllocator>>
    >
    class document_tree {
    public:
        using string_view       = std::basic_string_view<CharT, std::char_traits<CharT>>;
        using string            = std::basic_string<CharT, std::char_traits<CharT>, BufferAllocator>;
        using node_value        = document_node_value<Integer, FloatingPoint, CharT, BufferAllocator>;
        using float_type        = FloatingPoint;
        using int_type          = Integer;
        
        using allocator_type    = TreeAllocator;
        using value_type        = document_tree_node<Integer, FloatingPoint, CharT, BufferAllocator>;
        using reference         = value_type&;
        using const_reference   = const value_type&;
        using pointer           = value_type*;
        using const_pointer     = const value_type*;
        using difference_type   = std::ptrdiff_t;

        using container                        = std::vector<value_type, allocator_type>;
        using container_iterator               = typename container::iterator;
        using container_const_iterator         = typename container::const_iterator;
        using container_reverse_iterator       = typename container::reverse_iterator;
        using container_const_reverse_iterator = typename container::const_reverse_iterator;

        using iterator               = document_tree_node_iterator<document_tree>;
        using const_iterator         = document_tree_node_const_iterator<document_tree>;
    protected:
        container             nodes_;
        static constexpr auto upper_bound_proj = [](const value_type& v) { return v.parent_index(); };

        template <class ... Args>
        constexpr document_tree(allocator_type alloc, std::size_t init_cap, pointer parent, Args&& ... args) : nodes_(alloc) {
            if (parent->value().type)
            nodes_.reserve(init_cap);
            emplace_back_(parent, std::forward<Args>(args)...);
        }

        template <class ... Args>
        constexpr container_iterator emplace_back_(pointer parent, Args&& ... args) {
            return nodes_.emplace(nodes_.end(), parent, std::forward<Args>(args)...);
        }
        
        template <class ... Args>
        constexpr container_iterator emplace_sorted_(pointer parent, Args&& ... args) {
            auto insert_pos = std::ranges::upper_bound(nodes_, parent - data(), std::less<difference_type>(), upper_bound_proj) - nodes_.begin();
            auto update_itr = std::ranges::upper_bound(nodes_, insert_pos - 1 , std::less<difference_type>(), upper_bound_proj);
            std::ranges::for_each(update_itr, nodes_.end(), [](value_type& nd) { ++nd.parent_index(); });
            return nodes_.emplace(nodes_.begin() + insert_pos, parent, std::forward<Args>(args)...);
        }
        
        template <class ... Args>
        constexpr container_iterator emplace_auto_(pointer parent, Args&& ... args) {
            if (parent->value().parent_type()) {
                return parent - data() >= nodes_.back().parent_index() ?
                emplace_back_(parent, std::forward<Args>(args)...) : emplace_sorted_(parent, std::forward<Args>(args)...);
            }
            return nodes_.end();
        }

        constexpr void               tag_current_and_all_children_to_unknow_(container_iterator root) {
            if (root == nodes_.end()) { return; }
            root->dying(true);
            auto beg = std::ranges::upper_bound(root, nodes_.end(), root - nodes_.begin() - 1, std::less<difference_type>(), upper_bound_proj);
            auto end = std::ranges::upper_bound(root, nodes_.end(), root - nodes_.begin(),     std::less<difference_type>(), upper_bound_proj);
            for (;beg != end; ++beg) { tag_current_and_all_children_to_unknow_(beg); }
        }

        constexpr container_iterator erase_single_node_and_rotate_(container_iterator which) {
            auto dbg = std::ranges::upper_bound(nodes_, which - nodes_.begin() - 1, std::less<difference_type>(), upper_bound_proj);
            std::ranges::for_each(dbg, nodes_.end(), [](value_type& n) { --n.parent_index(); });
            return nodes_.erase(which);
        }

        constexpr container_iterator erase_all_unknows_(container_iterator from) {
            for (;from != nodes_.end(); ++from) {
                if (from->dying()) {  from = erase_single_node_and_rotate_(from); --from; }
            }
            return from;
        }

        ///////////////////////////////////////////////////////////////////////////////////
        ///                             Output Method                                   ///
        ///////////////////////////////////////////////////////////////////////////////////

        template <class OutputIt>
        constexpr OutputIt format_to_impl(std::size_t depth, const_iterator it, OutputIt out) const {
            if (it == end()) {
                return out;
            }
            const bool is_parent_t_node  = it->value().parent_type();
            const bool is_last_sibling   = it == std::prev(it.parent().end());
            out = std::ranges::fill_n(out, depth << 1, CharT{' '});
            if (!it->name().empty()) {
                *out++ = CharT{'\"'};
                 out   = std::ranges::copy(it->name(), out).out;
                *out++ = CharT{'\"'};
                *out++ = CharT{':'};
            }
            out = it->value().template format_to<true>(out);
            if (!is_parent_t_node && !is_last_sibling) { *out++ = CharT{','}; } *out++ = CharT{'\n'};
            for (auto c = it.begin(); c != it.end(); ++c) {
                out = format_to_impl(depth + 1, c, out);
            }
            if (is_parent_t_node) { out = std::ranges::fill_n(out, depth << 1, CharT{' '}); }
            out = it->value().template format_to<false>(out);
            if (is_parent_t_node) {
                if (!is_last_sibling) { *out++ = CharT{','}; }
                *out++ = CharT{'\n'};
            }
            return out;
        }

        template <template <class Ty> class InserterAllocator, class JsonTree>
        friend class document_tree_parser;
    public:
        constexpr document_tree(std::size_t init_cap = 1024, const BufferAllocator& buf_alloc = BufferAllocator{}, const TreeAllocator& tree_alloc = TreeAllocator{})
        : nodes_(tree_alloc) {
            nodes_.reserve(init_cap);
            emplace_back_(nullptr, data(), "", node_value{document_node_root_tag, buf_alloc});
        }
        
        constexpr std::size_t        size() const noexcept { return nodes_.size(); }
        constexpr pointer            data()       noexcept { return nodes_.data(); }
        constexpr const_pointer      data() const noexcept { return nodes_.data(); }
        constexpr decltype(auto)     begin()         { return iterator(this, data());                         }
        constexpr decltype(auto)     end()           { return iterator(this, data() + size());                }
        constexpr decltype(auto)     begin()   const { return const_iterator(this, data());                   }
        constexpr decltype(auto)     end()     const { return const_iterator(this, data() + size());          }
        constexpr decltype(auto)     root()    const { return begin() + 1; }
        constexpr decltype(auto)     root()          { return begin() + 1; }
        
        constexpr iterator       emplace(iterator parent, string_view name, const node_value& value) {
            return iterator(this, &*emplace_auto_(&*parent, data(), name, value));
        }

        constexpr const_iterator search_child_begin(const_iterator parent) const {
            auto it = std::ranges::upper_bound(nodes_.begin() + (parent - begin()), nodes_.end(), parent - begin() - 1, std::less<difference_type>(), upper_bound_proj);
            return const_iterator(this, it == nodes_.end() || (it->parent_index() != parent - begin()) ? data() + size() : &*it);
        }

        constexpr const_iterator search_child_end(const_iterator parent) const {
            auto it = std::ranges::upper_bound(nodes_.begin() + (parent - begin()), nodes_.end(), parent - begin(), std::less<difference_type>(), upper_bound_proj);
            return const_iterator(this, it == nodes_.end() || ((it - 1)->parent_index() != parent - begin()) ? data() + size() : &*it);
        }

        constexpr void           remove(iterator which) {
            tag_current_and_all_children_to_unknow_(nodes_.begin() + (which - begin()));
        }

        constexpr iterator       erase(iterator from) {
            auto it = erase_all_unknows_(nodes_.begin() + (from - begin()));
            return iterator(this, it != nodes_.end() ? &*it : (data() + size()));
        }

        constexpr iterator           insert_or_access(iterator actual_root, string_view name) {
            auto it = std::ranges::find_if(actual_root.begin(), actual_root.end(), [name](auto& v) { return v.name() == name; });
            return it == actual_root.end() ? actual_root.emplace(name, {}) : it;
        }

        constexpr iterator           insert_or_access(iterator actual_root, std::size_t i) {
            for (std::ptrdiff_t j = 0; j < static_cast<std::ptrdiff_t>(i + 1 - (actual_root.end() - actual_root.begin())); ++j) { actual_root.emplace("", {}); }
            return (actual_root.begin() + i);
        }

        constexpr const_iterator     access(const_iterator actual_root, string_view name) const {
            auto it = std::ranges::find_if(actual_root.begin(), actual_root.end(), [name](const auto& v) {
                return v.name() == name;
            });
            return it == actual_root.end() ? end() : it;
        }

        constexpr const_iterator     access(const_iterator actual_root, std::size_t i) const {
            if (i + 1 > static_cast<std::size_t>(actual_root.end() - actual_root.begin())) { return end(); }
            return actual_root.begin() + i;
        }

        constexpr iterator           access(iterator actual_root, string_view name) {
            auto it = std::ranges::find_if(actual_root.begin(), actual_root.end(), [name](const auto& v) {
                return v.name() == name;
            });
            return it == actual_root.end() ? end() : it;
        }

        constexpr iterator           access(iterator actual_root, std::size_t i) {
            if (i + 1 > static_cast<std::size_t>(actual_root.end() - actual_root.begin())) { return end(); }
            return actual_root.begin() + i;
        }

        constexpr iterator           operator[](std::string_view name)       { return insert_or_access(begin() + 1, name); }
        constexpr iterator           operator[](std::size_t      id)         { return insert_or_access(begin() + 1, id); }
        constexpr iterator           find(std::string_view name)             { return access(begin() + 1, name); }
        constexpr iterator           find(std::size_t      id)               { return access(begin() + 1, id); }
        constexpr const_iterator     find(std::string_view name)       const { return access(begin() + 1, name); }
        constexpr const_iterator     find(std::size_t      id)         const { return access(begin() + 1, id); }
        
        template <class OutputIt>
        constexpr OutputIt format_to(OutputIt out) const {
            return format_to_impl(0, begin() + 1, out);  // Not format root.
        }
    };

    template <template <class Ty> class InserterAllocator = std::allocator, class JsonTree = document_tree<>>
    class document_tree_parser {
    public:
        using int_type           = typename JsonTree::int_type;
        using float_type         = typename JsonTree::float_type;
        using string_view        = typename JsonTree::string_view;
        using string             = typename JsonTree::string;
        using char_type          = typename JsonTree::string::value_type;
        using node_value         = typename JsonTree::node_value;
        using inserter           = document_tree_batch_inserter<int_type, float_type, char_type, typename string::allocator_type, InserterAllocator>;
        using inserter_allocator = typename inserter::container_allocator;

        JsonTree& tree;

        template <class InputIt>
        static constexpr InputIt parse_spaces(InputIt beg, InputIt end) {
            for (;beg != end && std::isspace(*beg); ++beg) { }
            return beg;
        }
        
        template <class InputIt>
        static constexpr InputIt parse_name_or_string(bool& is_name, string& str, InputIt beg, InputIt end) {
            for (;beg != end && *beg != char_type{'\"'}; ++beg) { str.push_back(*beg); } ++beg;
            beg = parse_spaces(beg, end);
            if (*beg == char_type{':'}) { is_name = true; ++beg; }
            else { is_name = false; } 
            return beg;
        }

        template <class InputIt>
        static constexpr InputIt parse_number(bool& is_float, string& buffer, InputIt beg, InputIt end) {
            for (;beg != end && (std::isalnum(*beg) || *beg == char_type{'.'} || *beg == char_type{'+'} || *beg == char_type{'-'}); ++beg) {
                if (*beg == char_type{'.'} || *beg == char_type{'e'} || *beg == char_type{'E'}) {
                    is_float = true;
                }
                buffer.push_back(*beg);
            }
            return beg;
        }

        template <typename Ty>
        static constexpr auto   emplace_value(inserter& inserter, typename inserter::container_iterator current, typename inserter::container_iterator current_parent, Ty&& value) {
            if (current_parent->value.type == document_tree_node_type::array) {
                current = inserter.emplace(current_parent, string_view(), std::forward<Ty>(value));
            } else { current->value = std::forward<Ty>(value); }
            return current;
        }

        template <class InputIt>
        constexpr InputIt operator()(InputIt beg, InputIt end, const typename string::allocator_type& sa = typename string::allocator_type{},  const inserter_allocator& ia = inserter_allocator{}) {
            string                                   buffer; buffer.reserve(2048);
            inserter                                 inserter{sa, ia};
            typename inserter::container_iterator    current = inserter.root(), current_parent = inserter.root();
            for (;beg != end;) {
                switch (*beg) {
                default: return beg;
                case char_type{' '}: case char_type{'\n'}: case char_type{'\t'}: case char_type{'\r'}: beg = parse_spaces(beg, end); break;
                case char_type{'\"'}: {
                    bool is_name = false; ++beg;
                    beg = parse_name_or_string(is_name, buffer, beg, end);
                    current = is_name ? inserter.emplace(current_parent, buffer, {}, sa) : emplace_value(inserter, current, current_parent,  node_value(buffer, sa));
                    buffer.clear();
                } break;
                case char_type{'{'}: {
                    current = current->name.empty() ?  inserter.emplace(current_parent, "", document_node_object_tag, sa) :
                    emplace_value(inserter, current, current_parent, node_value(document_node_object_tag, sa)); 
                    current_parent = current; ++beg;
                } break;
                case char_type{'['}: {
                    current = current->name.empty() ?  inserter.emplace(current_parent, "", document_node_array_tag, sa) :
                    emplace_value(inserter, current, current_parent, node_value(document_node_array_tag, sa));
                    current_parent = current; ++beg;
                } break;
                case char_type{'}'}: case char_type{']'}:{
                    current = current_parent;
                    current_parent = current_parent->parent; ++beg;
                } break;
                case char_type{'0'}: case char_type{'1'}: case char_type{'2'}: case char_type{'3'}: case char_type{'4'}:
                case char_type{'5'}: case char_type{'6'}: case char_type{'7'}: case char_type{'8'}: case char_type{'9'}:
                case char_type{'-'}: {
                    bool is_float = false;
                    beg = parse_number(is_float, buffer, beg, end);
                    if (is_float) {
                        float_type f;
                        std::from_chars(buffer.data(), buffer.data() + buffer.size(), f, std::chars_format::general);
                        emplace_value(inserter, current, current_parent, node_value(f, sa));
                    } else {
                        int_type i;
                        std::from_chars(buffer.data(), buffer.data() + buffer.size(), i, 10);
                        emplace_value(inserter, current, current_parent, node_value(i, sa));
                    }
                    buffer.clear();
                } break;
                case char_type{'t'}: {
                    ++beg; if (*beg != char_type{'r'}) { break; }
                    ++beg; if (*beg != char_type{'u'}) { break; }
                    ++beg; if (*beg != char_type{'e'}) { break; } ++beg;
                    emplace_value(inserter, current, current_parent, node_value(true, sa));
                } break;
                case char_type{'f'}: {
                    ++beg; if (*beg != char_type{'a'}) { break; }
                    ++beg; if (*beg != char_type{'l'}) { break; }
                    ++beg; if (*beg != char_type{'s'}) { break; }
                    ++beg; if (*beg != char_type{'e'}) { break; } ++beg;
                    emplace_value(inserter, current, current_parent, node_value(false, sa));
                } break;
                case char_type{'n'}: {
                    ++beg; if (*beg != char_type{'u'}) { break; }
                    ++beg; if (*beg != char_type{'l'}) { break; }
                    ++beg; if (*beg != char_type{'l'}) { break; } ++beg;
                    emplace_value(inserter, current, current_parent, node_value(sa));
                }
                case char_type{','}: ++beg; break;
                }
            }
            // Copy data from depth first tree to breadth first tree.
            tree.nodes_.resize(inserter.size());
            std::size_t counter = 1;
            for (std::size_t i = 1; i != inserter.depth() + 1; ++i) {
                auto layer = inserter | std::views::filter([i](auto& node) { return node.depth == i; });
                for (auto& j : layer) {
                    j.index = counter;
                    std::construct_at(&tree.nodes_[counter], j.parent->index, j.name, j.value);
                    ++counter;
                }
            }
            return beg;
        }
    };
}
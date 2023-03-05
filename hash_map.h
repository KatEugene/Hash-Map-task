//
// Created by eugene on 22.01.23.
//

#pragma once

#include <vector>
#include <typeindex>
#include <utility>
#include <cstddef>
#include <stdexcept>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    class SmartPair {
    public:
        using PairType = std::pair<const KeyType, ValueType>;

        SmartPair() = default;
        SmartPair(const KeyType &key, const ValueType &value) : pair_({key, value}) {}
        explicit SmartPair(std::pair<const KeyType, ValueType> pair) : pair_(pair) {}
        PairType &get_pair() {
            return pair_;
        }
        const PairType &get_pair() const {
            return pair_;
        }
        KeyType get_first() const {
            return pair_.first;
        }
        ValueType get_second() const {
            return pair_.second;
        }
        ValueType &get_ref_second() {
            return pair_.second;
        }
        const ValueType &get_ref_second() const {
            return pair_.second;
        }
        std::pair<KeyType &, ValueType &> get_ref() {
            return std::pair<KeyType &, ValueType &>(const_cast<KeyType &>(pair_.first), pair_.second);
        }
        SmartPair &operator=(const SmartPair &other) {
            get_ref() = other.pair_;
            return *this;
        }
    private:
        PairType pair_;
    };
    struct slot {
        SmartPair element;
        bool empty = true;
        bool deleted = false;

        slot() = default;
        slot(KeyType key, ValueType value) : element(key, value), empty(false) {}
        explicit slot(std::pair<KeyType, ValueType> other_element) : element(other_element), empty(false) {}
        slot(const slot &other) {
            element = other.element;
            empty = other.empty;
            deleted = other.deleted;
        }
        KeyType get_key() const {
            return element.get_first();
        }
        ValueType get_value() const {
            return element.get_second();
        }
        slot &operator=(const slot &other) {
            element = other.element;
            empty = other.empty;
            deleted = other.deleted;
            return *this;
        }
        bool operator==(const slot &other) {
            return element.get_pair() == other.element.get_pair() and empty == other.empty and deleted == other.deleted;
        }
    };
    class iterator {
    public:
        using IteratorType = typename std::vector<slot>::iterator;

        iterator() = default;
        iterator(IteratorType pointer, IteratorType end) :
                pointer_(pointer), end_(end) {
        }
        std::pair<const KeyType, ValueType> &operator*() const {
            return pointer_->element.get_pair();
        }
        std::pair<const KeyType, ValueType> *operator->() const {
            return &(pointer_->element.get_pair());
        }
        iterator &operator++() {
            ++pointer_;
            while (pointer_ != end_ and (pointer_->empty or pointer_->deleted)) {
                ++pointer_;
            }
            return *this;
        }
        iterator operator++(int) {
            iterator copy_iterator(*this);
            ++pointer_;
            while (pointer_ != end_ and (pointer_->empty or pointer_->deleted)) {
                ++pointer_;
            }
            return copy_iterator;
        }
        iterator operator+(size_t shift) const {
            iterator copy_iterator(*this);
            copy_iterator.pointer_ += shift;
            return copy_iterator;
        }
        bool operator==(const iterator &other) const {
            return pointer_ == other.pointer_;
        }
        bool operator!=(const iterator &other) const {
            return pointer_ != other.pointer_;
        }
    private:
        IteratorType pointer_;
        IteratorType end_;
    };
    class const_iterator {
    public:
        using ConstIteratorType = typename std::vector<slot>::const_iterator;

        const_iterator() = default;
        const_iterator(ConstIteratorType pointer, ConstIteratorType end) : pointer_(pointer), end_(end) {
        }
        const std::pair<const KeyType, ValueType> &operator*() const {
            return pointer_->element.get_pair();
        }
        const std::pair<const KeyType, ValueType> *operator->() const {
            return &(pointer_->element.get_pair());
        }
        const_iterator &operator++() {
            ++pointer_;
            while (pointer_ != end_ and (pointer_->empty or pointer_->deleted)) {
                ++pointer_;
            }
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator copy_iterator(*this);
            ++pointer_;
            while (pointer_ != end_ and (pointer_->empty or pointer_->deleted)) {
                ++pointer_;
            }
            return copy_iterator;
        }
        bool operator==(const const_iterator &other) const {
            return pointer_ == other.pointer_;
        }
        bool operator!=(const const_iterator &other) const {
            return pointer_ != other.pointer_;
        }
    private:
        ConstIteratorType pointer_;
        ConstIteratorType end_;
    };

    explicit HashMap(Hash hash = Hash()) : hash_(hash), hash_map_(std::vector<slot>(sizes.front())) {}
    template<class Iterator>
    HashMap(Iterator begin, Iterator end, Hash hash = Hash())
            : hash_(hash), hash_map_(std::vector<slot>(sizes.front())) {
        while (begin != end) {
            insert(*begin);
            ++begin;
        }
    }
    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> seq, Hash hash = Hash())
            : hash_(hash), hash_map_(std::vector<slot>(sizes.front())) {
        for (auto element : seq) {
            insert(element);
        }
    }

    size_t size() const {
        return real_size_;
    }
    bool empty() const {
        return real_size_ == 0;
    }
    Hash hash_function() const {
        return hash_;
    }

    void insert(std::pair<KeyType, ValueType> element) {
        size_t index = get_index(element.first);
        if (hash_map_[index].empty or (hash_map_[index].get_key() == element.first and hash_map_[index].deleted)) {
            hash_map_[index] = slot(element);
            ++real_size_;
            ++captured_cnt_;
        }

        if (static_cast<double>(captured_cnt_) / hash_map_.size() >= MAX_LOAD_FACTOR) {
            rebuild();
        }
    }
    void erase(KeyType key) {
        size_t index = get_index(key);
        if (not hash_map_[index].empty and not hash_map_[index].deleted and hash_map_[index].get_key() == key) {
            hash_map_[index].deleted = true;
            --real_size_;
        }
    }
    iterator find(KeyType key) {
        size_t index = get_index(key);
        return hash_map_[index].get_key() == key and not hash_map_[index].empty and not hash_map_[index].deleted
               ? iterator(hash_map_.begin() + index, hash_map_.end()) : end();
    }
    const_iterator find(KeyType key) const {
        size_t index = get_index(key);
        return hash_map_[index].get_key() == key and not hash_map_[index].empty and not hash_map_[index].deleted
               ? const_iterator(hash_map_.begin() + index, hash_map_.end()) : end();
    }
    ValueType &operator[](KeyType key) {
        size_t index = get_index(key);
        if (hash_map_[index].empty or (hash_map_[index].get_key() == key and hash_map_[index].deleted)) {
            hash_map_[index] = slot(key, ValueType());
            ++real_size_;
            ++captured_cnt_;
        }

        if (static_cast<double>(captured_cnt_) / hash_map_.size() >= MAX_LOAD_FACTOR) {
            rebuild();
            index = get_index(key);
        }
        return hash_map_[index].element.get_ref_second();
    }
    const ValueType &at(KeyType key) const {
        size_t index = get_index(key);
        if (not(hash_map_[index].get_key() == key)) {
            throw std::out_of_range("There is no such key in hash map");
        }
        return hash_map_[index].element.get_ref_second();
    }
    void clear() {
        cur_size_ = 0;
        real_size_ = 0;
        captured_cnt_ = 0;
        hash_map_.clear();
        hash_map_.resize(sizes.front());
    }

    iterator begin() {
        for (size_t i = 0; i < hash_map_.size(); ++i) {
            if (not hash_map_[i].empty and not hash_map_[i].deleted) {
                return iterator(hash_map_.begin() + i, hash_map_.end());
            }
        }
        return iterator(hash_map_.end(), hash_map_.end());
    }
    iterator end() {
        return iterator(hash_map_.end(), hash_map_.end());
    }
    const_iterator begin() const {
        for (size_t i = 0; i < hash_map_.size(); ++i) {
            if (not hash_map_[i].empty and not hash_map_[i].deleted) {
                return const_iterator(hash_map_.cbegin() + i, hash_map_.cend());
            }
        }
        return const_iterator(hash_map_.cend(), hash_map_.cend());
    }
    const_iterator end() const {
        return const_iterator(hash_map_.cend(), hash_map_.cend());
    }
private:
    constexpr const static double MAX_LOAD_FACTOR = 0.8;
    std::vector<int> sizes =
            {11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877, 205759, 411527, 823117,
             1646237, 3292489, 6584983};
    size_t cur_size_ = 0;
    size_t real_size_ = 0;
    size_t captured_cnt_ = 0;
    Hash hash_;
    std::vector<slot> hash_map_;

    size_t get_index(KeyType key) const {
        size_t shift = hash_(key) % hash_map_.size();
        size_t index = shift;
        if (shift == 0) {
            shift = 1;
        }
        while (not hash_map_[index].empty and not(hash_map_[index].get_key() == key)) {
            index += shift;
            if (index >= hash_map_.size()) {
                index -= hash_map_.size();
            }
        }
        return index;
    }
    void rebuild() {
        std::vector<slot> copy_hash_map = hash_map_;
        hash_map_.clear();
        real_size_ = 0;
        captured_cnt_ = 0;
        ++cur_size_;
        hash_map_.resize(sizes[cur_size_]);
        for (auto slt : copy_hash_map) {
            if (not slt.empty and not slt.deleted) {
                insert({slt.get_key(), slt.get_value()});
            }
        }
    }
};

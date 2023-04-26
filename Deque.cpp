#pragma once
#include <iostream>
#include <vector>
using std::vector;
using std::pair;

template<typename T>
class Deque {

 public:

  Deque() : begin_({0, 0}), end_({0, -1}) {
    pointers.resize(0, nullptr);
  }

  Deque(const Deque &other) {
    if (other.pointers.size() == 0) {
      begin_ = {0, 0};
      end_ = {0, -1};
      return;
    }

    pointers.resize(other.pointers.size(), nullptr);
    int i;
    try {
      for (i = other.begin_.vector_number; i <= other.end_.vector_number; ++i) {
        pointers[i] = reinterpret_cast<T *>(new int8_t[bucket_size * sizeof(T)]);
        for (int j = 0; j < bucket_size; ++j) {
          pointers[i][j] = other.pointers[i][j];
        }
      }
      begin_ = other.begin_;
      end_ = other.end_;
    }

    catch (...) {
      for (int k = other.begin_.vector_number; k < i; ++k) {
        delete[] reinterpret_cast<int8_t *>(pointers[i]);
      }
      pointers.clear();
      pointers.shrink_to_fit();
      throw;
    }
  }

  Deque(int n, const T &value = T()) {
    int cnt_blocks = (n - 1 + bucket_size) / bucket_size;
    pointers.resize(cnt_blocks * 2, nullptr);
    int i;
    int j;
    try {
      for (i = cnt_blocks; i < 2 * cnt_blocks; ++i) {
        pointers[i] = reinterpret_cast<T *>(new int8_t[bucket_size * sizeof(T)]);
        for (j = 0; j < bucket_size && (bucket_size * (i - cnt_blocks)) + j < n; ++j) {
          new(pointers[i] + j) T(value);
        }
      }
      begin_ = {cnt_blocks, 0};
      end_ = {i - 1, j - 1};
    }
    catch (...) {
      for (int k = cnt_blocks; k < i; ++k) {
        for (int l = 0; l < bucket_size && (bucket_size * i) + j < n; ++l) {
          (pointers[k] + l)->~T();
        }
        delete[] reinterpret_cast<int8_t *>(pointers[k]);
      }
      pointers.clear();
      pointers.shrink_to_fit();
      throw;
    }
  }

  ~Deque() {
    for (int i = 0; i < (int) pointers.size(); ++i) {
      delete[] reinterpret_cast<int8_t *>(pointers[i]);
    }
  }

  Deque &operator=(const Deque &other) {
    Deque<T> new_d = Deque(other);
    swap(new_d);
    return *this;
  }

  unsigned int size() const {
    if (end_.vector_number - begin_.vector_number == 0) {
      return end_.pos_in_vector - begin_.pos_in_vector + 1;
    }
    return 1 + end_.pos_in_vector + bucket_size - begin_.pos_in_vector
        + 8 * (end_.vector_number - begin_.vector_number - 1);
  }

  void push_back(const T &value) {
    build_deque();
    if (end_.pos_in_vector == bucket_size - 1) {
      pointers.push_back(nullptr);
      pointers[end_.vector_number + 1] = reinterpret_cast<T *>(new int8_t[bucket_size * sizeof(T)]);
      ++end_.vector_number;
      end_.pos_in_vector = -1;
    }
    try {
      new(pointers[end_.vector_number] + end_.pos_in_vector + 1) T(value);
      ++end_.pos_in_vector;
    }
    catch (...) {
      if (end_.pos_in_vector == bucket_size - 1) {
        pointers.pop_back();
        --end_.vector_number;
        end_.pos_in_vector = bucket_size;
      }
      --end_.pos_in_vector;
      throw;
    }
  }

  void pop_back() {
    if (end_.pos_in_vector == 0) {
      (pointers[end_.vector_number])->~T();
      delete[] reinterpret_cast<int8_t *>(pointers[end_.vector_number]);
      pointers.pop_back();
      if (pointers.size() == 0) {
        end_.vector_number = 0;
        end_.pos_in_vector = -1;
      } else {
        --end_.vector_number;
        end_.pos_in_vector = bucket_size - 1;
      }
    } else {
      (pointers[end_.vector_number] + end_.pos_in_vector)->~T();
      --end_.pos_in_vector;
    }
  }

  void push_front(const T &value) {
    if (pointers.size() == 0) {
      this->push_back(value);
    } else if (begin_.vector_number == 0 && begin_.pos_in_vector == 0) {
      vector<T *> new_d(2 * pointers.size(), nullptr);
      for (size_t i = pointers.size(); i < 2 * pointers.size(); ++i) {
        new_d[i] = pointers[i - pointers.size()];
      }
      vector<T *> copy = pointers;
      pointers = new_d;
      pointers[begin_.vector_number + pointers.size() / 2 - 1] =
          reinterpret_cast<T *>(new int8_t[bucket_size * sizeof(T)]);
      try {
        new(pointers[begin_.vector_number + pointers.size() / 2 - 1] + bucket_size - 1) T(value);
        begin_.vector_number += pointers.size() / 2 - 1;
        end_.vector_number += pointers.size() / 2;
        begin_.pos_in_vector = bucket_size - 1;
      }
      catch (...) {
        pointers = copy;
        throw;
      }
    } else if (begin_.pos_in_vector == 0) {
      pointers[begin_.vector_number - 1] = reinterpret_cast<T *>(new int8_t[bucket_size * sizeof(T)]);
      try {
        new(pointers[begin_.vector_number - 1] + bucket_size - 1) T(value);
        --begin_.vector_number;
        begin_.pos_in_vector = bucket_size - 1;
      }
      catch (...) {
        throw;
      }
    } else {
      try {
        new(pointers[begin_.vector_number] + begin_.pos_in_vector - 1) T(value);
        --begin_.pos_in_vector;
      }
      catch (...) {
        throw;
      }

    }
  }

  void pop_front() {
    if (begin_.pos_in_vector == bucket_size - 1) {
      ++begin_.vector_number;
      begin_.pos_in_vector = -1;
    }
    ++begin_.pos_in_vector;
    (pointers[begin_.vector_number] + begin_.pos_in_vector)->~T();
  }

  T &operator[](int n) {
    return pointers[begin_.vector_number + (begin_.pos_in_vector + n) / 8][(begin_.pos_in_vector + n) % 8];
  }
  const T &operator[](int n) const {
    return pointers[begin_.vector_number + (begin_.pos_in_vector + n) / 8][(begin_.pos_in_vector + n) % 8];
  }
  const T &at(int n) const {
    if (n >= size()) { throw std::out_of_range("..."); }
    return pointers[begin_.vector_number + (begin_.pos_in_vector + n) / 8][(begin_.pos_in_vector + n) % 8];
  }
  T &at(int n) {
    if (n >= int(size())) { throw std::out_of_range("..."); }
    return pointers[begin_.vector_number + (begin_.pos_in_vector + n) / 8][(begin_.pos_in_vector + n) % 8];
  }

 public:
  template<bool IsConst>
  struct common_iterator {
   private:
    using T_c = std::conditional_t<IsConst, const T, T>;
   public:
    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    common_iterator() = default;

    common_iterator(const common_iterator &other) : index(other.index), iter(other.iter) {}
    common_iterator(int bucket_size,
                    int index,
                    std::conditional_t<IsConst,
                                       typename vector<T *>::const_iterator,
                                       typename vector<T *>::iterator> iter) : bucket_size(
        bucket_size), index(index), iter(iter) {}

    common_iterator &operator=(const common_iterator<IsConst> &other) {
      index = other.index;
      iter = other.iter;
      return *this;
    }
    T_c &operator*() {
      return *(*iter + index);
    }
    T_c *operator->() {
      return (*iter + index);
    }
    common_iterator &operator++() {
      if (index == bucket_size - 1) {
        ++iter;
        index = 0;
      } else {
        ++index;
      }
      return *this;
    }
    common_iterator &operator--() {
      if (index == 0) {
        --iter;
        index = bucket_size - 1;
      } else {
        --index;
      }
      return *this;
    }
    common_iterator operator++(int) {
      common_iterator<IsConst> copy = *this;
      ++copy;
      return copy;
    }
    common_iterator operator--(int) {
      common_iterator<IsConst> copy = *this;
      --copy;
      return copy;
    }
    bool operator==(const iterator &other) const {
      if (iter != other.iter) {
        return false;
      }
      return index == other.index;
    }
    bool operator!=(const iterator &other) const {
      return !(*this == other);
    }
    bool operator<(const iterator &other) const {
      return iter < other.iter || (iter == other.iter && index < other.index);
    }
    bool operator<=(const iterator &other) const {
      return *this < other || *this == other;
    }
    bool operator>(const iterator &other) const {
      return !(*this <= other);
    }
    bool operator>=(const iterator &other) const {
      return !(*this < other);
    }
    common_iterator<IsConst> &operator+=(int x) {
      if (x < 0) {
        iter -= ((-x - index + bucket_size - 1) / bucket_size);
        index = (((index + x) % bucket_size) + bucket_size) % bucket_size;
      } else {
        iter += (index + x) / bucket_size;
        index = (index + x) % bucket_size;
      }
      return *this;
    }
    common_iterator<IsConst> &operator-=(int x) {
      return (*this += -x);
    }

    common_iterator<IsConst> operator+(int x) const {
      common_iterator<IsConst> copy = *this;
      return copy += x;
    }
    common_iterator<IsConst> operator-(int x) const {
      common_iterator<IsConst> copy = *this;
      return copy -= x;
    }

    int operator-(const iterator &other) const {
      if (iter - other.iter == 0) {
        return index - other.index;
      }
      return index + bucket_size - other.index + 8 * (iter - other.iter - 1);
    }

    using iterator_category = std::random_access_iterator_tag;
    using value_type = T_c;
    using difference_type = std::ptrdiff_t;
    using pointer = T_c *;
    using reference = T_c &;

   private:
    operator common_iterator<true>() const {
      return common_iterator<true>(bucket_size, index, iter);
    }
    const int bucket_size = 8;
    int index;
    std::conditional_t<IsConst, typename vector<T *>::const_iterator, typename vector<T *>::iterator> iter;
  };

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(bucket_size, begin_.pos_in_vector, pointers.begin() + begin_.vector_number);
  }
  const_iterator begin() const {
    return const_iterator(bucket_size, begin_.pos_in_vector, pointers.begin() + begin_.vector_number);
  }
  iterator end() {
    if (end_.pos_in_vector == bucket_size - 1) {
      return iterator(bucket_size, 0, pointers.end());
    }
    return iterator(bucket_size, end_.pos_in_vector + 1, pointers.end() - 1);
  }
  const_iterator end() const {
    if (end_.pos_in_vector == bucket_size - 1) {
      return const_iterator(bucket_size, 0, pointers.end());
    }
    return const_iterator(bucket_size, end_.pos_in_vector + 1, pointers.end() - 1);
  }
  const_iterator cbegin() const {
    return const_iterator(bucket_size, begin_.pos_in_vector, pointers.begin() + begin_.vector_number);
  }
  const_iterator cend() const {
    if (end_.pos_in_vector == bucket_size - 1) {
      return const_iterator(bucket_size, 0, pointers.end() - 1);
    }
    return const_iterator(bucket_size, end_.pos_in_vector + 1, pointers.end() - 1);
  }
  reverse_iterator rbegin() {
    return --end();
  }
  reverse_iterator rend() {
    return --begin();
  }
  const_reverse_iterator crbegin() const {
    return --cend();
  }
  const_reverse_iterator crend() const {
    return --cbegin();
  }

  void insert(const iterator &other, const T &value) {
    Deque<T> new_d;//
    for (auto it = begin(); it != other; ++it) {
      new_d.push_back(*it);
    }
    new_d.push_back(value);
    for (auto it = other; it != end(); ++it) {
      new_d.push_back(*it);
    }
    swap(new_d);
  }

  void erase(const iterator &other) {
    Deque<T> new_d;
    for (auto it = begin(); it != other; ++it) {
      new_d.push_back(*it);
    }
    for (auto it = other + 1; it != end(); ++it) {
      new_d.push_back(*it);
    }
    swap(new_d);
  }

 private:

  void build_deque() {
    if (pointers.size() == 0) {
      end_ = {0, -1};
      pointers.push_back(nullptr);
      pointers[0] = reinterpret_cast<T *>(new int8_t[bucket_size * sizeof(T)]);
    }
  }
  void swap(Deque &d) {
    std::swap(d.pointers, pointers);
    std::swap(d.begin_, begin_);
    std::swap(d.end_, end_);
  }

 private:
  struct position_in_deque {
    int vector_number;
    int pos_in_vector;
  };

  position_in_deque begin_ = {-1, -1};
  position_in_deque end_ = {-1, -1};
  vector<T *> pointers;
  static const int bucket_size = 8;
};

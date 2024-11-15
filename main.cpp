#include <chrono>
 
#include<atomic>
#include <ranges>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#define print(message) std::cout<<(message)<<std::endl
 

#define string(message) std::to_string(message)
 
#include <stdexcept>

template <typename T>
class SafeSharedPtr {
public:
    SafeSharedPtr() = default;
    explicit SafeSharedPtr(T* ptr) : ptr_(std::shared_ptr<T>(ptr)) {}
    explicit SafeSharedPtr(const std::shared_ptr<T>& ptr) : ptr_(ptr) {}
    explicit SafeSharedPtr(std::shared_ptr<T>&& ptr) : ptr_(std::move(ptr)) {}
    SafeSharedPtr(std::nullptr_t) : ptr_(nullptr) {}
    // Override dereference operator
    T& operator*() const {
        if (!ptr_) {
            throw std::runtime_error("Dereferencing a null shared pointer!");
        }
        return *ptr_;
    }
    T* operator->() const {
        if (!ptr_) {
            throw std::runtime_error("Accessing a null shared pointer!");
        }
        return ptr_.get();
    }
    void unbind() {ptr_=nullptr;}
    operator std::shared_ptr<T>() const {return ptr_;}
    bool is_null() const {return !ptr_;}
    void reset(T* ptr = nullptr) {ptr_.reset(ptr);}
    std::shared_ptr<T> get() const {return ptr_;}
private:
    std::shared_ptr<T> ptr_;
};

template <typename T, typename... Args>
SafeSharedPtr<T> make_safe_shared(Args&&... args) {
    return SafeSharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

 template <typename Iterable>
class LockedIterable {
private:
    Iterable& m_iterable;
public:
    Iterable& get() {return m_iterable;}
    LockedIterable(Iterable& iterable) : m_iterable(iterable) {m_iterable->lock();}
    ~LockedIterable() {m_iterable->unlock();}
    LockedIterable(const LockedIterable&) = delete;
    LockedIterable& operator=(const LockedIterable&) = delete;
    LockedIterable(LockedIterable&& other) noexcept : m_iterable(other.m_iterable) {other.m_iterable = nullptr;}
    LockedIterable& operator=(LockedIterable&& other) noexcept {
        if (this != &other) {m_iterable->unlock();m_iterable = other.m_iterable;other.m_iterable = nullptr;}
        return *this;
    }
    auto begin() const { return m_iterable->begin(); }
    auto end() const { return m_iterable->end(); }
};

 template <typename T>
class SafeVector {
private:
    std::vector<T> data;
    std::atomic<int> itercount;

public:
    SafeVector() = default;
    SafeVector(int size) : data(size), itercount(0) {}
    SafeVector(std::initializer_list<T> init) : data(init), itercount(0) {}
    SafeVector(const SafeVector& other) = delete;
    SafeVector(SafeVector<T>&& other) : data(std::move(other.data)), itercount(0) {if(other.itercount) throw std::out_of_range("Cannot return a vector from within a loop.");}
    operator auto() const {return data.begin();} 
    auto lock() { ++itercount; }
    auto unlock() { --itercount; }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    size_t size() const { return data.size(); }
    SafeVector* operator->() {return this;} // optimized away by -O2 
    const SafeVector* operator->() const {return this;} // optimized away by -O2 
    T& operator[](size_t index) {
        if (index >= data.size()) throw std::out_of_range("Index "+std::to_string(index)+" casted from negative int or out of bounds in `vector` with "+std::to_string(data.size())+" elements");
        return data[index];
    }
    const T& operator[](size_t index) const {
        if (index >= data.size()) throw std::out_of_range("Index "+std::to_string(index)+" casted from negative int or out of bounds in `vector` with "+std::to_string(data.size())+" elements");
        return data[index];
    }
    void set(size_t index, const T& value) {
        if (index >= data.size()) throw std::out_of_range("Index "+std::to_string(index)+" casted from negative int or out of bounds in `vector` with "+std::to_string(data.size())+" elements");
        data[index] = value;
    }
    void pop() {
        if (data.empty()) throw std::out_of_range("Pop from empty SafeVector");
        if (itercount) throw std::out_of_range("Cannot pop from an iterating vector.");        data.pop_back();
    }
    void reserve(size_t size) { data.reserve(size); }
    void push(const T& value) { if (itercount) throw std::out_of_range("Cannot push to an iterating vector."); data.push_back(value); }
    void clear() { if (itercount) throw std::out_of_range("Cannot clear an iterating vector."); data.clear(); }
    bool empty() const { return data.empty(); }
};

 
namespace cimple_time{
   auto now=std::chrono::high_resolution_clock::now;
}

struct Number{
   Number* operator->() {return this;} // optimized away by -O2 
   const Number* operator->() const {return this;} // optimized away by -O2 
   Number(const Number& other) = default; 
   Number(Number&& other) = default; 
   double value;
   Number(double value){
      this -> value=value;
   }
}
;
int main(){
   print(cimple_time::now());
   auto number=make_safe_shared<Number>(10);
   print(number -> value);
   auto x=make_safe_shared<SafeVector<double>>(); x-> reserve(100);
   x-> push(0);
   x-> push(1);
   for(auto i:LockedIterable(x))print(i);
   return 0;
}
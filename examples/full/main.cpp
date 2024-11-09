#include <chrono>
 
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

 template <typename T>
class SafeVector {
private:
    std::vector<T> data;

public:
    SafeVector() = default;
    SafeVector(int size) : data(size) {}
    SafeVector(std::initializer_list<T> init) : data(init) {}
    operator auto() const {return data.begin();} 
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
        data.pop_back();
    }
    void reserve(size_t size) { data.reserve(size); }
    void push(const T& value) { data.push_back(value); }
    void clear() { data.clear(); }
    bool empty() const { return data.empty(); }
};

 
namespace cimple_time{
   auto now=std::chrono::high_resolution_clock::now;
}


namespace cimple_lib{
   struct Data{
      Data* operator->() {return this;} // optimized away by -O2 
      const Data* operator->() const {return this;} // optimized away by -O2 
      Data(const Data& other) = default; 
      Data(Data&& other) = default; 
      SafeVector<double>predictions;
      SafeVector<double>labels;
      Data(){
      }
   }
   ;
}

int main(){
   auto tic=cimple_time::now();
   auto data=cimple_lib::Data();
   data -> labels -> push(0);
   data -> labels -> push(1);
   data -> predictions -> push(0);
   data -> predictions -> push(1);
   auto acc=0.0;
   for(auto[i,j]:std::views::zip(data -> labels,data -> labels)){
      acc+=i+j+0.5;
   }
   print(acc);
   print(cimple_time::now()-tic);
   return 0;
}
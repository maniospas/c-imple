#include <iostream>
#include <memory>
#define print(message) std::cout<<(message)<<std::endl
 
#include <stdexcept>

template <typename T>
class SafeSharedPtr {
public:
    SafeSharedPtr() = default;
    explicit SafeSharedPtr(T* ptr) : ptr_(std::shared_ptr<T>(ptr)) {}
    SafeSharedPtr(const std::shared_ptr<T>& ptr) : ptr_(ptr) {}
    SafeSharedPtr(std::shared_ptr<T>&& ptr) : ptr_(std::move(ptr)) {}
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

 
struct LinkedNode{
   LinkedNode* operator->() {return this;} // optimized away by -O2 
   const LinkedNode* operator->() const {return this;} // optimized away by -O2 
   double value;
   SafeSharedPtr<LinkedNode>next;
   SafeSharedPtr<LinkedNode>prev;
   LinkedNode(auto value){
      this -> value=value;
   }
}
;
auto set_next(const SafeSharedPtr<LinkedNode>&from,const SafeSharedPtr<LinkedNode>&to){
   from -> next=to;
   to -> prev=from;
}
int main(){
   auto node1=make_safe_shared<LinkedNode>(1);
   auto node2=make_safe_shared<LinkedNode>(2);
   auto node3=make_safe_shared<LinkedNode>(3);
   set_next(node1,node2);
   set_next(node2,node3);
   try{
      print(node1 -> value);
      print(node1 -> next -> value);
      print(node1 -> next -> next -> value);
      print(node1 -> next -> next -> next -> value);
   }
   catch(std::runtime_error){
      print("runtime error");
   }
   return 0;
}
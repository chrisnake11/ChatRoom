#ifndef SINGLETON_H
#define SINGLETON_H

#include <memory>
#include <mutex>
#include <iostream>

template <typename T>
class Singleton
{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;
    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> getInstance() {
        static std::once_flag flag;
        std::call_once(flag, [&] {
            _instance = std::shared_ptr<T>(new T);
            });
        return _instance;
    }
    ~Singleton() {
        std::cout << "~Singleton() " << std::endl;
    }
}; 

// definition
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H

#pragma once
#include <mutex>
#include <iostream>
template <typename T>
class Singleton
{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton<T>& operator=(const Singleton<T>&) = delete;
    static std::shared_ptr<T> _instance;
private:
    static std::once_flag _flag;
public:
    static std::shared_ptr<T> GetInstance()
    {
        std::call_once(_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
            });
        return _instance;
    }
    void PrintAddress()
    {
        std::cout << "Singleton address: " << _instance.get() << std::endl;
    }
    ~Singleton()
    {
        std::cout << "~Singleton()" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

template <typename T>
std::once_flag Singleton<T>::_flag;


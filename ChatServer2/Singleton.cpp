#include "Singleton.h"

template<typename T>
std::shared_ptr<T> Singleton<T>::getInstance()
{
    std::call_once(_flag, [&]() {
        _instance = std::shared_ptr<T>(new T);
        });
    return _instance;
}
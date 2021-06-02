#pragma once

template <typename T>
class ISingleton
{
  private:
    static T& Singleton()
    {
        static T singleton;
        return &singleton;
    }
};
#pragma once

/// @brief Interface for singletons, classes with a single global instance accessible from anywhere.
/// @note Be careful using them. They are fine for now as a mean to achieve inversion of control, but they're not such good ideas (they encourage coupling, might cause multithreading problems, make testing difficult, etc. )
/// @note Consider using some sort of service provider pattern, or directly injecting dependencies
template <typename T>
class ISingleton
{
  protected:
    /// @brief Get the singleton instance.
    static T& Singleton()
    {
        static T singleton;
        return singleton;
    }
};
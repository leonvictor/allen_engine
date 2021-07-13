#pragma once

#include <utils/singleton.hpp>

#include <iostream>
#include <string>

enum Level
{
    Info,
    Warning,
    Error,
};

/// @brief Debug utility for the Engine.
/// @todo For now these utility simply print the logs to the console
class Debug : private ISingleton<Debug>
{
    /// @brief Log a message at the info level.
    static void Log(std::string text)
    {
        std::cout << text << std::endl;
    }

    /// @brief Log a warning
    static void LogWarning(std::string text)
    {
        std::cout << text << std::endl;
    }

    /// @brief Log an error
    static void LogError(std::string text)
    {
        std::cout << text << std::endl;
    }
};
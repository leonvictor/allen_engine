#pragma once

#include <GLFW/glfw3.h>
#include <map>

struct KeyState
{
    bool pressed = false;
    bool pressedLastFrame = false;
};

class InputMonitor
{
private:
    std::map<int, KeyState> states;

public:

    bool isDown(int key)
    {
        return states[key].pressed;
    }

    bool isUp(int key)
    {
        return !isDown(key);
    }

    // Resets the field to default if reset is true
    bool isPressedLastFrame(int key, bool reset = false)
    {
        bool result = states[key].pressedLastFrame;
        if (reset && result)
            states[key].pressedLastFrame = false;
        return result ;
    }

    // GLFW Keyboard event callback
    void callback(int key, int scancode, int action, int mods)
    {
        states[key].pressedLastFrame = (action == GLFW_RELEASE && states[key].pressed);
        states[key].pressed = (action == GLFW_PRESS);
    }

    // GLFW Mouse event callback
    void callback(int key, int action, int mods)
    {
        callback(key, 0, action, mods);
    }
};
#pragma once

#include "uuid.hpp"
#include "common/containers/vector.hpp"

#include <functional>

namespace aln
{
/// @brief Event emitting object. Functions can bind to it and be triggered when it's fired.
template <typename... Args>
class Event
{
    using ListenerFunction = std::function<void(Args...)>;

    struct Listener
    {
        UUID m_id = UUID::Generate();
        ListenerFunction m_function;

        Listener(ListenerFunction&& function) : m_function(function) {}
    };

  private:
    Vector<Listener> m_listeners;

  public:
    ~Event()
    {
        assert(m_listeners.empty());
    }

    const UUID& BindListener(ListenerFunction&& listenerFunction)
    {
        auto& listener = m_listeners.emplace_back(std::forward<ListenerFunction>(listenerFunction));
        return listener.m_id;
    }

    void UnbindListener(const UUID& listenerID)
    {
        auto it = std::find_if(m_listeners.begin(), m_listeners.end(), [&](Listener& listener)
            { return listener.m_id == listenerID; });
        assert(it != m_listeners.end());
        m_listeners.erase(it);
    }

    void Fire(Args... args) const
    {
        for (auto& listener : m_listeners)
        {
            listener.m_function(std::forward<Args>(args)...);
        }
    }
};
} // namespace aln
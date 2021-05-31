#pragma once
#include <vector>

class IObservable;
class IObserver
{
  public:
    virtual void Update(IObservable* pObserved) = 0;
};

class IObservable
{
  protected:
    std::vector<IObserver*> m_observers;

  public:
    virtual void Attach(IObserver* obs)
    {
        m_observers.push_back(obs);
    }

    virtual void Notify()
    {
        for (auto obs : m_observers)
        {
            obs->Update(this);
        }
    }
};

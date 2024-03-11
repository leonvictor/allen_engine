#pragma once

namespace aln
{

class ServiceProvider;

/// @brief Base class for services
class IService
{
    friend class ServiceProvider;

  private:
    ServiceProvider* m_pServiceProvider = nullptr; // ServiceProvider we're registered with

    /// @todo : Use a macro to generate unique IDs, kinda like for assets

  protected:
    virtual void Initialize(ServiceProvider* pProvider)
    {
        m_pServiceProvider = pProvider;
    }

    virtual void Shutdown()
    {
        m_pServiceProvider = nullptr;
    }

    ServiceProvider* GetProvider() { return m_pServiceProvider; }

  public:
    bool IsRegisteredWithProvider() const { return m_pServiceProvider != nullptr; }
};
} // namespace aln

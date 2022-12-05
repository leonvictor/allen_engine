#pragma once

#include "types_editor.hpp"

namespace aln
{
/// @brief Interface for editor windows
class IEditorWindow
{
  private:
    const TypeEditorService* m_pTypeEditorService;

  protected:
    const TypeEditorService* GetTypeEditorService() const { return m_pTypeEditorService; }

  public:
    virtual void Draw() = 0; // TODO: name ?

    virtual void Initialize(const TypeEditorService* pTypeEditorService)
    {
        m_pTypeEditorService = pTypeEditorService;
    }

    virtual void Shutdown()
    {
        m_pTypeEditorService = nullptr;
    }
};
} // namespace aln
#pragma once

#include "primitive_type_editors.hpp"

#include <reflection/type_info.hpp>
#include <unordered_map>

namespace aln
{
class ReflectedTypeEditor
{
  private:
    Event<ReflectedTypeEditor*> m_typeEditionStartedEvent;
    Event<ReflectedTypeEditor*> m_typeEditionCompletedEvent;

    static std::unordered_map<StringID, IPrimitiveTypeEditor*> PrimitiveTypeEditors;

    static IPrimitiveTypeEditor* GetOrCreatePrimitiveTypeEditor(const StringID& typeID);

  public:
    static void Initialize() {}
    static void Shutdown();

    Event<ReflectedTypeEditor*>& OnTypeEditingStarted() { return m_typeEditionStartedEvent;}
    Event<ReflectedTypeEditor*>& OnTypeEditingCompleted() { return m_typeEditionCompletedEvent;}

    /// @brief Parse the reflected fields of a custom reflected class to display them in editor
    /// @param widgetColumnWidth Desired widget column width. Default behavior is to use two third of the available window width.
    void Draw(const reflect::TypeInfo* pTypeInfo, void* pTypeInstance, float widgetColumnWidth = -FLT_MIN);
};
}; // namespace aln
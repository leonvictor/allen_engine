#pragma once

#include "primitive_type_editors.hpp"

#include <common/containers/hash_map.hpp>
#include <common/event.hpp>
#include <reflection/reflected_type.hpp>

namespace aln
{

namespace reflect
{
class ClassMemberInfo;
}

struct TypeEditedEventDetails
{
    enum class Action : uint8_t
    {
        Invalid,
        Edit,
        EditRequiresReload,
    };

    Action m_action = Action::Invalid;
    reflect::IReflected* m_pEditedTypeInstance;
    const reflect::ClassMemberInfo* m_pEditedMemberInfo;
};

class ReflectedTypeEditor
{
  private:
    Event<const TypeEditedEventDetails&> m_typeEditionStartedEvent;
    Event<const TypeEditedEventDetails&> m_typeEditionCompletedEvent;

    static HashMap<StringID, IPrimitiveTypeEditor*> PrimitiveTypeEditors;

    static IPrimitiveTypeEditor* GetOrCreatePrimitiveTypeEditor(const StringID& typeID);

  public:
    ~ReflectedTypeEditor();

    static void Initialize() {}
    static void Shutdown();

    Event<const TypeEditedEventDetails&>& OnTypeEditingStarted() { return m_typeEditionStartedEvent; }
    Event<const TypeEditedEventDetails&>& OnTypeEditingCompleted() { return m_typeEditionCompletedEvent; }

    /// @brief Parse the reflected fields of a custom reflected class to display them in editor
    /// @param widgetColumnWidth Desired widget column width. Default behavior is to use two third of the available window width.
    void Draw(const reflect::TypeInfo* pTypeInfo, void* pTypeInstance, float widgetColumnWidth = -FLT_MIN) const;
};
}; // namespace aln
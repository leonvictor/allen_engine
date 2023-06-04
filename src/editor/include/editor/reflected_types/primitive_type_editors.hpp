#pragma once

#include <cstddef>

namespace aln
{

class StringID;

/// @brief Base class for primitive type editor widgets.
/// Uses two-step editing: first draw the widget, then update the values
class IPrimitiveTypeEditor
{
    friend class ReflectedTypeEditor;

    /// @brief Factory method creating editor fields for primitive types
    static IPrimitiveTypeEditor* CreateEditor(const StringID& typeID);

    virtual bool DrawWidget(std::byte* pTypeInstance) = 0;
    virtual void UpdateValue() = 0;
};

} // namespace aln
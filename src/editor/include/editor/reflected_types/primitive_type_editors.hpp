#pragma once

#include <cstddef>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <assets/asset.hpp>
#include <assets/handle.hpp>

namespace aln
{

/// @brief Base class for primitive type editor widgets.
/// Use two-step editing: first draw the widget, then update the values
class IPrimitiveTypeEditor
{
    friend class ReflectedTypeEditor;

    /// @brief Factory method creating editor fields for primitive types
    static IPrimitiveTypeEditor* CreateEditor(const StringID& typeID);

    virtual bool DrawWidget(std::byte* pTypeInstance) = 0;
    virtual void UpdateValue() = 0;
};

} // namespace aln
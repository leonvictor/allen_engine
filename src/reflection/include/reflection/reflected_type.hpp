#pragma once

#include <common/string_id.hpp>

namespace aln::reflect
{
    class TypeInfo;

    /// @brief Interface for user-defined reflected class types
    class IReflected
    {
      public:
        virtual const aln::reflect::TypeInfo* GetTypeInfo() const = 0;
    };
}
#pragma once

#include "../type_info.hpp"

#include <common/services/service.hpp>
#include <common/string_id.hpp>
#include <common/containers/vector.hpp>

#include <map>
#include <string>

namespace aln
{

class TypeRegistryService : public IService
{
    friend class reflect::TypeInfo;

    std::map<StringID, const reflect::TypeInfo*> m_typeInfos;
    std::map<std::string, Vector<const reflect::TypeInfo*>> m_scopes;

  public:
    /// @brief Update internal maps with types registered in the current DLL.
    // Called once per module initialization
    /// @todo: Rename. Initialize ? Something about modules ?
    void PollRegisteredTypes()
    {
        for (auto& [scope, typeInfos] : reflect::TypeInfo::Scopes)
        {
            auto it = m_scopes.try_emplace(scope);
            it.first->second.insert(it.first->second.end(), typeInfos.begin(), typeInfos.end());
        }

        m_typeInfos.insert(reflect::TypeInfo::LookUpMap.begin(), reflect::TypeInfo::LookUpMap.end());
    }

    const reflect::TypeInfo* GetTypeInfo(const StringID& typeID) const
    {
        auto it = m_typeInfos.find(typeID);
        assert(it != m_typeInfos.end());
        return it->second;
    }

    const Vector<const reflect::TypeInfo*>& GetTypesInScope(const char* scopeName) const
    {
        auto it = m_scopes.find(scopeName);
        assert(it != m_scopes.end());
        return it->second;
    }

    const std::map<StringID, const reflect::TypeInfo*>& GetAllRegisteredTypes() const
    {
        return m_typeInfos;
    }
};
} // namespace aln
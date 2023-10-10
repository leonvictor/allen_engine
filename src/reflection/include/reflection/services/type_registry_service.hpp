#pragma once

#include "../type_info.hpp"

#include <common/containers/hash_map.hpp>
#include <common/containers/vector.hpp>
#include <common/services/service.hpp>
#include <common/string_id.hpp>
#include <string>

namespace aln
{

class TypeRegistryService : public IService
{
    friend class reflect::TypeInfo;

  private:
    HashMap<StringID, reflect::TypeInfo*> m_typeInfos;
    HashMap<std::string, Vector<reflect::TypeInfo*>, std::hash<std::string>> m_scopes;

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

    const Vector<reflect::TypeInfo*>& GetTypesInScope(const char* scopeName) const
    {
        auto it = m_scopes.find(scopeName);
        assert(it != m_scopes.end());
        return it->second;
    }

    const HashMap<StringID, reflect::TypeInfo*>& GetAllRegisteredTypes() const
    {
        return m_typeInfos;
    }
};
} // namespace aln
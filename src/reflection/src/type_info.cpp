#include "type_info.hpp"

namespace aln::reflect
{
void TypeInfo::RegisterTypeInfo(const TypeInfo* pTypeInfo, const std::string& scopeName)
{
    assert(pTypeInfo != nullptr);
    TypeInfo::LookUpMap.emplace(pTypeInfo->m_typeID, pTypeInfo);

    if (!scopeName.empty())
    {
        const auto& it = TypeInfo::Scopes.try_emplace(scopeName);
        it.first->second.push_back(pTypeInfo);
    }
}

TypeInfo::TypeInfo(void (*initFunction)(TypeInfo*), const char* scopeName)
{
    initFunction(this);
    RegisterTypeInfo(this, scopeName);
}
} // namespace aln::reflect
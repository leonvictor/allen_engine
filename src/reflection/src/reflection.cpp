#include "reflection.hpp"
#include <map>

namespace aln::reflect
{

// Type registry. When they are instanciated, type descriptors register themselves in a global map, which
// allows us to look them up and use them to instanciate their described types, during deserialization for example
/// @todo : Using a global is bad, find something else !
/// @note We might be able to register them in a dll-local map, then gather all the maps
// during the engine's initialization, in a "module" fashion
const TypeDescriptor* GetType(const std::type_index& typeIndex)
{
    return RegisteredTypes()[typeIndex];
}

void RegisterType(std::type_index typeIndex, TypeDescriptor* pTypeDescriptor)
{

    auto& registry = RegisteredTypes();
    registry.try_emplace(typeIndex, pTypeDescriptor);
}

std::map<std::type_index, TypeDescriptor*>& RegisteredTypes()
{
    static std::map<std::type_index, TypeDescriptor*> types;
    return types;
}

// -------------
// Type Descriptors
// -------------
TypeDescriptor::TypeDescriptor(const char* name, size_t size, std::type_index typeIndex)
    : name{name}, size{size}, m_typeIndex(typeIndex)
{
    RegisterType(typeIndex, this);
}

bool operator==(const TypeDescriptor& a, const TypeDescriptor& b)
{
    return a.m_typeIndex == b.m_typeIndex;
}

bool operator!=(const TypeDescriptor& a, const TypeDescriptor& b)
{
    return !(a == b);
}

// ---------------
// TypeDescriptor_Struct
// ---------------
void TypeDescriptor_Struct::Dump(const void* obj, int indentLevel) const
{
    std::cout << GetFullName() << " {" << std::endl;
    for (const Member& member : members)
    {
        std::cout << std::string(4 * (indentLevel + 1), ' ') << member.name << " = ";
        member.type->Dump((char*) obj + member.offset, indentLevel + 1);
        std::cout << std::endl;
    }
    std::cout << std::string(4 * indentLevel, ' ') << "}";
}

std::vector<TypeDescriptor*>& GetTypesInScope(const std::string& scopeName)
{
    static std::map<std::string, std::vector<TypeDescriptor*>> typeScopes;
    auto it = typeScopes.try_emplace(scopeName);
    return it.first->second;
}

std::string TypeDescriptor_Struct::Member::GetPrettyName() const
{
    std::string prettyName = std::string(name);

    // Remove member variable prefix m_ if necessary
    auto prefix = prettyName.substr(0, prettyName.find("_"));
    if (prefix == "m")
    {
        prettyName = prettyName.substr(prettyName.find("_") + 1);
    }

    // Add spaces before each upper case letter
    for (int i = 0; i < prettyName.length(); i++)
    {
        if (std::isupper(prettyName[i]))
        {
            prettyName.insert(i++, " ");
        }
    }

    // First letter is uppercase in any way
    prettyName[0] = std::toupper(prettyName[0]);

    return prettyName;
}
} // namespace aln::reflect
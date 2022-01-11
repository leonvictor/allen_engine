#include "reflection.hpp"
#include <map>

namespace aln::reflect
{
bool operator==(const TypeDescriptor& a, const TypeDescriptor& b)
{
    return a.m_ID == b.m_ID;
}

bool operator!=(const TypeDescriptor& a, const TypeDescriptor& b)
{
    return !(a == b);
}

void TypeDescriptor_Struct::Dump(const void* obj, int indentLevel) const
{
    std::cout << name << " {" << std::endl;
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
#pragma once

#include <common/memory.hpp>
#include <common/serialization/binary_archive.hpp>
#include <common/string_id.hpp>

#include <assert.h>
#include <concepts>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace aln
{
class TypeRegistryService;
}
namespace aln::reflect
{

/// @brief Generate a reading-friendly name from an original type/field name
/// @todo Hide from clients
static std::string PrettifyName(const char* originalName)
{
    std::string prettyName = originalName;

    // Remove namespace
    prettyName = prettyName.substr(prettyName.rfind(":") + 1);

    // Remove member variable prefix m_ if necessary
    auto prefix = prettyName.substr(0, prettyName.find("_"));
    if (prefix == "m")
    {
        prettyName = prettyName.substr(prettyName.find("_") + 1);
    }

    // Remove leading "p" prefix for pointers
    if (prettyName[0] == 'p' && std::isupper(prettyName[1]))
    {
        prettyName = prettyName.substr(1);
    }

    // Add spaces before each upper case letter
    for (int i = 1; i < prettyName.length(); i++)
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
/// @brief A type that has been manually registered for reflection
template <typename T>
concept Registered = requires(T a) { T::Reflection; };

// -----------------------
// Finding Types Info
// -----------------------
class TypeInfo;

/// @brief Customization point for finding TypeInfos.
/// @todo Document user-provided reflection
template <typename T>
struct TypeInfoResolver
{
    TypeInfoResolver() = delete;
    static const TypeInfo* Get()
    {
        assert(false);
        return nullptr;
    }
};

template <Registered T>
struct TypeInfoResolver<T>
{
    static const TypeInfo* Get() { return &T::GetStaticTypeInfo(); }
};

// -----------------------
// Type Info Classes
// -----------------------

class ClassMemberInfo
{
  private:
    StringID m_typeID;
    std::string m_name;
    size_t m_offset;
    size_t m_size;

    // TODO: Editor only
    std::string m_prettyName;

  public:
    ClassMemberInfo(const StringID& typeID, const char* name, size_t offset, size_t size)
        : m_typeID(typeID), m_name(name), m_offset(offset), m_size(size), m_prettyName(PrettifyName(name)) {}

    const StringID& GetTypeID() const { return m_typeID; }
    const std::string& GetName() const { return m_name; }
    const std::string& GetPrettyName() const { return m_prettyName; }
    size_t GetOffset() const { return m_offset; }
    size_t GetSize() const { return m_size; }
};

class TypeInfo
{
    template <typename T>
    friend class TypeInfoResolver;
    friend class aln::TypeRegistryService;

    // TODO: Members are public since they are initialized in custom reflected types bodies
    // Find a good way to avoid that
  public:
    StringID m_typeID = StringID::InvalidID();
    std::string m_name;
    size_t m_size = 0;
    size_t m_alignment = 0;
    std::vector<ClassMemberInfo> m_members;
    const TypeInfo* m_pBaseTypeInfo = nullptr;

    // Bound lifetime functions
    std::function<void*()> m_createType;
    std::function<void*(void*)> m_createTypeInPlace;

    std::function<void(BinaryMemoryArchive&, void*)> m_serialize;

    // TODO: Editor only
    std::string m_prettyName;

  protected:
    // Registry
    inline static std::map<StringID, const TypeInfo*> LookUpMap;
    inline static std::map<std::string, std::vector<const TypeInfo*>> Scopes;

    /// @brief Register a type to the dll-local maps. Polled from each dlls during module initialization
    static void RegisterTypeInfo(const TypeInfo* pTypeInfo, const std::string& scopeName = "")
    {
        assert(pTypeInfo != nullptr);
        TypeInfo::LookUpMap.emplace(pTypeInfo->m_typeID, pTypeInfo);

        if (!scopeName.empty())
        {
            const auto& it = TypeInfo::Scopes.try_emplace(scopeName);
            it.first->second.push_back(pTypeInfo);
        }
    }

  public:
    TypeInfo() = default;
    TypeInfo(void (*initFunction)(TypeInfo*), const char* scopeName)
    {
        initFunction(this);
        RegisterTypeInfo(this, scopeName);
    }

    bool IsValid() const { return m_typeID.IsValid(); }
    bool operator==(const TypeInfo& other) const { return m_typeID == other.m_typeID; }

    const StringID& GetTypeID() const { return m_typeID; }
    const std::string& GetName() const { return m_name; }
    const std::string& GetPrettyName() const { return m_prettyName; }
    size_t GetSize() const { return m_size; }
    size_t GetAlignment() const { return m_alignment; }
    const std::vector<ClassMemberInfo>& GetMembers() const { return m_members; }
    size_t GetMemberCount() const { return m_members.size(); }
    const ClassMemberInfo* GetMemberInfo(size_t memberIdx) const { return &m_members[memberIdx]; }
    virtual bool IsPrimitive() const { return false; }

    /// @brief Create an instance of the described type
    /// @tparam T: Type to cast the instanciated object to
    /// @return A pointer to the instanciated object
    template <typename T>
    T* CreateTypeInstance() const { return (T*) m_createType(); }

    /// @brief Placement-new an instance of the described type in pre-allocated memory
    /// @param pMemory: Pointer to the allocated memory block
    /// @tparam T: Type to cast the instanciated object to
    /// @return A pointer to the instanciated object
    template <typename T>
    T* CreateTypeInstanceInPlace(void* pMemory) const { return (T*) m_createTypeInPlace(pMemory); }
};

class PrimitiveTypeInfo : public TypeInfo
{
    template <typename T>
    friend struct TypeInfoResolver;

    // TODO: These should be generic
    std::function<void(BinaryMemoryArchive&, const void*)> m_serialize;
    std::function<void(BinaryMemoryArchive&, void*)> m_deserialize;

  public:
    void Serialize(BinaryMemoryArchive& archive, const void* pTypeInstance) const
    {
        assert(archive.IsWriting());
        assert(m_serialize);
        m_serialize(archive, pTypeInstance);
    }

    void Deserialize(BinaryMemoryArchive& archive, void* pTypeInstance) const
    {
        assert(archive.IsReading());
        assert(m_deserialize);
        m_deserialize(archive, pTypeInstance);
    }

    virtual bool IsPrimitive() const override { return true; }
};

// -----------------------
// Types Registration Macros
// -----------------------

/// @brief Register the current type for class reflection.
#define ALN_REGISTER_TYPE()                                             \
  public:                                                               \
    static const aln::reflect::TypeInfo* GetStaticTypeInfo();           \
    virtual const aln::reflect::TypeInfo* GetTypeInfo() const override; \
                                                                        \
  private:                                                              \
    static aln::reflect::TypeInfo Reflection;                           \
    static void InitReflection(aln::reflect::TypeInfo*);

#define ALN_REGISTER_IMPL_BEGIN(scope, type)                               \
    aln::reflect::TypeInfo type::Reflection(type::InitReflection, #scope); \
                                                                           \
    const aln::reflect::TypeInfo* type::GetTypeInfo() const                \
    {                                                                      \
        return &Reflection;                                                \
    }                                                                      \
                                                                           \
    const aln::reflect::TypeInfo* type::GetStaticTypeInfo()                \
    {                                                                      \
        return &Reflection;                                                \
    }                                                                      \
                                                                           \
    void type::InitReflection(aln::reflect::TypeInfo* pTypeInfo)           \
    {                                                                      \
        using T = type;                                                    \
        pTypeInfo->m_typeID = aln::StringID(#type);                        \
        pTypeInfo->m_name = #type;                                         \
        pTypeInfo->m_prettyName = aln::reflect::PrettifyName(#type);       \
        pTypeInfo->m_size = sizeof(T);                                     \
        pTypeInfo->m_alignment = alignof(T);                               \
        pTypeInfo->m_createType = []() { return aln::New<T>(); };          \
        pTypeInfo->m_createTypeInPlace = [](void* pMemory) { return aln::PlacementNew<T>(pMemory); };

#define ALN_REGISTER_ABSTRACT_IMPL_BEGIN(type)                                 \
    aln::reflect::TypeInfo type::Reflection(type::InitReflection, "ABSTRACT"); \
                                                                               \
    const aln::reflect::TypeInfo* type::GetTypeInfo() const                    \
    {                                                                          \
        return &Reflection;                                                    \
    }                                                                          \
                                                                               \
    const aln::reflect::TypeInfo* type::GetStaticTypeInfo()                    \
    {                                                                          \
        return &Reflection;                                                    \
    }                                                                          \
                                                                               \
    void type::InitReflection(aln::reflect::TypeInfo* pTypeInfo)               \
    {                                                                          \
        using T = type;                                                        \
        pTypeInfo->m_typeID = aln::StringID(#type);                            \
        pTypeInfo->m_name = #type;                                             \
        pTypeInfo->m_prettyName = aln::reflect::PrettifyName(#type);           \
        pTypeInfo->m_size = sizeof(T);                                         \
        pTypeInfo->m_alignment = alignof(T);

#define ALN_REFLECT_BASE(baseClass) \
    pTypeInfo->m_pBaseTypeInfo = baseClass::GetStaticTypeInfo();

#define ALN_REFLECT_MEMBER(name, displayName) \
    pTypeInfo->m_members.emplace_back(aln::reflect::TypeInfoResolver<decltype(T::name)>::Get()->m_typeID, #name, offsetof(T, name), sizeof(decltype(T::name)));

#define ALN_REGISTER_IMPL_END() \
    }

#define ALN_REGISTER_PRIMITIVE(primitiveType)                                                                                                 \
    namespace reflect                                                                                                                         \
    {                                                                                                                                         \
    template <>                                                                                                                               \
    struct TypeInfoResolver<primitiveType>                                                                                                    \
    {                                                                                                                                         \
        static const TypeInfo* Get()                                                                                                          \
        {                                                                                                                                     \
            static PrimitiveTypeInfo typeInfo;                                                                                                \
            if (!typeInfo.IsValid())                                                                                                          \
            {                                                                                                                                 \
                typeInfo.m_typeID = StringID(#primitiveType);                                                                                 \
                typeInfo.m_name = #primitiveType;                                                                                             \
                typeInfo.m_prettyName = PrettifyName(#primitiveType);                                                                         \
                typeInfo.m_alignment = alignof(primitiveType);                                                                                \
                typeInfo.m_size = sizeof(primitiveType);                                                                                      \
                typeInfo.m_createType = []() { return aln::New<primitiveType>(); };                                                           \
                typeInfo.m_createTypeInPlace = [](void* pMemory) { return aln::PlacementNew<primitiveType>(pMemory); };                       \
                typeInfo.m_serialize = [](BinaryMemoryArchive& archive, const void* pInstance) { archive << *((primitiveType*) pInstance); }; \
                typeInfo.m_deserialize = [](BinaryMemoryArchive& archive, void* pInstance) { archive >> *((primitiveType*) pInstance); };     \
                TypeInfo::RegisterTypeInfo(&typeInfo);                                                                                        \
            }                                                                                                                                 \
            return &typeInfo;                                                                                                                 \
        }                                                                                                                                     \
    };                                                                                                                                        \
    }

} // namespace aln::reflect

// Register primitives
// TODO: Move colors/glm out
#include <common/colors.hpp>
#include <common/transform.hpp>
#include <glm/vec3.hpp>

namespace aln
{
ALN_REGISTER_PRIMITIVE(RGBAColor)
ALN_REGISTER_PRIMITIVE(RGBColor)
ALN_REGISTER_PRIMITIVE(glm::vec3)
ALN_REGISTER_PRIMITIVE(int)
ALN_REGISTER_PRIMITIVE(float)
ALN_REGISTER_PRIMITIVE(bool)
ALN_REGISTER_PRIMITIVE(uint32_t)
ALN_REGISTER_PRIMITIVE(uint8_t)
ALN_REGISTER_PRIMITIVE(uint16_t)
ALN_REGISTER_PRIMITIVE(Transform)
ALN_REGISTER_PRIMITIVE(std::string)
} // namespace aln
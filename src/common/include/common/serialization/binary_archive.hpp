#pragma once

#include "../memory.hpp"
#include "../containers/vector.hpp"

#include <assert.h>
#include <concepts>
#include <filesystem>
#include <fstream>


namespace aln
{

// Forward declarations
class IBinaryArchive;
class BinaryMemoryArchive;
class BinaryFileArchive;

template <typename T>
concept TriviallyCopyableType = std::is_trivially_copyable_v<T>;

template <typename T>
concept ContiguousContainer = requires(T a) {
                                  std::contiguous_iterator<typename T::iterator>;
                              };

/// @brief Access point for types whose serialization methods need to be private. Set as friend in class definitions
/// to allow the serialization system to access them.
/// @todo This is not functionnal ! I haven't found a good way to restrict access AND keep the constrained concept specialization
/// @note Not functionnal ! Serialization function need to be public for now
struct ArchiveAccess
{
    template <typename T, typename Archive>
    static void Serialize(const T& a, Archive& archive)
    {
        a.Serialize(archive);
    }

    template <typename T, typename Archive>
    static void Deserialize(T& a, Archive& archive)
    {
        a.Deserialize(archive);
    }
};

template <typename T>
concept CustomSerializable = requires(T a, BinaryMemoryArchive archive) {
                                 a.Serialize(archive);
                                 a.Deserialize(archive);
                             };

template <typename T>
concept Serializable = TriviallyCopyableType<T> || CustomSerializable<T>;

/// @brief Base class for binary archives
/// @note Supports IO operations for trivially copyable types and contiguous containers of trivially copyable types.
/// "Recursive" containers are also supported
/// @todo Handle state flags such as EoF or fail to signal user when something goes wrong
class IBinaryArchive
{
  public:
    enum class IOMode : uint8_t
    {
        Read,
        Write
    };

  private:
    IOMode m_mode;

  public:
    IBinaryArchive(IOMode mode) : m_mode(mode) {}

    bool IsReading() const { return m_mode == IOMode::Read; }
    bool IsWriting() const { return m_mode == IOMode::Write; }
    virtual bool IsValid() const = 0;
};

/// @brief A binary file archive to write to or read from
class BinaryFileArchive : public IBinaryArchive
{
  private:
    void* m_pFileStream = nullptr;
    const std::filesystem::path& m_path;

  public:
    BinaryFileArchive(BinaryFileArchive&) = delete;
    BinaryFileArchive(const std::filesystem::path& path, IOMode mode) : IBinaryArchive(mode), m_path(path)
    {
        if (IsReading())
        {
            auto pFileStream = aln::New<std::ifstream>();
            pFileStream->open(path.string(), std::ios::binary | std::ios::in);
            m_pFileStream = pFileStream;
        }
        else
        {
            auto pFileStream = aln::New<std::ofstream>();
            pFileStream->open(path.string(), std::ios::binary | std::ios::out);
            m_pFileStream = pFileStream;
        }
    }

    ~BinaryFileArchive()
    {
        assert(m_pFileStream != nullptr);

        if (IsReading())
        {
            auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
            aln::Delete(pFileStream);
        }
        else
        {
            auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
            aln::Delete(pFileStream);
        }
    }

    void Close()
    {
        assert(m_pFileStream != nullptr);
        if (IsReading())
        {
            auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
            pFileStream->close();
        }
        else
        {
            auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
            pFileStream->close();
        }
    }

    bool IsValid() const override
    {
        if (IsReading())
        {
            auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
            return pFileStream->is_open();
        }
        else
        {
            auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
            return pFileStream->is_open();
        }
    }

    void Write(const void* pData, size_t size)
    {
        assert(IsWriting());
        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
        pFileStream->write(reinterpret_cast<const char*>(pData), size);
    }

    void Read(void* pData, size_t size)
    {
        assert(IsReading());
        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
        pFileStream->read(reinterpret_cast<char*>(pData), size);
    }

    // --------------------------
    //  Trivially copyable types
    // --------------------------
    template <TriviallyCopyableType T>
    BinaryFileArchive& operator<<(const T& data)
    {
        assert(IsWriting());

        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
        pFileStream->write(reinterpret_cast<const char*>(&data), sizeof(T));

        return *this;
    }

    template <TriviallyCopyableType T>
    BinaryFileArchive& operator>>(T& data)
    {
        assert(IsReading());

        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
        pFileStream->read(reinterpret_cast<char*>(&data), sizeof(T));

        return *this;
    }

    // --------------------------
    //  Containers
    // --------------------------
    template <ContiguousContainer T>
    BinaryFileArchive& operator<<(const T& container)
    {
        assert(IsWriting());
        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);

        auto size = container.size();
        pFileStream->write(reinterpret_cast<const char*>(&size), sizeof(T::size_type));
        for (const auto& item : container)
        {
            *this << item;
        }

        return *this;
    }

    template <ContiguousContainer T>
    BinaryFileArchive& operator>>(T& container)
    {
        assert(IsReading());
        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);

        typename T::size_type size;
        pFileStream->read(reinterpret_cast<char*>(&size), sizeof(T::size_type));

        container.resize(size);
        for (auto i = 0; i < size; ++i)
        {
            *this >> container[i];
        }

        return *this;
    }

    template <typename T>
        requires ContiguousContainer<T> && TriviallyCopyableType<typename T::value_type>
    BinaryFileArchive& operator<<(const T& container)
    {
        assert(IsWriting());
        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);

        auto size = container.size();
        pFileStream->write(reinterpret_cast<const char*>(&size), sizeof(T::size_type));
        pFileStream->write(reinterpret_cast<const char*>(container.data()), size * sizeof(T::value_type));

        return *this;
    }

    template <typename T>
        requires ContiguousContainer<T> && TriviallyCopyableType<typename T::value_type>
    BinaryFileArchive& operator>>(T& container)
    {
        assert(IsReading());
        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);

        typename T::size_type size;
        pFileStream->read(reinterpret_cast<char*>(&size), sizeof(T::size_type));

        container.resize(size);
        pFileStream->read(reinterpret_cast<char*>(container.data()), size * sizeof(T::value_type));

        return *this;
    }

    // --------------------------
    //  Custom serializable types
    // --------------------------
    template <CustomSerializable T>
    BinaryFileArchive& operator<<(const T& object)
    {

        assert(IsWriting());
        ArchiveAccess::Serialize(object, *this);
        return *this;
    }

    template <CustomSerializable T>
    BinaryFileArchive& operator>>(T& object)
    {
        assert(IsReading());
        ArchiveAccess::Deserialize(object, *this);
        return *this;
    }

    // --------------------------
    //  Binary archives
    // --------------------------
    friend BinaryFileArchive& operator>>(BinaryFileArchive& in, BinaryMemoryArchive& out);
    friend BinaryFileArchive& operator<<(BinaryFileArchive& out, BinaryMemoryArchive& in);
    friend BinaryMemoryArchive& operator>>(BinaryMemoryArchive& in, BinaryFileArchive& out);
    friend BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, BinaryFileArchive& in);
};

/// @brief Archive view of an existing binary memory array
class BinaryMemoryArchive : public IBinaryArchive
{
  private:
    Vector<std::byte>& m_memory;
    Vector<std::byte>::iterator m_pReader;

  public:
    BinaryMemoryArchive(BinaryMemoryArchive&) = delete;
    // TODO: Use span instead
    BinaryMemoryArchive(Vector<std::byte>& memory, IOMode mode) : IBinaryArchive(mode), m_memory(memory)
    {
        if (IsReading())
        {
            m_pReader = m_memory.begin();
        }
    }

    bool IsValid() const override { return m_pReader != m_memory.begin(); }

    void Write(const void* pData, size_t size)
    {
        assert(IsWriting());
        auto pBytes = reinterpret_cast<const std::byte*>(pData);
        m_memory.insert(m_memory.end(), pBytes, pBytes + size);
    }

    void Read(void* pData, size_t size)
    {
        assert(IsReading());
        memcpy(pData, &*m_pReader, size);
        m_pReader += size;
    }

    // --------------------------
    //  Trivially copyable types
    // --------------------------
    template <TriviallyCopyableType T>
    BinaryMemoryArchive& operator<<(const T& data)
    {
        assert(IsWriting());

        auto pData = reinterpret_cast<const std::byte*>(&data);
        m_memory.insert(m_memory.end(), pData, pData + sizeof(T));

        return *this;
    }

    template <TriviallyCopyableType T>
    BinaryMemoryArchive& operator>>(T& data)
    {
        assert(IsReading());

        memcpy(&data, &*m_pReader, sizeof(T));
        m_pReader += sizeof(T);

        return *this;
    }

    // --------------------------
    //  Containers
    // --------------------------
    template <ContiguousContainer T>
    BinaryMemoryArchive& operator<<(const T& container)
    {
        assert(IsWriting());

        auto containerSize = container.size();
        *this << containerSize;

        for (const auto& item : container)
        {
            *this << item;
        }

        return *this;
    }

    template <ContiguousContainer T>
    BinaryMemoryArchive& operator>>(T& container)
    {
        assert(IsReading());

        typename T::size_type containerSize;
        *this >> containerSize;

        container.resize(containerSize);
        for (auto i = 0; i < containerSize; ++i)
        {
            *this >> container[i];
        }

        return *this;
    }

    template <typename T>
        requires ContiguousContainer<T> && TriviallyCopyableType<typename T::value_type>
    BinaryMemoryArchive& operator<<(const T& container)
    {
        assert(IsWriting());

        auto containerSize = container.size();
        *this << containerSize;

        auto pData = reinterpret_cast<const std::byte*>(container.data());
        m_memory.insert(m_memory.end(), pData, pData + (containerSize * sizeof(T::value_type)));

        return *this;
    }

    template <typename T>
        requires ContiguousContainer<T> && TriviallyCopyableType<typename T::value_type>
    BinaryMemoryArchive& operator>>(T& container)
    {
        assert(IsReading());

        typename T::size_type containerSize;
        memcpy(&containerSize, &*m_pReader, sizeof(T::size_type));
        m_pReader += sizeof(T::size_type);

        auto pDataTypePtr = reinterpret_cast<T::value_type*>(&*m_pReader);
        container.assign(pDataTypePtr, pDataTypePtr + containerSize);

        m_pReader += (containerSize * sizeof(T::value_type));
        return *this;
    }

    // --------------------------
    //  Custom serializable types
    // --------------------------
    template <CustomSerializable T>
    BinaryMemoryArchive& operator<<(const T& object)
    {

        assert(IsWriting());
        ArchiveAccess::Serialize(object, *this);
        return *this;
    }

    template <CustomSerializable T>
    BinaryMemoryArchive& operator>>(T& object)
    {
        assert(IsReading());
        ArchiveAccess::Deserialize(object, *this);
        return *this;
    }

    // --------------------------
    //  Binary archives
    // --------------------------
    friend BinaryFileArchive& operator>>(BinaryFileArchive& in, BinaryMemoryArchive& out);
    friend BinaryFileArchive& operator<<(BinaryFileArchive& out, BinaryMemoryArchive& in);
    friend BinaryMemoryArchive& operator>>(BinaryMemoryArchive& in, BinaryFileArchive& out);
    friend BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, BinaryFileArchive& in);
};

// --------------------
// Interop
// --------------------

/// @brief Save the full file in memory
BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, BinaryFileArchive& in);
BinaryMemoryArchive& operator>>(BinaryMemoryArchive& in, BinaryFileArchive& out);
BinaryFileArchive& operator<<(BinaryFileArchive& out, BinaryMemoryArchive& in);
BinaryFileArchive& operator>>(BinaryFileArchive& in, BinaryMemoryArchive& out);

} // namespace aln
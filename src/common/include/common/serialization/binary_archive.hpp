#pragma once

#include <lz4.h>

#include <assert.h>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <vector>

#include "../memory.hpp"

namespace aln
{

// Forward declarations
class IBinaryArchive;
class BinaryMemoryArchive;
class BinaryFileArchive;

/// @brief Contigious containers containing trivial types, i.e. std::vector<float> and std::string
template <typename T>
concept TrivialTypeContiguousContainer = requires(T a)
{
    T::size_type;
    std::contiguous_iterator<typename T::iterator>;
    {std::is_trivial_v<typename T::value_type>};
};

/// @brief Base class for binary archives
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

    // --------------------------
    //  Trivial types
    // --------------------------
    template <typename T>
    BinaryFileArchive& operator<<(const T& data)
    {
        static_assert(std::is_trivial_v<T>);
        assert(IsWriting());

        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
        pFileStream->write(reinterpret_cast<const char*>(&data), sizeof(T));

        return *this;
    }

    template <typename T>
    const BinaryFileArchive& operator>>(T& data) const
    {
        static_assert(std::is_trivial_v<T>);
        assert(IsReading());

        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
        pFileStream->read(reinterpret_cast<char*>(&data), sizeof(T));

        return *this;
    }

    // --------------------------
    //  Containers
    // --------------------------
    template <TrivialTypeContiguousContainer T>
    BinaryFileArchive& operator<<(const T& data)
    {
        assert(IsWriting());
        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);

        auto size = data.size();
        pFileStream->write(reinterpret_cast<const char*>(&size), sizeof(T::size_type));
        pFileStream->write(reinterpret_cast<const char*>(data.data()), size * sizeof(T::value_type));

        return *this;
    }

    template <TrivialTypeContiguousContainer T>
    const BinaryFileArchive& operator>>(T& data) const
    {
        assert(IsReading());
        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);

        typename T::size_type size;
        pFileStream->read(reinterpret_cast<char*>(&size), sizeof(T::size_type));

        data.resize(size);
        pFileStream->read(reinterpret_cast<char*>(data.data()), size * sizeof(T::value_type));

        return *this;
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
    // --------------------------
    //  Binary archives
    // --------------------------
    friend const BinaryFileArchive& operator>>(const BinaryFileArchive& in, BinaryMemoryArchive& out);
    friend BinaryFileArchive& operator<<(BinaryFileArchive& out, const BinaryMemoryArchive& in);
    friend const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& in, BinaryFileArchive& out);
    friend BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, const BinaryFileArchive& in);
};

/// @brief Archive view of an existing binary memory array
class BinaryMemoryArchive : public IBinaryArchive
{
  private:
    std::vector<std::byte>& m_memory;
    mutable std::vector<std::byte>::iterator m_pReader;

  public:
    BinaryMemoryArchive(std::vector<std::byte>& memory, IOMode mode) : IBinaryArchive(mode), m_memory(memory)
    {
        if (IsReading())
        {
            m_pReader = m_memory.begin();
        }
    }

    bool IsValid() const override { return m_pReader != m_memory.begin(); }

    // --------------------------
    //  Trivial types
    // --------------------------
    template <typename T>
    BinaryMemoryArchive& operator<<(const T& data)
    {
        static_assert(std::is_trivial_v<T>);
        assert(IsWriting());

        auto pData = reinterpret_cast<const std::byte*>(&data);
        m_memory.insert(m_memory.end(), pData, pData + sizeof(T));

        return *this;
    }

    template <typename T>
    const BinaryMemoryArchive& operator>>(T& data) const
    {
        static_assert(std::is_trivial_v<T>);
        assert(IsReading());

        memcpy(&data, &*m_pReader, sizeof(T));
        m_pReader += sizeof(T);

        return *this;
    }

    // --------------------------
    //  Containers
    // --------------------------
    template <TrivialTypeContiguousContainer T>
    BinaryMemoryArchive& operator<<(const T& container)
    {
        assert(IsWriting());

        auto containerSize = container.size();
        *this << containerSize;

        auto pData = reinterpret_cast<const std::byte*>(container.data());
        m_memory.insert(m_memory.end(), pData, pData + (containerSize * sizeof(T::value_type)));

        return *this;
    }

    template <TrivialTypeContiguousContainer T>
    const BinaryMemoryArchive& operator>>(T& container) const
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
    //  Binary archives
    // --------------------------
    friend const BinaryFileArchive& operator>>(const BinaryFileArchive& in, BinaryMemoryArchive& out);
    friend BinaryFileArchive& operator<<(BinaryFileArchive& out, const BinaryMemoryArchive& in);
    friend const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& in, BinaryFileArchive& out);
    friend BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, const BinaryFileArchive& in);
};

// --------------------
// Interop
// --------------------

/// @brief Save the full file in memory
BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, const BinaryFileArchive& in);
const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& in, BinaryFileArchive& out);
BinaryFileArchive& operator<<(BinaryFileArchive& out, const BinaryMemoryArchive& in);
const BinaryFileArchive& operator>>(const BinaryFileArchive& in, BinaryMemoryArchive& out);

} // namespace aln
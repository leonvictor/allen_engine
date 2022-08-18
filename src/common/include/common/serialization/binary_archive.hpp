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
/// @todo They are more archives than streams... What do i call them ?
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
};

/// @brief A binary file to write to or read from
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

    // --------------------------
    //  Binary archives
    // --------------------------
    friend const BinaryFileArchive& operator>>(const BinaryFileArchive& is, BinaryMemoryArchive& os);
    friend BinaryFileArchive& operator<<(BinaryFileArchive& os, const BinaryMemoryArchive& is);
    friend const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& is, BinaryFileArchive& os);
    friend BinaryMemoryArchive& operator<<(BinaryMemoryArchive& os, const BinaryFileArchive& is);
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

    /// --------------------------
    /// Compression
    /// @note This version creates a copy of the data in order to compress/decompress it
    /// A cleverer alternative probably exists
    /// --------------------------

    /// @brief Compress the archive
    /// @return The original size of the uncompressed archive
    size_t Compress()
    {
        auto uncompressedSize = static_cast<int>(m_memory.size());
        auto maxCompressedSize = LZ4_compressBound(uncompressedSize);

        auto buffer = std::vector<std::byte>(maxCompressedSize);

        auto compressedSize = LZ4_compress_default(
            reinterpret_cast<const char*>(m_memory.data()),
            reinterpret_cast<char*>(buffer.data()),
            uncompressedSize,
            maxCompressedSize);

        buffer.resize(compressedSize);
        m_memory.swap(buffer);

        if (IsReading())
        {
            m_pReader = m_memory.begin();
        }

        return uncompressedSize;
    }

    void Decompress(size_t originalSize)
    {
        auto buffer = std::vector<std::byte>(originalSize);

        LZ4_decompress_safe(
            reinterpret_cast<const char*>(m_memory.data()),
            reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(m_memory.size()),
            static_cast<int>(originalSize));

        m_memory.swap(buffer);

        if (IsReading())
        {
            m_pReader = m_memory.begin();
        }
    }

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
    friend const BinaryFileArchive& operator>>(const BinaryFileArchive& is, BinaryMemoryArchive& os);
    friend BinaryFileArchive& operator<<(BinaryFileArchive& os, const BinaryMemoryArchive& is);
    friend const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& is, BinaryFileArchive& os);
    friend BinaryMemoryArchive& operator<<(BinaryMemoryArchive& os, const BinaryFileArchive& is);
};

/// --------------------
/// Interop
/// --------------------
BinaryMemoryArchive& operator<<(BinaryMemoryArchive& os, const BinaryFileArchive& is)
{
    // Save the full file in memory
    assert(os.IsWriting() && is.IsReading());
    auto pFileStream = reinterpret_cast<std::ifstream*>(is.m_pFileStream);

    auto currentFilePos = pFileStream->tellg();

    // Find the file's size
    pFileStream->seekg(0, pFileStream->end);
    auto size = (size_t) pFileStream->tellg();
    pFileStream->seekg(0, pFileStream->beg);

    // Write the stream's size
    os << size;

    // Reserve memory and read
    auto originalSize = os.m_memory.size();
    os.m_memory.resize(os.m_memory.size() + size);

    auto pEnd = os.m_memory.data() + originalSize;
    pFileStream->read(reinterpret_cast<char*>(pEnd), size);

    // Put the file reader back to its original position, just in case
    pFileStream->seekg(currentFilePos);

    return os;
};

const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& is, BinaryFileArchive& os)
{
    assert(os.IsWriting() && is.IsReading());
    auto pFileStream = reinterpret_cast<std::ofstream*>(os.m_pFileStream);

    // Overwrite the content of the file by reopening it
    pFileStream->close();
    pFileStream->open(os.m_path, std::ios::binary | std::ios::out);

    // Extract the stream's size
    size_t size;
    is >> size;

    // Write the data from the current reading position to the end
    // auto size = std::distance(is.m_pReader, is.m_memory.end()); // TODO: This might not work
    pFileStream->write(reinterpret_cast<const char*>(is.m_memory.data()), size);

    // Invalidate the memory stream which as reached the end of the buffer
    is.m_pReader += size;

    return is;
};

BinaryFileArchive& operator<<(BinaryFileArchive& os, const BinaryMemoryArchive& is)
{
    assert(os.IsWriting() && is.IsReading());
    os << is.m_memory;
    return os;
};

const BinaryFileArchive& operator>>(const BinaryFileArchive& is, BinaryMemoryArchive& os)
{
    assert(os.IsWriting() && is.IsReading());
    is >> os.m_memory;
    return is;
};

} // namespace aln
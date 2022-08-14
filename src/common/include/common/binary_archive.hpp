#pragma once

#include <assert.h>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <string>

#include <common/memory.hpp>

namespace aln
{
/// @brief Concept for container-like types such as std::vector and std::string
/// @note This is not very restrictive, we have to be careful when serializing data
/// @todo: clang-format supports formatting one liners with RequiresClausePosition: OwnLine
/// from v15 onward. Upgrade as soon as we can !
template <typename T>
concept SequenceContainer = requires(T a)
{
    T::value_type;
    T::size_type;
    { a.size() } -> std::same_as<typename T::size_type>;
    { a.data() } -> std::same_as<typename T::value_type*>;
};

class BinaryArchive
{
  public:
    enum class Mode
    {
        Read,
        Write
    };

  private:
    Mode m_mode;
    void* m_pFileStream = nullptr;

  public:
    BinaryArchive(const std::filesystem::path& path, Mode mode) : m_mode(mode)
    {
        int flags = std::ios::binary;
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

    ~BinaryArchive()
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

    bool IsReading() const { return m_mode == Mode::Read; }
    bool IsWriting() const { return m_mode == Mode::Write; }

    template <typename T>
    BinaryArchive& operator<<(const T& data)
    {
        assert(IsWriting());
        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);
        pFileStream->write(reinterpret_cast<const char*>(&data), sizeof(T));

        return *this;
    }

    template <typename T>
    const BinaryArchive& operator>>(T& data) const
    {
        assert(IsReading());
        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);
        pFileStream->read(reinterpret_cast<char*>(&data), sizeof(T));

        return *this;
    }

    template <SequenceContainer T>
    BinaryArchive& operator<<(const T& data)
    {
        assert(IsWriting());
        auto pFileStream = reinterpret_cast<std::ofstream*>(m_pFileStream);

        auto size = data.size();
        pFileStream->write(reinterpret_cast<const char*>(&size), sizeof(T::size_type));
        pFileStream->write(reinterpret_cast<const char*>(data.data()), size * sizeof(T::value_type));

        return *this;
    }

    template <SequenceContainer T>
    const BinaryArchive& operator>>(T& data) const
    {
        assert(IsReading());
        auto pFileStream = reinterpret_cast<std::ifstream*>(m_pFileStream);

        typename T::size_type size;
        pFileStream->read(reinterpret_cast<char*>(&size), sizeof(T::size_type));

        data.resize(size);
        pFileStream->read(reinterpret_cast<char*>(data.data()), size * sizeof(T::value_type));

        return *this;
    }
};
} // namespace aln
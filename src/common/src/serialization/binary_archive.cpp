#include "serialization/binary_archive.hpp"

namespace aln
{
/// @brief Save the full file in memory
BinaryMemoryArchive& operator<<(BinaryMemoryArchive& out, const BinaryFileArchive& in)
{
    assert(out.IsWriting() && in.IsReading());
    auto pFileStream = reinterpret_cast<std::ifstream*>(in.m_pFileStream);

    auto currentFilePos = pFileStream->tellg();

    // Find the file's size
    pFileStream->seekg(0, pFileStream->end);
    auto size = (size_t) pFileStream->tellg();
    pFileStream->seekg(0, pFileStream->beg);

    // Write the archive size
    out << size;

    // Reserve memory and read
    auto originalSize = out.m_memory.size();
    out.m_memory.resize(originalSize + size);

    auto pEnd = out.m_memory.data() + originalSize;
    pFileStream->read(reinterpret_cast<char*>(pEnd), size);

    // Put the file reader back to its original position, just in case
    pFileStream->seekg(currentFilePos);

    return out;
};

const BinaryMemoryArchive& operator>>(const BinaryMemoryArchive& in, BinaryFileArchive& out)
{
    assert(out.IsWriting() && in.IsReading());
    auto pFileStream = reinterpret_cast<std::ofstream*>(out.m_pFileStream);

    // Overwrite the content of the file by reopening it
    pFileStream->close();
    pFileStream->open(out.m_path, std::ios::binary | std::ios::out);

    // Extract the archive size
    size_t size;
    in >> size;

    // Write the data from the current reading position to the end
    pFileStream->write(reinterpret_cast<const char*>(in.m_memory.data()), size);

    // Update the reader ptr to point to the new end
    in.m_pReader += size;

    return in;
};

BinaryFileArchive& operator<<(BinaryFileArchive& out, const BinaryMemoryArchive& in)
{
    assert(out.IsWriting() && in.IsReading());
    out << in.m_memory;
    return out;
};

const BinaryFileArchive& operator>>(const BinaryFileArchive& in, BinaryMemoryArchive& out)
{
    assert(out.IsWriting() && in.IsReading());
    in >> out.m_memory;
    return in;
};
} // namespace aln
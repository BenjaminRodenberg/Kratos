//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   license: HDF5Application/license.txt
//
//  Main author:     Máté Kelemen
//

// --- Internal Includes ---
#include "custom_utilities/file_content_iterator.h"

// --- STL Includes ---
#include <limits>


namespace Kratos
{


FileContentIterator::FileContentIterator(std::shared_ptr<FileAccess>&& rpFileAccess,
                                         char delimiter)
    : mpFileAccess(std::move(rpFileAccess)),
      mDelimiter(delimiter),
      mPosition(0)
{
}


FileContentIterator::FileContentIterator(const std::shared_ptr<FileAccess>& rpFileAccess,
                                         char delimiter)
    : mpFileAccess(rpFileAccess),
      mDelimiter(delimiter),
      mPosition(0)
{
}


FileContentIterator& FileContentIterator::operator++()
{
    this->RestoreState();
    this->SeekNext(mDelimiter);
    this->StoreState();
    return *this;
}


FileContentIterator FileContentIterator::operator++(int)
{
    this->RestoreState();
    FileContentIterator copy = *this;
    ++(*this);
    return copy;
}


FileContentIterator::value_type FileContentIterator::operator*() const
{
    // Detect if EOF is being dereferenced
    {
        FileContentIterator it_eof = *this;
        it_eof.SeekEOF();
        KRATOS_ERROR_IF(it_eof.mPosition == this->mPosition)
        << "Attempt to dereference file end iterator";
    }

    this->RestoreState();
    FileContentIterator end = *this;
    ++end;
    return std::make_pair(*this, end);
}


std::string FileContentIterator::value() const
{
    const auto range = **this;
    std::string output;

    this->RestoreState();
    auto& r_stream = this->mpFileAccess->value().first;
    while (r_stream.tellg() != range.second.GetPosition()) {
        // Use the convoluted syntax to get unformatted output
        char c;
        r_stream.get(c);
        output.push_back(c);
    }

    return output;
}


bool FileContentIterator::SeekNext(char Target)
{
    this->RestoreState();
    auto& r_stream = this->mpFileAccess->value().first;

    if (r_stream.eof()) {
        return false;
    }

    char c;
    while (r_stream.get(c) && c != mDelimiter) {}
    return c == mDelimiter;
}


void FileContentIterator::SeekEOF()
{
    this->mpFileAccess->value().first.seekg(0, std::ios::end);
    this->StoreState();
}


FileContentIterator::Position FileContentIterator::GetPosition() const noexcept
{
    return mPosition;
}


void FileContentIterator::StoreState()
{
    KRATOS_TRY;
    this->mPosition = this->mpFileAccess->value().first.tellg();
    KRATOS_CATCH("");
}


void FileContentIterator::RestoreState() const
{
    KRATOS_TRY;
    this->mpFileAccess->value().first.seekg(mPosition);
    KRATOS_CATCH("");
}


bool operator==(const FileContentIterator& rLeft, const FileContentIterator& rRight)
{
    KRATOS_ERROR_IF_NOT(&rLeft.mpFileAccess->value().first == &rRight.mpFileAccess->value().first)
    << "Comparison of incompatible iterators pointing to different files.";
    return rLeft.mPosition == rRight.mPosition;
}


bool operator!=(const FileContentIterator& rLeft, const FileContentIterator& rRight)
{
    return !(rLeft == rRight);
}


FileStringIterator::FileStringIterator(std::shared_ptr<FileAccess>&& rpFileAccess, char delimiter)
    : mWrapped(std::move(rpFileAccess), delimiter)
{
}


FileStringIterator::FileStringIterator(const std::shared_ptr<FileAccess>& rpFileAccess, char delimiter)
    : mWrapped(rpFileAccess, delimiter)
{
}


FileStringIterator& FileStringIterator::operator++()
{
    ++mWrapped;
    return *this;
}


FileStringIterator FileStringIterator::operator++(int)
{
    FileStringIterator copy = *this;
    ++(*this);
    return copy;
}


FileStringIterator::value_type FileStringIterator::operator*() const
{
    return mWrapped.value();
}


void FileStringIterator::SeekEOF()
{
    mWrapped.SeekEOF();
}


bool operator==(const FileStringIterator& rLeft, const FileStringIterator& rRight)
{
    return rLeft.mWrapped == rRight.mWrapped;
}


bool operator!=(const FileStringIterator& rLeft, const FileStringIterator& rRight)
{
    return rLeft.mWrapped != rRight.mWrapped;
}


} // namespace Kratos

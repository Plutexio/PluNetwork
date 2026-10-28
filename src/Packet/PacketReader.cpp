//
// Created by Plutex on 10/28/26.
//

#include "PluNetwork/Packet/PacketReader.h"

#include <cstring>

PluNetwork::PacketReader::PacketReader(const std::byte* data, UInt64 size)
    : mData(data), mSize(data ? size : 0)
{
}

PluNetwork::PacketReader::PacketReader(const Plu::DynamicArray<std::byte>& data)
    : PacketReader(data.Data(), data.Size())
{
}

bool PluNetwork::PacketReader::Read(Plu::String* string)
{
    UInt32 length = 0;
    if (!Read(&length)) return false;
    if (length > Remaining()) return Fail();
    *string = Plu::String(reinterpret_cast<const char*>(mData + mPosition), length);
    if (string->Length() != length) return Fail();
    mPosition += length;
    return true;
}

bool PluNetwork::PacketReader::ReadBytes(std::byte* data, UInt64 size)
{
    if (mFailed || size > Remaining()) return Fail();
    if (size == 0) return true;
    std::memcpy(data, mData + mPosition, size);
    mPosition += size;
    return true;
}

bool PluNetwork::PacketReader::Skip(UInt64 size)
{
    if (mFailed || size > Remaining()) return Fail();
    mPosition += size;
    return true;
}

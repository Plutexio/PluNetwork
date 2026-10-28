//
// Created by Plutex on 10/28/26.
//

#include "PluNetwork/Packet/PacketWriter.h"

#include <cstring>

PluNetwork::PacketWriter::PacketWriter(UInt64 reserve)
{
    mBuffer.Reserve(reserve);
}

void PluNetwork::PacketWriter::Write(const Plu::String& string)
{
    if (!WriteLength(string.Length())) return;
    WriteBytes(reinterpret_cast<const std::byte*>(string.CStr()), string.Length());
}

void PluNetwork::PacketWriter::WriteBytes(const std::byte* data, UInt64 size)
{
    if (mFailed || size == 0) return;
    const UInt64 offset = mBuffer.Size();
    const UInt64 newSize = offset + size;
    //DynamicArray::Resize grows to exactly the asked size, many small writes would reallocate every time
    if (newSize > mBuffer.Capacity()) {
        UInt64 grown = mBuffer.Capacity() * 2;
        mBuffer.Reserve(grown > newSize ? grown : (newSize > 64 ? newSize : 64));
    }
    mBuffer.Resize(newSize);
    if (mBuffer.Size() != newSize) {
        mFailed = true;
        return;
    }
    std::memcpy(mBuffer.Data() + offset, data, size);
}

bool PluNetwork::PacketWriter::WriteLength(UInt64 length)
{
    if (length > UINT32_MAX) {
        mFailed = true;
        return false;
    }
    Write(static_cast<UInt32>(length));
    return !mFailed;
}

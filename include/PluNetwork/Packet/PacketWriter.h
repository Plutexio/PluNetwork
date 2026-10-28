//
// Created by Plutex on 10/28/26.
//

#ifndef PLUNETWORK_PACKETWRITER_H
#define PLUNETWORK_PACKETWRITER_H

#include <bit>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

#include <PluBase/Base.h>
#include <PluSTL/Array/Array.h>
#include <PluSTL/String/String.h>

#include "PluNetwork/Core.h"

namespace PluNetwork
{
    class PacketWriter;
    class PacketReader;

    //Integers, floats, bool and enums. Written by their size, so prefer the fixed-width aliases (long is 4 bytes on Windows, 8 on Linux)
    template<typename T>
    concept PacketPrimitive = std::is_arithmetic_v<T> || std::is_enum_v<T>;

    //One byte types that can be copied in bulk (bool is left out, the reader validates it)
    template<typename T>
    concept PacketRawByte = std::is_same_v<T, std::byte> || (std::is_integral_v<T> && sizeof(T) == 1 && !std::is_same_v<T, bool>);

    //Own types: a member function...
    //    void Serialize(PluNetwork::PacketWriter& writer) const;
    template<typename T>
    concept MemberSerializable = requires(const T& value, PacketWriter& writer) { value.Serialize(writer); };

    //...or a free function found by ADL, for types you can't change (put it in the type's namespace or in PluNetwork)
    //    void Serialize(PluNetwork::PacketWriter& writer, const T& value);
    template<typename T>
    concept FreeSerializable = requires(const T& value, PacketWriter& writer) { Serialize(writer, value); };

    //Builds a packet in network byte order (big endian). Strings and arrays get a 4-byte length prefix
    class PLUNETWORK_API PacketWriter
    {
        Plu::DynamicArray<std::byte> mBuffer;
        bool mFailed = false;
    public:
        PacketWriter() = default;
        explicit PacketWriter(UInt64 reserve);

        template<typename T> requires PacketPrimitive<T> || MemberSerializable<T> || FreeSerializable<T>
        void Write(const T& value)
        {
            if constexpr (PacketPrimitive<T>) WritePrimitive(value);
            else if constexpr (MemberSerializable<T>) value.Serialize(*this);
            else Serialize(*this, value);
        }

        void Write(const Plu::String& string);

        //Element count followed by the elements
        template<typename T, typename Allocator>
        void Write(const Plu::DynamicArray<T, Allocator>& array)
        {
            if (!WriteLength(array.Size())) return;
            if constexpr (PacketRawByte<T>) {
                WriteBytes(reinterpret_cast<const std::byte*>(array.Data()), array.Size());
            } else {
                for (const T& element : array) Write(element);
            }
        }

        //Raw bytes, no length prefix
        void WriteBytes(const std::byte* data, UInt64 size);

        [[nodiscard]] const Plu::DynamicArray<std::byte>& Buffer() const { return mBuffer; }
        [[nodiscard]] Plu::DynamicArray<std::byte> TakeBuffer() { return std::move(mBuffer); }
        [[nodiscard]] UInt64 Size() const { return mBuffer.Size(); }
        //Allocation failed or something was too big for its length prefix, the packet is incomplete
        [[nodiscard]] bool HasFailed() const { return mFailed; }

        //Keeps the capacity, so one writer can be reused for every packet
        void Clear() { mBuffer.Clear(); mFailed = false; }

    private:
        bool WriteLength(UInt64 length);

        template<typename U>
        void WriteUnsigned(U value)
        {
            std::byte bytes[sizeof(U)];
            for (std::size_t i = 0; i < sizeof(U); ++i)
                bytes[i] = static_cast<std::byte>(value >> (8 * (sizeof(U) - 1 - i)));
            WriteBytes(bytes, sizeof(U));
        }

        template<PacketPrimitive T>
        void WritePrimitive(T value)
        {
            if constexpr (std::is_same_v<T, bool>) {
                WriteUnsigned(static_cast<UInt8>(value ? 1 : 0));
            } else if constexpr (std::is_enum_v<T>) {
                WritePrimitive(static_cast<std::underlying_type_t<T>>(value));
            } else if constexpr (std::is_floating_point_v<T>) {
                static_assert(std::numeric_limits<T>::is_iec559 && (sizeof(T) == 4 || sizeof(T) == 8),
                              "Only 32 and 64 bit IEEE floats can be sent");
                using Bits = std::conditional_t<sizeof(T) == 4, UInt32, UInt64>;
                WriteUnsigned(std::bit_cast<Bits>(value));
            } else {
                WriteUnsigned(static_cast<std::make_unsigned_t<T>>(value));
            }
        }
    };
}

#endif //PLUNETWORK_PACKETWRITER_H

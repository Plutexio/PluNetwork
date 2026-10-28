//
// Created by Plutex on 10/28/26.
//

#ifndef PLUNETWORK_PACKETREADER_H
#define PLUNETWORK_PACKETREADER_H

#include "PluNetwork/Packet/PacketWriter.h"

namespace PluNetwork
{
    //Own types: a member function...
    //    bool Deserialize(PluNetwork::PacketReader& reader);
    template<typename T>
    concept MemberDeserializable = requires(T& value, PacketReader& reader) { { value.Deserialize(reader) } -> std::convertible_to<bool>; };

    //...or a free function found by ADL
    //    bool Deserialize(PluNetwork::PacketReader& reader, T* value);
    template<typename T>
    concept FreeDeserializable = requires(T* value, PacketReader& reader) { { Deserialize(reader, value) } -> std::convertible_to<bool>; };

    //Reads what PacketWriter wrote, without copying the buffer, so the data has to outlive the reader.
    //Every read is bounds checked: a short or malformed packet makes it return false, never read past the end.
    //Failure is sticky, after the first failed read every next one fails too, so a chain of reads
    //can be checked once at the end with HasFailed()
    class PLUNETWORK_API PacketReader
    {
        const std::byte* mData = nullptr;
        UInt64 mSize = 0;
        UInt64 mPosition = 0;
        bool mFailed = false;
    public:
        PacketReader(const std::byte* data, UInt64 size);
        explicit PacketReader(const Plu::DynamicArray<std::byte>& data);
        //Would point into a destroyed buffer
        explicit PacketReader(Plu::DynamicArray<std::byte>&&) = delete;

        template<typename T> requires PacketPrimitive<T> || MemberDeserializable<T> || FreeDeserializable<T>
        bool Read(T* value)
        {
            if (mFailed) return false;
            bool ok;
            if constexpr (PacketPrimitive<T>) ok = ReadPrimitive(value);
            else if constexpr (MemberDeserializable<T>) ok = value->Deserialize(*this);
            else ok = Deserialize(*this, value);
            if (!ok) mFailed = true;
            return !mFailed;
        }

        bool Read(Plu::String* string);

        template<typename T, typename Allocator>
        bool Read(Plu::DynamicArray<T, Allocator>* array)
        {
            UInt32 count = 0;
            if (!Read(&count)) return false;
            //Every element takes at least one byte, so a bigger count can only be a broken packet.
            //Checked before allocating, otherwise 4 bytes from the network could ask for gigabytes
            if (count > Remaining()) return Fail();

            array->Clear();
            if constexpr (PacketRawByte<T>) {
                array->Resize(count);
                if (array->Size() != count) return Fail();
                return ReadBytes(reinterpret_cast<std::byte*>(array->Data()), count);
            } else {
                array->Reserve(count);
                for (UInt32 i = 0; i < count; ++i) {
                    T element{};
                    if (!Read(&element)) return false;
                    array->PushBack(std::move(element));
                }
                return true;
            }
        }

        //Raw bytes, no length prefix
        bool ReadBytes(std::byte* data, UInt64 size);
        bool Skip(UInt64 size);

        [[nodiscard]] UInt64 Remaining() const { return mSize - mPosition; }
        [[nodiscard]] UInt64 Position() const { return mPosition; }
        //True when the whole packet was read, extra bytes after the last field usually mean a mismatched message
        [[nodiscard]] bool IsAtEnd() const { return mPosition == mSize; }
        [[nodiscard]] bool HasFailed() const { return mFailed; }

    private:
        bool Fail() { mFailed = true; return false; }

        template<typename U>
        bool ReadUnsigned(U* value)
        {
            std::byte bytes[sizeof(U)];
            if (!ReadBytes(bytes, sizeof(U))) return false;
            U result = 0;
            for (std::size_t i = 0; i < sizeof(U); ++i)
                result = static_cast<U>((result << 8) | static_cast<U>(bytes[i]));
            *value = result;
            return true;
        }

        template<PacketPrimitive T>
        bool ReadPrimitive(T* value)
        {
            if constexpr (std::is_same_v<T, bool>) {
                UInt8 byte = 0;
                if (!ReadUnsigned(&byte)) return false;
                if (byte > 1) return Fail();
                *value = byte == 1;
            } else if constexpr (std::is_enum_v<T>) {
                //The value isn't checked against the enumerators, validate it yourself if that matters
                std::underlying_type_t<T> underlying{};
                if (!ReadPrimitive(&underlying)) return false;
                *value = static_cast<T>(underlying);
            } else if constexpr (std::is_floating_point_v<T>) {
                static_assert(std::numeric_limits<T>::is_iec559 && (sizeof(T) == 4 || sizeof(T) == 8),
                              "Only 32 and 64 bit IEEE floats can be received");
                using Bits = std::conditional_t<sizeof(T) == 4, UInt32, UInt64>;
                Bits bits = 0;
                if (!ReadUnsigned(&bits)) return false;
                *value = std::bit_cast<T>(bits);
            } else {
                std::make_unsigned_t<T> bits = 0;
                if (!ReadUnsigned(&bits)) return false;
                *value = static_cast<T>(bits);
            }
            return true;
        }
    };
}

#endif //PLUNETWORK_PACKETREADER_H

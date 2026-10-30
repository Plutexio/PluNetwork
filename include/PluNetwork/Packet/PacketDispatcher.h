//
// Created by Plutex on 10/28/26.
//
// Packets that say what they are. WritePacket puts a 2-byte id in front of the message,
// PacketDispatcher reads it back and hands the message to the handler registered for that id,
// so the receiver doesn't have to know up front which type is coming.
//

#ifndef PLUNETWORK_PACKETDISPATCHER_H
#define PLUNETWORK_PACKETDISPATCHER_H

#include <functional>
#include <utility>

#include <PluSTL/HashMap/HashMap.h>

#include "PluNetwork/Packet/PacketReader.h"
#include "PluNetwork/Packet/PacketWriter.h"

namespace PluNetwork
{
    using PacketId = UInt16;

    //A message with an id, either an integer or an enum value:
    //    static constexpr MyPacketType Type = MyPacketType::Chat;
    //Ids are part of the protocol, give them fixed values and never reuse one
    template<typename T>
    concept IdentifiedPacket = requires { T::Type; }
        && (std::is_integral_v<std::remove_cvref_t<decltype(T::Type)>> || std::is_enum_v<std::remove_cvref_t<decltype(T::Type)>>)
        && std::default_initializable<T>
        && (MemberSerializable<T> || FreeSerializable<T>)
        && (MemberDeserializable<T> || FreeDeserializable<T>);

    template<IdentifiedPacket T>
    consteval PacketId PacketIdOf()
    {
        using IdType = std::remove_cvref_t<decltype(T::Type)>;
        if constexpr (std::is_enum_v<IdType>) {
            static_assert(std::in_range<PacketId>(static_cast<std::underlying_type_t<IdType>>(T::Type)), "Packet id doesn't fit in PacketId");
        } else {
            static_assert(std::in_range<PacketId>(T::Type), "Packet id doesn't fit in PacketId");
        }
        return static_cast<PacketId>(T::Type);
    }

    //The id followed by the message, what PacketDispatcher expects
    template<IdentifiedPacket T>
    void WritePacket(PacketWriter& writer, const T& message)
    {
        writer.Write(PacketIdOf<T>());
        writer.Write(message);
    }

    enum class DispatchResult : UInt8
    {
        Handled,
        //No handler for this id: an older/newer peer or garbage from the network
        UnknownPacket,
        //Too short for an id, or the message didn't parse or had bytes left over
        Malformed,
    };

    //Context is whatever the handlers need next to the message, passed through Dispatch as is,
    //e.g. PacketDispatcher<const Address&> for who sent a datagram
    template<typename... Context>
    class PacketDispatcher
    {
        //false when the message didn't parse
        using Handler = std::function<bool(PacketReader&, Context...)>;
        Plu::HashMap<PacketId, Handler> mHandlers;
    public:
        //The handler gets the message as an rvalue, so it can take const T&, T&& or T.
        //Registering the same type again replaces the old handler. Don't do it from inside that handler
        template<IdentifiedPacket T, typename F> requires std::invocable<F&, T&&, Context...>
        void On(F&& handler)
        {
            mHandlers.InsertOrAssign(PacketIdOf<T>(), Handler(
                [handler = std::forward<F>(handler)](PacketReader& reader, Context... context) mutable -> bool {
                    T message{};
                    if (!reader.Read(&message) || !reader.IsAtEnd()) return false;
                    std::invoke(handler, std::move(message), std::forward<Context>(context)...);
                    return true;
                }));
        }

        template<IdentifiedPacket T>
        void Remove() { mHandlers.Remove(PacketIdOf<T>()); }

        template<IdentifiedPacket T>
        [[nodiscard]] bool Handles() const { return mHandlers.Contains(PacketIdOf<T>()); }

        DispatchResult Dispatch(const std::byte* data, UInt64 size, Context... context) const
        {
            PacketReader reader(data, size);
            PacketId id = 0;
            if (!reader.Read(&id)) return DispatchResult::Malformed;

            const Handler* handler = mHandlers.Find(id);
            if (!handler) return DispatchResult::UnknownPacket;
            return (*handler)(reader, std::forward<Context>(context)...) ? DispatchResult::Handled : DispatchResult::Malformed;
        }

        DispatchResult Dispatch(const Plu::DynamicArray<std::byte>& buffer, Context... context) const
        {
            return Dispatch(buffer.Data(), buffer.Size(), std::forward<Context>(context)...);
        }
    };
}

#endif //PLUNETWORK_PACKETDISPATCHER_H

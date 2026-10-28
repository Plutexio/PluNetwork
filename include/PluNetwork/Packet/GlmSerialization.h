//
// Created by Plutex on 10/28/26.
//
// Opt-in support for glm vectors, matrices and quaternions (Vec3, Matrix4, Quaternion...).
// Include it wherever such a type is written or read; without it the call doesn't compile.
//

#ifndef PLUNETWORK_GLMSERIALIZATION_H
#define PLUNETWORK_GLMSERIALIZATION_H

#include <glm/fwd.hpp>

#include "PluNetwork/Packet/PacketReader.h"
#include "PluNetwork/Packet/PacketWriter.h"

namespace PluNetwork
{
    template<glm::length_t L, typename T, glm::qualifier Q>
    void Serialize(PacketWriter& writer, const glm::vec<L, T, Q>& vector)
    {
        for (glm::length_t i = 0; i < L; ++i) writer.Write(vector[i]);
    }

    template<glm::length_t L, typename T, glm::qualifier Q>
    bool Deserialize(PacketReader& reader, glm::vec<L, T, Q>* vector)
    {
        for (glm::length_t i = 0; i < L; ++i)
            if (!reader.Read(&(*vector)[i])) return false;
        return true;
    }

    //Column by column, the way glm stores it
    template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
    void Serialize(PacketWriter& writer, const glm::mat<C, R, T, Q>& matrix)
    {
        for (glm::length_t i = 0; i < C; ++i) writer.Write(matrix[i]);
    }

    template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
    bool Deserialize(PacketReader& reader, glm::mat<C, R, T, Q>* matrix)
    {
        for (glm::length_t i = 0; i < C; ++i)
            if (!reader.Read(&(*matrix)[i])) return false;
        return true;
    }

    //x, y, z, w no matter which order GLM_FORCE_QUAT_DATA_WXYZ stores them in
    template<typename T, glm::qualifier Q>
    void Serialize(PacketWriter& writer, const glm::qua<T, Q>& quaternion)
    {
        writer.Write(quaternion.x);
        writer.Write(quaternion.y);
        writer.Write(quaternion.z);
        writer.Write(quaternion.w);
    }

    template<typename T, glm::qualifier Q>
    bool Deserialize(PacketReader& reader, glm::qua<T, Q>* quaternion)
    {
        return reader.Read(&quaternion->x) && reader.Read(&quaternion->y)
            && reader.Read(&quaternion->z) && reader.Read(&quaternion->w);
    }
}

#endif //PLUNETWORK_GLMSERIALIZATION_H

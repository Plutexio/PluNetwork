//
// Created by Plutex on 10/22/26.
//

#ifndef PLUNETWORK_CORE_H
#define PLUNETWORK_CORE_H

#include <PluBase/Base.h>

#ifdef PLU_BUILD_PLUNETWORK
    #define PLUNETWORK_API PLU_EXPORT
#else
    #define PLUNETWORK_API PLU_IMPORT
#endif

//Ones digit is major version (breaking changes), tenths number is minor changes in protocol (SHOULD work fine)
#define PLU_PROTOCOL_VERSION 1.0
#define PLU_MAGIC "PLUN"

#endif //PLUNETWORK_CORE_H

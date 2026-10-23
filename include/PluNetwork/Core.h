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

#endif //PLUNETWORK_CORE_H

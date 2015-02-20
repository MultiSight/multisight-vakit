
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// XSDK
// Copyright (c) 2015 Schneider Electric
//
// Use, modification, and distribution is subject to the Boost Software License,
// Version 1.0 (See accompanying file LICENSE).
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef __VAKit_BitStream_h
#define __VAKit_BitStream_h

#include "XSDK/Types.h"

namespace VAKit
{

class BitStream
{
public:
    BitStream();
    virtual ~BitStream() throw();

    void End();
    void PutUI( uint32_t val, int32_t size_in_bits );
    void PutUE( int32_t val );
    void PutSE( int32_t val );
    void ByteAligning( int32_t bit );

    uint8_t* Map();
    size_t Size();
    size_t SizeInBits();

private:
    int32_t _bitOffset;
    int32_t _maxSizeInDword;
    uint32_t* _buffer;
};

}

#endif

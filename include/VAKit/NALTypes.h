
#ifndef __VAKit_NALTypes_h
#define __VAKit_NALTypes_h

#include "VAKit/BitStream.h"

#ifndef WIN32

extern "C"
{
#include <va/va.h>
#include <va/va_enc_h264.h>
}

#endif

namespace VAKit
{

#ifndef WIN32
int BuildPackedPicBuffer( BitStream& bs,
                          VAEncPictureParameterBufferH264& pps,
                          bool annexB = true );

int BuildPackedSeqBuffer( BitStream& bs,
                          VAEncSequenceParameterBufferH264& sps,
                          const VAProfile& h264Profile,
                          int32_t constraintSetFlag,
                          uint32_t numUnitsInTick,
                          uint32_t timeScale,
                          uint32_t frameBitrate,
                          bool annexB = true );
#endif
}

#endif

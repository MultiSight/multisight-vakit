
#ifndef __VAKit_VAH264Encoder_h
#define __VAKit_VAH264Encoder_h

extern "C"
{
#include <va/va.h>
#include <va/va_enc_h264.h>
#include <va/va_drm.h>
}

#include "XSDK/Types.h"
#include "XSDK/XSocket.h"
#include "XSDK/XMemory.h"
#include "XSDK/XString.h"
#include "AVKit/Options.h"
#include "AVKit/FrameTypes.h"

#include <unistd.h>
#include <fcntl.h>

namespace VAKit
{

const size_t NUM_REFERENCE_FRAMES = 16;

class VAH264Encoder
{
public:

    X_API VAH264Encoder( const struct AVKit::CodecOptions& options,
                         const XSDK::XString& devicePath,
                         bool annexB = true );

    X_API virtual ~VAH264Encoder() throw();

    X_API size_t EncodeYUV420P( uint8_t* pic,
                                uint8_t* output,
                                size_t outputSize,
                                AVKit::FrameType type = AVKit::FRAME_TYPE_AUTO_GOP );

    X_API XIRef<XSDK::XMemory> EncodeYUV420P( XIRef<XSDK::XMemory> input,
                                              AVKit::FrameType type = AVKit::FRAME_TYPE_AUTO_GOP );


    X_API bool LastWasKey() const;

    X_API struct AVKit::CodecOptions GetOptions() const;

    X_API XIRef<XSDK::XMemory> GetExtraData() const;

private:

    int32_t _ComputeCurrentFrameType( uint32_t currentFrameNum,
                                      int32_t intraPeriod,
                                      AVKit::FrameType type ) const;

    void _UpdateReferenceFrames();
    void _UpdateRefPicList();
    void _RenderSequence();
    int32_t _CalcPOC( int32_t picOrderCntLSB );
    void _RenderPicture( bool done );
    void _RenderSlice();
    void _UploadImage( uint8_t* yv12, VAImage& image, uint16_t width, uint16_t height );

    XSDK::XString _devicePath;
    bool _annexB;
    int _fd;
    VADisplay _display;
    VAProfile _h264Profile;
    VAConfigID _configID;
    VASurfaceID _srcSurfaceID;
    VABufferID _codedBufID;
    VASurfaceID _refSurfaceIDs[NUM_REFERENCE_FRAMES];
    VAContextID _contextID;
    VAEncSequenceParameterBufferH264 _seqParam;
    VAEncPictureParameterBufferH264 _picParam;
    VAEncSliceParameterBufferH264 _sliceParam;
    VAPictureH264 _currentCurrPic;
    VAPictureH264 _referenceFrames[NUM_REFERENCE_FRAMES];
    VAPictureH264 _refPicListP[32];
    uint32_t _numShortTerm;
    int32_t _constraintSetFlag;
    int32_t _h264EntropyMode;
    int32_t _frameWidth;
    int32_t _frameWidthMBAligned;
    int32_t _frameHeight;
    int32_t _frameHeightMBAligned;
    uint32_t _frameBitRate;
    int32_t _intraPeriod;
    uint64_t _currentIDRDisplay;
    uint32_t _currentFrameNum;
    int32_t _currentFrameType;
    uint32_t _timeBaseNum;
    uint32_t _timeBaseDen;
    XIRef<XSDK::XMemory> _extraData;
    XIRef<XSDK::XMemory> _pkt;
    struct AVKit::CodecOptions _options;
};

}

#endif

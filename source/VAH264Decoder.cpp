
#include "VAKit/VAH264Decoder.h"
#include "MediaParser/MediaParser.h"
#include "XSDK/XException.h"
#include "XSDK/XGuard.h"

extern "C"
{
#include "libavutil/opt.h"
}

#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace VAKit;
using namespace AVKit;
using namespace XSDK;

static const size_t DEFAULT_PADDING = 16;

VAH264Decoder::VAH264Decoder( const struct CodecOptions& options )
#ifndef WIN32
  : _codec( avcodec_find_decoder( CODEC_ID_H264 ) ),
    _context( avcodec_alloc_context3( _codec ) ),
    _options( options ),
    _frame( avcodec_alloc_frame() ),
    _scaler( NULL ),
    _outputWidth( 0 ),
    _outputHeight( 0 ),
    _initComplete( false ),
    _fd( -1 ),
    _vc(),
    _attrib(),
    _surfaces(),
    _derivedImage(),
    _surfaceLock(),
    _surfaceOrder( 0 )
#endif
{
#ifndef WIN32
    if( !_codec )
        X_THROW(( "Failed to find H264 decoder." ));

    if( !_context )
        X_THROW(( "Failed to allocate decoder context." ));

    if( !_frame )
        X_THROW(( "Failed to allocate frame." ));

    _context->extradata = NULL;
    _context->extradata_size = 0;

    if( _options.device_path.IsNull() )
        X_THROW(( "device_path required for VAH264Decoder." ));

    // We stash our this pointer INSIDE our AVCodecContext because in some FFMPEG
    // callback functions we use, we need to get at some members of our object...
    _context->opaque = (void*)this;
#else
    X_THROW(("Not implemented."));
#endif
}

VAH264Decoder::~VAH264Decoder() throw()
{
#ifndef WIN32
    _DestroyVAAPIDecoder();

    _DestroyScaler();

    if( _frame )
        av_free( _frame );

    if( _context )
    {
        avcodec_close( _context );

        av_free( _context );
    }
#endif
}

void VAH264Decoder::Decode( uint8_t* frame, size_t frameSize )
{
#ifndef WIN32
    if( !_initComplete )
    {
        _FinishFFMPEGInit( frame, frameSize );

        _initComplete = true;
    }

    AVPacket inputPacket;
    av_init_packet( &inputPacket );
    inputPacket.data = frame;
    inputPacket.size = frameSize;

    int gotPicture = 0;
    int ret = 0;
    int decodeAttempts = 16;

    while( !gotPicture && decodeAttempts > 0 )
    {
        ret = avcodec_decode_video2( _context,
                                     _frame,
                                     &gotPicture,
                                     &inputPacket );
        if( ret < 0 )
            X_THROW(( "Decoding returned error: %d", ret ));

        decodeAttempts--;
    }

    if( gotPicture < 1 )
        X_THROW(( "Unable to decode frame." ));

    VAStatus status = vaGetImage( _vc.display,
                                  (VASurfaceID)(uintptr_t)_frame->data[3],
                                  0,
                                  0,
                                  _context->width,
                                  _context->height,
                                  _derivedImage.image_id );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaGetImage(): %s\n", vaErrorStr(status)));
#else
    X_THROW(("Not implemented."));
#endif
}

void VAH264Decoder::Decode( XIRef<XSDK::XMemory> frame )
{
#ifndef WIN32
    Decode( frame->Map(), frame->GetDataSize() );
#else
    X_THROW(("Not implemented."));
#endif
}

uint16_t VAH264Decoder::GetInputWidth() const
{
#ifndef WIN32
    return (uint16_t)_context->width;
#else
    X_THROW(("Not implemented."));
#endif
}

uint16_t VAH264Decoder::GetInputHeight() const
{
#ifndef WIN32
    return (uint16_t)_context->height;
#else
    X_THROW(("Not implemented."));
#endif
}

void VAH264Decoder::SetOutputWidth( uint16_t outputWidth )
{
#ifndef WIN32
    if( _outputWidth != outputWidth )
    {
        _outputWidth = outputWidth;

        if( _scaler )
            _DestroyScaler();
    }
#endif
}

uint16_t VAH264Decoder::GetOutputWidth() const
{
#ifndef WIN32
    return _outputWidth;
#else
    X_THROW(("Not implemented."));
#endif
}

void VAH264Decoder::SetOutputHeight( uint16_t outputHeight )
{
#ifndef WIN32
    if( _outputHeight != outputHeight )
    {
        _outputHeight = outputHeight;

        if( _scaler )
            _DestroyScaler();
    }
#endif
}

uint16_t VAH264Decoder::GetOutputHeight() const
{
#ifndef WIN32
    return _outputHeight;
#else
    X_THROW(("Not implemented."));
#endif
}

size_t VAH264Decoder::GetYUV420PSize() const
{
#ifndef WIN32
    if( (_outputWidth == 0) || (_outputHeight == 0) )
        return _context->width * _context->height * 1.5;

    return _outputWidth * _outputHeight * 1.5;
#else
    X_THROW(("Not implemented."));
#endif
}

void VAH264Decoder::MakeYUV420P( uint8_t* dest )
{
#ifndef WIN32
    if( _outputWidth == 0 )
        _outputWidth = _context->width;

    if( _outputHeight == 0 )
        _outputHeight = _context->height;

    unsigned char* surface_p = NULL;
    VAStatus status = vaMapBuffer( _vc.display, _derivedImage.buf, (void **)&surface_p );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaMapBuffer(): %s\n", vaErrorStr(status)));

    unsigned char* Y_start = surface_p + _derivedImage.offsets[0];
    int Y_pitch = _derivedImage.pitches[0];

    int U_pitch = 0;
    unsigned char* U_start = NULL;
    switch (_derivedImage.format.fourcc)
    {
    case VA_FOURCC_NV12:
        U_start = (unsigned char *)surface_p + _derivedImage.offsets[1];
        U_pitch = _derivedImage.pitches[1];
        break;
    default:
        X_THROW(("Fall into the fourcc that is not handled"));
    }

    if( _scaler == NULL )
    {
        _scaler = sws_getContext( _context->width,
                                  _context->height,
                                  PIX_FMT_NV12,
                                  _outputWidth,
                                  _outputHeight,
                                  (_options.jpeg_source.IsNull()) ? PIX_FMT_YUV420P : PIX_FMT_YUVJ420P,
                                  SWS_BICUBIC,
                                  NULL,
                                  NULL,
                                  NULL );

        if( !_scaler )
            X_THROW(( "Unable to allocate scaler context "
                      "(input_width=%u,input_height=%u,output_width=%u,output_height=%u).",
                      _context->width, _context->height, _outputWidth, _outputHeight ));
    }

    AVPicture pict;
    pict.data[0] = dest;
    dest += _outputWidth * _outputHeight;
    pict.data[1] = dest;
    dest += (_outputWidth/4) * _outputHeight;
    pict.data[2] = dest;

    pict.linesize[0] = _outputWidth;
    pict.linesize[1] = _outputWidth/2;
    pict.linesize[2] = _outputWidth/2;

    uint8_t* srcPlanes[2];
    srcPlanes[0] = Y_start;
    srcPlanes[1] = U_start;

    int srcStrides[2];
    srcStrides[0] = Y_pitch;
    srcStrides[1] = U_pitch;


    int ret = sws_scale( _scaler,
                         srcPlanes,
                         srcStrides,
                         0,
                         _context->height,
                         pict.data,
                         pict.linesize );
    if( ret <= 0 )
        X_THROW(( "Unable to create YUV420P image." ));

#endif
}

XIRef<XMemory> VAH264Decoder::MakeYUV420P()
{
#ifndef WIN32
    size_t pictureSize = GetYUV420PSize();
    XIRef<XMemory> buffer = new XMemory( pictureSize + DEFAULT_PADDING );
    uint8_t* p = &buffer->Extend( pictureSize );
    MakeYUV420P( p );
    return buffer;
#else
    X_THROW(("Not implemented."));
#endif
}

void VAH264Decoder::_FinishFFMPEGInit( uint8_t* frame, size_t frameSize )
{
#ifndef WIN32
    MEDIA_PARSER::H264Info h264Info;

    if( !MEDIA_PARSER::MediaParser::GetMediaInfo( frame, frameSize, h264Info ) )
        X_THROW(("Unable to parse SPS."));
    MEDIA_PARSER::MediaInfo* mediaInfo = &h264Info;

    _context->width = mediaInfo->GetFrameWidth();
    _context->height = mediaInfo->GetFrameHeight();
    _context->thread_count = 1;
    _context->get_format = _GetFormat;
    _context->get_buffer = _GetBuffer;
    _context->reget_buffer = _GetBuffer;
    _context->release_buffer = _ReleaseBuffer;
    _context->draw_horiz_band = NULL;
    _context->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

    if( avcodec_open2( _context, _codec, NULL ) < 0 )
        X_THROW(( "Unable to open H264 decoder." ));
#endif
}

void VAH264Decoder::_DestroyScaler()
{
#ifndef WIN32
    if( _scaler )
    {
        sws_freeContext( _scaler );
        _scaler = NULL;
    }
#endif
}

void VAH264Decoder::_InitVAAPIDecoder()
{
#ifndef WIN32
    XString devicePath = _options.device_path.Value();

    _fd = open( devicePath.c_str(), O_RDWR );
    if( _fd <= 0 )
        X_THROW(( "Unable to open: %s", devicePath.c_str() ));

    _vc.display = (VADisplay)vaGetDisplayDRM( _fd );

    if( !vaDisplayIsValid( _vc.display ) )
        X_THROW(("Unable to open a valid display."));

    int majorVer = 0, minorVer = 0;
    VAStatus status = vaInitialize( _vc.display, &majorVer, &minorVer );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaInitialize(): %s\n", vaErrorStr(status)));

    _attrib.type = VAConfigAttribRTFormat;
    vaGetConfigAttributes( _vc.display, VAProfileH264High, VAEntrypointVLD, &_attrib, 1 );
    if( (_attrib.value & VA_RT_FORMAT_YUV420) == 0 )
        X_THROW(("VA_RT_FORMAT_YUV420 is not supported."));

    status = vaCreateConfig( _vc.display,
                             VAProfileH264High,
                             VAEntrypointVLD,
                             &_attrib,
                             1,
                             &_vc.config_id );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaCreateConfig(): %s\n", vaErrorStr(status)));

    VASurfaceID surfaceIDs[NUM_VA_BUFFERS];
    status = vaCreateSurfaces( _vc.display,
                               _context->width,
                               _context->height,
                               VA_RT_FORMAT_YUV420,
                               NUM_VA_BUFFERS,
                               surfaceIDs );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaCreateSurfaces(): %s\n", vaErrorStr(status)));

    for( int i = 0; i < NUM_VA_BUFFERS; i++ )
    {
        _surfaces[i].id = surfaceIDs[i];
        _surfaces[i].refcount = 0;
        _surfaces[i].order = 0;
    }

    status = vaCreateContext( _vc.display,
                              _vc.config_id,
                              _context->width,
                              _context->height,
                              VA_PROGRESSIVE,
                              surfaceIDs,
                              NUM_VA_BUFFERS,
                              &_vc.context_id );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaCreateContext(): %s\n", vaErrorStr(status)));

    VAImageFormat formatList[vaMaxNumImageFormats( _vc.display )];
    int numFormats = 0;
    vaQueryImageFormats( _vc.display,
                         formatList,
                         &numFormats );

    VAImageFormat* nv12ImageFormat = NULL;
    for( int i = 0; i < numFormats; i++ )
    {
        char* fourCC = (char*)&formatList[i].fourcc;
        if( memcmp( fourCC, "NV12", 4 ) == 0 )
            nv12ImageFormat = &formatList[i];
    }

    if( !nv12ImageFormat )
        X_THROW(("Unable to locate NV12 VAImageFormat!"));

    status = vaCreateImage( _vc.display,
                            nv12ImageFormat,
                            _context->width,
                            _context->height,
                            &_derivedImage );
    if( status != VA_STATUS_SUCCESS )
        X_THROW(("Unable to vaCreateImage(): %s\n", vaErrorStr(status)));
#endif
}

void VAH264Decoder::_DestroyVAAPIDecoder()
{
#ifndef WIN32
    if( _initComplete )
    {
        vaDestroyImage( _vc.display, _derivedImage.image_id );
        vaDestroyContext( _vc.display, _vc.context_id );
        for( int i = 0; i < NUM_VA_BUFFERS; i++ )
        {
            vaDestroySurfaces( _vc.display, &_surfaces[i].id, 1 );
        }
        vaDestroyConfig( _vc.display, _vc.config_id );
        vaTerminate( _vc.display );

        close( _fd );

        _initComplete = false;
    }
#endif
}

enum PixelFormat VAH264Decoder::_GetFormat( struct AVCodecContext *avctx, const enum PixelFormat *fmt )
{
#ifndef WIN32
    VAH264Decoder* context = (VAH264Decoder*)avctx->opaque;
    if( !context )
        X_THROW(("Unable to get decoder context."));

    for( int i = 0; fmt[i] != PIX_FMT_NONE; i++ )
    {
        if( fmt[i] != PIX_FMT_VAAPI_VLD )
            continue;

        if( avctx->codec_id == CODEC_ID_H264 )
        {
            context->_InitVAAPIDecoder();

            avctx->hwaccel_context = &context->_vc;

            return fmt[i];
        }
    }

    return PIX_FMT_NONE;
#else
    X_THROW(("Not implemented."));
#endif
}

int VAH264Decoder::_GetBuffer( struct AVCodecContext* avctx, AVFrame* pic )
{
#ifndef WIN32
    VAH264Decoder* context = (VAH264Decoder*)avctx->opaque;
    if( !context )
        X_THROW(("Unable to get decoder context."));

    int oldest, i;

    {
        XGuard g( context->_surfaceLock );

        for( i = 0, oldest = 0; i < NUM_VA_BUFFERS; i++ )
        {
            struct HWSurface* surface = &context->_surfaces[i];

            if( !surface->refcount )
                break;

            if( surface->order < context->_surfaces[oldest].order )
                oldest = i;
        }
        if( i >= NUM_VA_BUFFERS )
            i = oldest;
    }

    struct HWSurface* surface = &context->_surfaces[i];

    surface->refcount = 1;
    surface->order = context->_surfaceOrder++;

    /* data[0] must be non-NULL for libavcodec internal checks.
     * data[3] actually contains the format-specific surface handle. */

    pic->opaque = surface;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->data[0] = (uint8_t*)(uintptr_t)surface->id;
    pic->data[1] = NULL;
    pic->data[2] = NULL;
    pic->data[3] = (uint8_t*)(uintptr_t)surface->id;
    pic->linesize[0] = 0;
    pic->linesize[1] = 0;
    pic->linesize[2] = 0;
    pic->linesize[3] = 0;

    return 0;
#else
    X_THROW(("Not implemented."));
#endif
}

void VAH264Decoder::_ReleaseBuffer( struct AVCodecContext* avctx, AVFrame* pic )
{
#ifndef WIN32
    VAH264Decoder* context = (VAH264Decoder*)avctx->opaque;
    if( !context )
        X_THROW(("Unable to get decoder context."));

    {
        XGuard g( context->_surfaceLock );
        struct HWSurface* surface = (struct HWSurface*)pic->opaque;
        surface->refcount--;
    }

    pic->data[0] = NULL;
    pic->data[1] = NULL;
    pic->data[2] = NULL;
    pic->data[3] = NULL;
#endif
}

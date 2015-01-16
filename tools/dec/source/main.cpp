
#include "XSDK/XIRef.h"
#include "XSDK/XString.h"
#include "XSDK/XMemory.h"
#include "XSDK/LargeFiles.h"
#include "XSDK/XStopWatch.h"
#include "XSDK/TimeUtils.h"
#include "AVKit/AVDeMuxer.h"
#include "AVKit/Options.h"
#include "AVKit/Locky.h"
#include "AVKit/Decoder.h"
#include "AVKit/H264Decoder.h"
#include "VAKit/VAH264Decoder.h"

using namespace XSDK;
using namespace AVKit;
using namespace VAKit;
using namespace std;

XIRef<XMemory> LoadFile( const XString& fileName )
{
    struct x_file_info fileInfo;
    x_stat( fileName, &fileInfo );

    XIRef<XMemory> buf = new XMemory();
    uint8_t* dest = &buf->Extend( fileInfo._fileSize );

    FILE* inFile = fopen( fileName.c_str(), "r+b" );
    if( !inFile )
        X_THROW(("Unable to open input file."));

    fread( dest, 1, fileInfo._fileSize, inFile );

    fclose( inFile );

    return buf;
}

int main( int argc, char* argv[] )
{
    if( argc < 4 )
    {
        printf("Invalid args.\n");
        fflush(stdout);
        exit(1);
    }

    XString inputFileName = argv[1];
    int fps = XString( argv[2] ).ToInt();
    bool useHW = (XString( argv[3] ).Contains( "yes" )) ? true : false;

    int64_t sleepMicros = 1000000 / fps;

    Locky::RegisterFFMPEG();

    XRef<Decoder> decoder;
    if( useHW )
        decoder = new VAH264Decoder( GetFastH264DecoderOptions( "/dev/dri/card0" ) );
    else decoder = new H264Decoder( GetFastH264DecoderOptions() );

    XIRef<XMemory> file = LoadFile( inputFileName );

    XRef<AVDeMuxer> deMuxer;

    bool timerStarted = false;
    uint64_t clockStart = 0;
    uint64_t clockStop = 0;

    bool done = false;

    while( !done )
    {
        if( !deMuxer.IsValid() )
            deMuxer = new AVDeMuxer( file );

        int videoStreamIndex = deMuxer->GetVideoStreamIndex();

        int streamIndex = -1;

        bool readFrame = deMuxer->ReadFrame( streamIndex );

        if( !readFrame )
        {
            deMuxer.Clear();
            continue;
        }

        if( streamIndex == videoStreamIndex )
        {
            if( deMuxer->IsKey() )
            {
                XIRef<XMemory> frame = deMuxer->GetFrame();

                decoder->Decode( frame );

                unsigned int wait = sleepMicros;

                if( timerStarted )
                {
                    clockStop = XMonoClock::GetTime();
                    unsigned int delta = (unsigned int)(XMonoClock::GetElapsedTime( clockStart, clockStop ) * 1000000);
                    wait -= (delta < sleepMicros) ? delta : sleepMicros;

                    x_usleep( wait );
                }

                timerStarted = true;
                clockStart = XMonoClock::GetTime();
            }
        }
    }

    Locky::UnregisterFFMPEG();

    return 0;
}

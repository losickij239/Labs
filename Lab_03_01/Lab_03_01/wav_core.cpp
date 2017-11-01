#include <cstdio>
#include <cstring>

#include "wav_header.h"
#include "wav_core.h"

// TODO: Remove all 'magic' numbers
// TODO: Make the code more secure. Get rid of pointers (after creating a class, of course).

WavData::WavData(const char *filename)
{
	CreateFromFile(filename);
}

void WavData::read_header(const char *filename)
{
    printf( ">>>> read_header( %s )\n", filename );
    null_header(); // Fill header with zeroes.

    FILE* f = fopen(filename, "rb");
    if ( !f ) {
		throw exceptions("IO_ERROR");
    }

    size_t blocks_read = fread( &header, sizeof(wav_header_s), 1, f);
    if ( blocks_read != 1 ) {
        // can't read header, because the file is too small.
		throw exceptions("BAD_FORMAT");
    }

    fseek( f, 0, SEEK_END ); // seek to the end of the file
    size_t file_size = ftell( f ); // current position is a file size!
    fclose( f );

	check_header(file_size);
}

void WavData::addToStr(std::string& in, std::string in2, unsigned long num)
{
	in += in2;
	in += std::to_string(num);
	in += "\n";
}

std::string WavData::GetDescription()
{
	std::string out;
	out += "-------------------------\n";
	addToStr(out, " audioFormat   ", header.audioFormat);
	addToStr(out, " numChannels   ", header.numChannels);
	addToStr(out, " sampleRate    ", header.sampleRate);
	addToStr(out, " bitsPerSample ", header.bitsPerSample);
	addToStr(out, " byteRate      ", header.byteRate);
	addToStr(out, " blockAlign    ", header.blockAlign);
	addToStr(out, " chunkSize     ", header.chunkSize);
	addToStr(out, " subchunk1Size ", header.subchunk1Size);
	addToStr(out, " subchunk2Size ", header.subchunk2Size);
	out += "-------------------------\n";
	return out;
}

void WavData::CreateFromFile( const char* filename )
{
    printf( ">>>> extract_data_int16( %s )\n", filename );
  
    read_header(filename);

    if ( header.bitsPerSample != 16 ) {
        // Only 16-bit samples is supported.
		throw exceptions("UNSUPPORTED_FORMAT");
    }

    FILE* f = fopen(filename, "rb");
    if ( !f ) {
		throw exceptions("IO_ERROR");
    }
    fseek( f, 44, SEEK_SET ); // Seek to the begining of PCM data.

    int chan_count = header.numChannels;
    int samples_per_chan = ( header.subchunk2Size / sizeof(short) ) / chan_count;

    // 1. Reading all PCM data from file to a single vector.
    std::vector<short> all_channels;
    all_channels.resize( chan_count * samples_per_chan );
    size_t read_bytes = fread( all_channels.data(), 1, header.subchunk2Size, f );
    if ( read_bytes != header.subchunk2Size ) {
        printf( "extract_data_int16() read only %zu of %u\n", read_bytes, header.subchunk2Size );
		throw exceptions("IO_ERROR");
    }
    fclose( f );

    // 2. Put all channels to its own vector.
    channels_data.resize( chan_count );
    for ( size_t ch = 0; ch < channels_data.size(); ch++ ) {
        channels_data[ ch ].resize( samples_per_chan );
    }

    for ( int ch = 0; ch < chan_count; ch++ ) {
        std::vector<short>& chdata = channels_data[ ch ];
        for ( size_t i = 0; i < samples_per_chan; i++ ) {
            chdata[ i ] = all_channels[ chan_count * i + ch ];
        }
    }

}


void WavData::check_header( size_t file_size_bytes )
{
    // Go to wav_header.h for details

    if ( header.chunkId[0] != 0x52 ||
         header.chunkId[1] != 0x49 ||
         header.chunkId[2] != 0x46 ||
         header.chunkId[3] != 0x46 )
    {
		throw exceptions("HEADER_RIFF_ERROR");
    }

    if ( header.chunkSize != file_size_bytes - 8 )
	{
		throw exceptions("HEADER_FILE_SIZE_ERROR");
    }

    if ( header.format[0] != 0x57 ||
         header.format[1] != 0x41 ||
         header.format[2] != 0x56 ||
         header.format[3] != 0x45 )
    {
		throw exceptions("HEADER_WAVE_ERROR");
    }

    if ( header.subchunk1Id[0] != 0x66 ||
         header.subchunk1Id[1] != 0x6d ||
         header.subchunk1Id[2] != 0x74 ||
         header.subchunk1Id[3] != 0x20 )
    {
		throw exceptions("HEADER_FMT_ERROR");
    }

    if ( header.audioFormat != 1 )
	{
		throw exceptions("HEADER_NOT_PCM");
    }

    if ( header.subchunk1Size != 16 )
	{
		throw exceptions("HEADER_SUBCHUNK1_ERROR");
    }

    if ( header.byteRate != header.sampleRate * header.numChannels * header.bitsPerSample/8 )
	{
		throw exceptions("HEADER_BYTES_RATE_ERROR");
    }

    if ( header.blockAlign != header.numChannels * header.bitsPerSample/8 )
	{
		throw exceptions("HEADER_BLOCK_ALIGN_ERROR");
    }

    if ( header.subchunk2Id[0] != 0x64 ||
         header.subchunk2Id[1] != 0x61 ||
         header.subchunk2Id[2] != 0x74 ||
         header.subchunk2Id[3] != 0x61 )
    {
		throw exceptions("HEADER_FMT_ERROR");
    }

    if ( header.subchunk2Size != file_size_bytes - 44 )
    {
		throw exceptions("HEADER_SUBCHUNK2_SIZE_ERROR");
    }
}

void WavData::prefill_header()
{
    // Go to wav_header.h for details

    header.chunkId[0] = 0x52;
    header.chunkId[1] = 0x49;
    header.chunkId[2] = 0x46;
    header.chunkId[3] = 0x46;

    header.format[0] = 0x57;
    header.format[1] = 0x41;
    header.format[2] = 0x56;
    header.format[3] = 0x45;

    header.subchunk1Id[0] = 0x66;
    header.subchunk1Id[1] = 0x6d;
    header.subchunk1Id[2] = 0x74;
    header.subchunk1Id[3] = 0x20;

    header.subchunk2Id[0] = 0x64;
    header.subchunk2Id[1] = 0x61;
    header.subchunk2Id[2] = 0x74;
    header.subchunk2Id[3] = 0x61;

    header.audioFormat   = 1;
    header.subchunk1Size = 16;
    header.bitsPerSample = 16;

}

void WavData::fill_header(int chan_count, int bits_per_sample, int sample_rate, int samples_count_per_chan)
{
    if ( bits_per_sample != 16 )
	{
		throw exceptions("UNSUPPORTED_FORMAT");
    }

    if ( chan_count < 1 ) 
	{
		throw exceptions("BAD_PARAMS");
    }
    prefill_header();

    int file_size_bytes = 44 + chan_count * (bits_per_sample/8) * samples_count_per_chan;

    header.sampleRate    = sample_rate;
    header.numChannels   = chan_count;
    header.bitsPerSample = 16;

    header.chunkSize     = file_size_bytes - 8;
    header.subchunk2Size = file_size_bytes - 44;

    header.byteRate      = header.sampleRate * header.numChannels * header.bitsPerSample/8;
    header.blockAlign    = header.numChannels * header.bitsPerSample/8;
}

void WavData::SaveToFile(const char* filename)
{
    printf( ">>>> SaveToFile( %s )\n", filename );

    int chan_count = (int)channels_data.size();

    if ( chan_count < 1 ) {
		throw exceptions("BAD_PARAMS");
    }

    int samples_count_per_chan = (int)channels_data[0].size();

    // Verify that all channels have the same number of samples.
    for ( size_t ch = 0; ch < chan_count; ch++ ) {
        if ( channels_data[ ch ].size() != (size_t) samples_count_per_chan ) {
			throw exceptions("BAD_PARAMS");
        }
    }

	int sample_rate = header.sampleRate;
    fill_header(chan_count, 16, sample_rate, samples_count_per_chan );

    std::vector<short> all_channels;
    all_channels.resize( chan_count * samples_count_per_chan );

    for ( int ch = 0; ch < chan_count; ch++ ) {
        const std::vector<short>& chdata = channels_data[ ch ];
        for ( size_t i = 0; i < samples_count_per_chan; i++ ) {
            all_channels[ chan_count * i + ch ] = chdata[ i ];
        }
    }

    FILE* f = fopen( filename, "wb" );
    fwrite( &header, sizeof(wav_header_s), 1, f );
    fwrite( all_channels.data(), sizeof(short), all_channels.size(), f );
    if ( !f ) {
		throw exceptions("IO_ERROR");
    }

    fclose( f );
}

void WavData::null_header()
{
    memset( &header, 0, sizeof(wav_header_s) );
}

void WavData::ConvertStereoToMono()
{
	std::vector< std::vector<short> > dest_mono;
    int chan_count = (int)channels_data.size();

    if ( chan_count != 2 ) {
		throw exceptions("BAD_PARAMS");
    }

    int samples_count_per_chan = (int)channels_data[0].size();

    // Verify that all channels have the same number of samples.
    for ( size_t ch = 0; ch < chan_count; ch++ ) {
        if ( channels_data[ ch ].size() != (size_t) samples_count_per_chan ) {
			throw exceptions("BAD_PARAMS");
        }
    }

    dest_mono.resize( 1 );
    std::vector<short>& mono = dest_mono[ 0 ];
    mono.resize( samples_count_per_chan );

    // Mono channel is an arithmetic mean of all (two) channels.
    for ( size_t i = 0; i < samples_count_per_chan; i++ ) {
        mono[ i ] = (channels_data[0][i] + channels_data[1][i] ) / 2;
    }

	channels_data = dest_mono;
}

void WavData::ApplyReverb(double delay_seconds, float decay)
{
    int chan_count = (int)channels_data.size();

    if ( chan_count < 1 ) {
		throw exceptions("BAD_PARAMS");
    }

    int samples_count_per_chan = (int)channels_data[0].size();

    // Verify that all channels have the same number of samples.
    for ( size_t ch = 0; ch < chan_count; ch++ ) {
        if (channels_data[ ch ].size() != (size_t) samples_count_per_chan ) {
			throw exceptions("BAD_PARAMS");
        }
    }

	int sample_rate = header.sampleRate;
    int delay_samples = (int)(delay_seconds * sample_rate);

    for ( size_t ch = 0; ch < chan_count; ch++ ) {
        std::vector<float> tmp;
        tmp.resize(channels_data[ch].size());

        // Convert signal from short to float
        for ( size_t i = 0; i < samples_count_per_chan; i++ ) {
            tmp[ i ] = channels_data[ ch ][ i ];
        }

        // Add a reverb
        for ( size_t i = 0; i < samples_count_per_chan - delay_samples; i++ ) {
            tmp[ i + delay_samples ] += decay * tmp[ i ];
        }

        // Find maximum signal's magnitude
        float max_magnitude = 0.0f;
        for ( size_t i = 0; i < samples_count_per_chan - delay_samples; i++ ) {
            if ( abs(tmp[ i ]) > max_magnitude ) {
                max_magnitude = abs(tmp[ i ]);
            }
        }

        // Signed short can keep values from -32768 to +32767,
        // After reverb, usually there are values large 32000.
        // So we must scale all values back to [ -32768 ... 32768 ]
        float norm_coef = 30000.0f / max_magnitude;
        printf( "max_magnitude = %.1f, coef = %.3f\n", max_magnitude, norm_coef );

        // Scale back and transform floats to shorts.
        for ( size_t i = 0; i < samples_count_per_chan; i++ ) {
			channels_data[ ch ][ i ] = (short)(norm_coef * tmp[ i ]);
        }
    }
}

int WavData::GetSampleRate()
{
	return header.sampleRate;
}

int WavData::GetChanCount()
{
	return header.numChannels;
}

bool WavData::IsStereo()
{
	return (header.numChannels == 2);
}
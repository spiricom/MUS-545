#include <stdio.h> //need these to use the file system stuff
#include <string.h> //need these to use the file system stuff
#include "daisy_pod.h"
#include "leaf.h"

///THIS IS AN EXAMPLE FOR LOADING SAMPLES FROM AN SD CARD
// IT'S PRETTY COMPLEX. THERE ARE TWO GIANT FUNCTIONS BEFORE AUDIOCALLBACK AND MAIN THAT HANDLE READING THE WAVES. 
// JUST LEAVE THOSE AS THEY ARE, AND IF YOU ARE 
// MAKING YOUR OWN THING, JUST EDIT AUDIO CALLBACK AND MAIN.

using namespace daisy;
LEAF leaf;
char leafMemory[455355];
//for more recording space, we need to use the external 64 MB SDRAM chip instead of the internal memory
//to do that, set up a new variable that is the size of the memory amount you want to use
//and use the "DSY_SDRAM_BSS" macro after defining the variable (but before the semicolon) to place this array in the SDRAM memory
float bigMemory[8388608] DSY_SDRAM_BSS; //64 MB is 64 * 1024 * 1024 = 67108864 / 4 (because floats take up 4 bytes and tBuffer wants floats)
char bigMemoryScratch[33554432] DSY_SDRAM_BSS; 
#define MAX_WAV_FILES 32 //you could increase this - it's the number of wav files it looks for 

//now we need to make a LEAF "memorypool" object that will organize that large memory chunk
tMempool bigPool;
DaisyPod hw;
SdmmcHandler   sdcard;
FatFSInterface fsi;
tBuffer myWaves[MAX_WAV_FILES];
tSampler mySamplers[MAX_WAV_FILES];

//this is a "struct", defining a structure to hold the data for each wav file as we convert it to a floating point buffer
struct tWaveHeader {

    unsigned char riff[4];                      // RIFF string

    unsigned int overall_size;               // overall size of file in bytes

    unsigned char wave[4];                      // WAVE string

    unsigned char fmt_chunk_marker[4];          // fmt string with trailing null char

    unsigned int length_of_fmt;                 // length of the format data

    unsigned int format_type;                   // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law

    unsigned int channels;                      // no.of channels

    unsigned int sample_rate;                   // sampling rate (blocks per second)

    unsigned int byterate;                      // SampleRate * NumChannels * BitsPerSample/8

    unsigned int block_align;                   // NumChannels * BitsPerSample/8

    unsigned int bits_per_sample;               // bits per sample, 8- 8bits, 16- 16 bits etc

    unsigned char data_chunk_header[4];        // DATA string or FLLR string

    unsigned int data_size;                     // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read

};


//stuff for file system navigation and reading SD Card
FRESULT result = FR_OK;
FILINFO fno;
DIR     dir;
char *  fn;

WavFileInfo             file_info_[MAX_WAV_FILES];
size_t                  file_cnt_, file_sel_;
FIL                     fil_;

//this just stores information about the wav files to pass to the buffer
int waves[MAX_WAV_FILES][4];

//these are temp buffers for the wav reading process
unsigned char buffer4[4];
unsigned char buffer2[2];

//an instance of the tWaveHeader struct
struct tWaveHeader header;

uint32_t waveTimeout = 2048;
uint32_t numberOfGarbage = 0;
uint32_t memoryPointer = 0;
uint32_t scratchPosition = 0;




int32_t readWave(FIL *ptr,  float * largeMemory, char * largeMemoryScratch, uint32_t largeMemSize, uint32_t scratchSize, uint32_t * numChannels, uint32_t * sampleRate) {

 int read = 0;
 UINT numBytesRead = 0;

 // read header parts

 read = f_read(ptr, header.riff, sizeof(header.riff), &numBytesRead);
 //printf("(1-4): %s \n", header.riff);

 if ((header.riff[0] != 'R') && (header.riff[1] != 'I') && (header.riff[2] != 'F') && (header.riff[3] != 'F'))
 {
	 return -1;
 }
 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
 //printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 // convert little endian to big endian 4 byte int
 header.overall_size  = buffer4[0] |
						(buffer4[1]<<8) |
						(buffer4[2]<<16) |
						(buffer4[3]<<24);

 //printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size, header.overall_size/1024);

 read = f_read(ptr, header.wave, sizeof(header.wave), &numBytesRead);
 //printf("(9-12) Wave marker: %s\n", header.wave);

read = f_read(ptr, header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), &numBytesRead);
int numIterations = 0;

while ((header.fmt_chunk_marker[0] != 102) && (header.fmt_chunk_marker[1] != 109) && (header.fmt_chunk_marker[2] != 116) && (header.fmt_chunk_marker[3] != 32))
{
	 //printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

	 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
	// printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	 // convert little endian to big endian 4 byte integer
	 numberOfGarbage = buffer4[0] |
								(buffer4[1] << 8) |
								(buffer4[2] << 16) |
								(buffer4[3] << 24);
	 //printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);


	 read = f_read(ptr, largeMemoryScratch, numberOfGarbage, &numBytesRead);

	 read = f_read(ptr, header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), &numBytesRead);
	 numIterations++;
	 if (numIterations > waveTimeout)
	 {
		 return -2;
	 }

}

 //printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
// printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 // convert little endian to big endian 4 byte integer
 header.length_of_fmt = buffer4[0] |
							(buffer4[1] << 8) |
							(buffer4[2] << 16) |
							(buffer4[3] << 24);
 //printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

 read = f_read(ptr, buffer2, sizeof(buffer2), &numBytesRead);
 //printf("%u %u \n", buffer2[0], buffer2[1]);

 header.format_type = buffer2[0] | (buffer2[1] << 8);
 char format_name[10] = "";
 if (header.format_type == 1)
   strcpy(format_name,"PCM");
 else if (header.format_type == 6)
  strcpy(format_name, "A-law");
 else if (header.format_type == 7)
  strcpy(format_name, "Mu-law");

 //printf("(21-22) Format type: %u %s \n", header.format_type, format_name);

 read = f_read(ptr, buffer2, sizeof(buffer2), &numBytesRead);
 //printf("%u %u \n", buffer2[0], buffer2[1]);

 header.channels = buffer2[0] | (buffer2[1] << 8);
 //printf("(23-24) Channels: %u \n", header.channels);
 numChannels[0] = (uint32_t) header.channels;

 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
 //printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 header.sample_rate = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);

 //printf("(25-28) Sample rate: %u\n", header.sample_rate);
 sampleRate[0] = (uint32_t)header.sample_rate;
 
 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
 //printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 header.byterate  = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
 //printf("(29-32) Byte Rate: %u , Bit Rate:%u\n", header.byterate, header.byterate*8);

 read = f_read(ptr, buffer2, sizeof(buffer2), &numBytesRead);
 //printf("%u %u \n", buffer2[0], buffer2[1]);

 header.block_align = buffer2[0] |
					(buffer2[1] << 8);
 //printf("(33-34) Block Alignment: %u \n", header.block_align);

 read = f_read(ptr, buffer2, sizeof(buffer2), &numBytesRead);
 //printf("%u %u \n", buffer2[0], buffer2[1]);

 header.bits_per_sample = buffer2[0] |
					(buffer2[1] << 8);
 //printf("(35-36) Bits per sample: %u \n", header.bits_per_sample);

 read = f_read(ptr, header.data_chunk_header, sizeof(header.data_chunk_header), &numBytesRead);
 //printf("(37-40) Data Marker: %s \n", header.data_chunk_header);

 numIterations = 0;

 while ((header.data_chunk_header[0] != 'd') && (header.data_chunk_header[1] != 'a') && (header.data_chunk_header[2] != 't') && (header.data_chunk_header[3] != 'a'))
 {
 	 //printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

 	 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
 	// printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 	 // convert little endian to big endian 4 byte integer
 	 numberOfGarbage = buffer4[0] |
 								(buffer4[1] << 8) |
 								(buffer4[2] << 16) |
 								(buffer4[3] << 24);
 	 //printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

/*
 	 read = f_read(ptr, garbageBuffer, numberOfGarbage, &numBytesRead);
*/
 	 //move pointer ahead by that size
 	 read = f_read(ptr, largeMemoryScratch, numberOfGarbage, &numBytesRead);

 	 read = f_read(ptr, header.data_chunk_header, sizeof(header.data_chunk_header), &numBytesRead);
 	 numIterations++;
	 if (numIterations > waveTimeout)
	 {
		 return -3;
	 }
 }


 read = f_read(ptr, buffer4, sizeof(buffer4), &numBytesRead);
// printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 header.data_size = buffer4[0] |
				(buffer4[1] << 8) |
				(buffer4[2] << 16) |
				(buffer4[3] << 24 );
 //printf("(41-44) Size of data chunk: %u \n", header.data_size);


 // calculate no.of samples
 long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
 //printf("Number of samples:%lu \n", num_samples);

 long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
 //printf("Size of each sample:%ld bytes\n", size_of_each_sample);

 // calculate duration of file
 float duration_in_seconds = (float) header.overall_size / header.byterate;
 //printf("Approx.Duration in seconds=%f\n", duration_in_seconds);
 //printf("Approx.Duration in h:m:s=%s\n", seconds_to_time(duration_in_seconds));



 // read each sample from data chunk if PCM
 if (header.format_type == 1)
 { // PCM

	long i =0;
	int  size_is_correct = 1;

	// make sure that the bytes-per-sample is completely divisible by num.of channels
	long bytes_in_each_channel = (size_of_each_sample / header.channels);
	if ((bytes_in_each_channel  * header.channels) != size_of_each_sample)
	{
		//printf("Error: %ld x %ud <> %ld\n", bytes_in_each_channel, header.channels, size_of_each_sample);
		size_is_correct = 0;
	}

	if (size_is_correct)
	{
				// the valid amplitude range for values based on the bits per sample
		long low_limit = 0l;
		long high_limit = 0l;
		float inv_high_limit = 1.0f;

		switch (header.bits_per_sample) {
			case 8:
				low_limit = -128;
				high_limit = 127;

				break;
			case 16:
				low_limit = -32768;
				high_limit = 32767;
				break;
			case 24: //packed left like a 32 bit
				low_limit = -2147483648;
				high_limit = 2147483647;
				break;
			case 32:
				low_limit = -2147483648;
				high_limit = 2147483647;
				break;
		}
		inv_high_limit = 1.0f / high_limit;

		if ((memoryPointer + (num_samples*header.channels)) >= largeMemSize)
		{
			//ran out of space in SDRAM
			//OutOfSpace = 1;
			return -4;
		}

		if (header.data_size < scratchSize)
		{
			read = f_read(ptr, largeMemoryScratch, header.data_size, &numBytesRead);
			//printf("\n\n.Valid range for data values : %ld to %ld \n", low_limit, high_limit);
			scratchPosition = 0;
			if (bytes_in_each_channel == 4)
			{
				for (i =1; i <= num_samples; i++)
				{
					// dump the data read
					unsigned int  xchannels = 0;

					int32_t data_in_channel_32 = 0;
					float float_data = 0.0f;

					for (xchannels = 0; xchannels < header.channels; xchannels ++ )
					{
						//printf("Channel#%d : ", (xchannels+1));
						// convert data from little endian to big endian based on bytes in each channel sample
						unsigned int byteOffset =  xchannels * bytes_in_each_channel;

						data_in_channel_32 =	largeMemoryScratch[scratchPosition] |
											(largeMemoryScratch[scratchPosition + 1]<<8) |
											(largeMemoryScratch[scratchPosition + 2]<<16) |
											(largeMemoryScratch[scratchPosition + 3]<<24);
						float_data = ((float)data_in_channel_32) * inv_high_limit;
						scratchPosition = scratchPosition + 4;

						largeMemory[memoryPointer] = float_data;
						memoryPointer++;

					}
				}
			}
			else if (bytes_in_each_channel == 3)
			{
				for (i =1; i <= num_samples; i++)
				{
					// dump the data read
					unsigned int  xchannels = 0;

					int32_t data_in_channel_32 = 0;
					float float_data = 0.0f;

					for (xchannels = 0; xchannels < header.channels; xchannels ++ )
					{
						//printf("Channel#%d : ", (xchannels+1));
						// convert data from little endian to big endian based on bytes in each channel sample

						data_in_channel_32 =	largeMemoryScratch[scratchPosition]<<8 |
												(largeMemoryScratch[scratchPosition + 1]<<16) |
												(largeMemoryScratch[scratchPosition + 2]<<24);
						float_data = ((float)data_in_channel_32) * inv_high_limit;
						scratchPosition = scratchPosition + 3;

						largeMemory[memoryPointer] = float_data;
						memoryPointer++;
					}
				}
			}
			else if (bytes_in_each_channel == 2)
			{
				for (i =1; i <= num_samples; i++)
				{
					// dump the data read
					unsigned int  xchannels = 0;

					int16_t data_in_channel_16 = 0;
					float float_data = 0.0f;

					for (xchannels = 0; xchannels < header.channels; xchannels ++ )
					{
						//printf("Channel#%d : ", (xchannels+1));
						// convert data from little endian to big endian based on bytes in each channel sample

						data_in_channel_16 = (int16_t)(largeMemoryScratch[scratchPosition] |
																		(largeMemoryScratch[scratchPosition + 1] << 8));
						float_data = ((float)data_in_channel_16) * inv_high_limit;
						scratchPosition = scratchPosition + 2;

						largeMemory[memoryPointer] = float_data;
						memoryPointer++;
					}
				}
			}
			else if (bytes_in_each_channel == 1)
			{
				for (i =1; i <= num_samples; i++)
				{
					// dump the data read
					unsigned int  xchannels = 0;

					int8_t data_in_channel_8 = 0;
					float float_data = 0.0f;

					for (xchannels = 0; xchannels < header.channels; xchannels ++ )
					{
						//printf("Channel#%d : ", (xchannels+1));
						// convert data from little endian to big endian based on bytes in each channel sample

						data_in_channel_8 = (int8_t)largeMemoryScratch[scratchPosition];
						float_data = ((float)data_in_channel_8) * inv_high_limit;
						scratchPosition = scratchPosition + 1;

						largeMemory[memoryPointer] = float_data;
						memoryPointer++;
					}
				}
			}

		}
		else
		{
			//tooBigForScratch = 1;
			return -5;
		}
		return memoryPointer;
	 }
 }

 return -6;

}

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


//this is the function to read the sd card and get info on the wav files
void readSDCardFiles(const char *search_path)
{

    // Open Dir and scan for files.
    if(f_opendir(&dir, search_path) != FR_OK)
    {
        return;
    }
    do
    {
        result = f_readdir(&dir, &fno);
        // Exit if bad read or NULL fname
        if(result != FR_OK || fno.fname[0] == 0)
            break;
        // Skip if its a directory or a hidden file.
        if(fno.fattrib & (AM_HID | AM_DIR))
            continue;
        // Now we'll check if its .wav and add to the list.
        fn = fno.fname;
        if(file_cnt_ < MAX_WAV_FILES - 1)
        {
            if(strstr(fn, ".wav") || strstr(fn, ".WAV"))
            {
                strcpy(file_info_[file_cnt_].name, search_path);
                strcat(file_info_[file_cnt_].name, fn);
                file_cnt_++;
                // For now lets break anyway to test.
                //                break;
            }
        }
        else
        {
            break;
        }
    } while(result == FR_OK);
    f_closedir(&dir);
    // Now we'll go through each file and load the WavInfo.
    for(size_t i = 0; i < file_cnt_; i++)
    {
        size_t bytesread;
        if(f_open(&fil_, file_info_[i].name, (FA_OPEN_EXISTING | FA_READ))
           == FR_OK)
        {
            // Populate the WAV Info
            if(f_read(&fil_,
                      (void *)&file_info_[i].raw_data,
                      sizeof(WAV_FormatTypeDef),
                      &bytesread)
               != FR_OK)
            {
                // Maybe add return type
                return;
            }
            f_close(&fil_);
        }
    }

}
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //get the next sample from the left channel of the input (right channel would be in[1][i])
        float mySample = in[0][i];

		//pitch shift the samples based on the knobs
		tSampler_setRate(&mySamplers[0], hw.knob1.Value() * 2.0f);
		tSampler_setRate(&mySamplers[1], hw.knob2.Value() * 2.0f);

		if (hw.button1.RisingEdge())
		{
			tSampler_play(&mySamplers[0]);
		}
		if (hw.button1.FallingEdge())
		{
			tSampler_stop(&mySamplers[0]);
		}
		if (hw.button2.RisingEdge())
		{
			tSampler_play(&mySamplers[1]);
		}
		if (hw.button2.FallingEdge())
		{
			tSampler_stop(&mySamplers[1]);
		}
		mySample = tSampler_tick(&mySamplers[0]) + tSampler_tick(&mySamplers[1]);

        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();

    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsi.GetSDFileSystem(), "/", 1);


    LEAF_init(&leaf, 48000, leafMemory, 455355, randomNumber); 

    int32_t prevMemoryPointer = 0;
	readSDCardFiles(fsi.GetSDPath());

    for (unsigned int i = 0; i< file_cnt_; i++)
	{
        
        file_sel_ = i;
    
        f_open(&fil_, file_info_[file_sel_].name, (FA_OPEN_EXISTING | FA_READ));
        uint32_t numChannels = 0;
        uint32_t sampleRate = 0;

        int32_t result = readWave(&fil_, bigMemory, bigMemoryScratch, 8388608, 33554432, &numChannels, &sampleRate);
        if (result > 0)
        {
            waves[i][0] = prevMemoryPointer; //save start point in memory for this wav file
            waves[i][1] = numChannels;
            waves[i][2] = sampleRate;
            waves[i][3] = result - prevMemoryPointer; //calculate length
            prevMemoryPointer = result; //store this start point for future length calculation of next wav
        }
        f_close(&fil_);
        tBuffer_init(&myWaves[i], 1, &leaf);
		if (file_cnt_ > i)
		{
			tBuffer_setBuffer(&myWaves[i], &bigMemory[waves[i][0]], waves[i][3], waves[i][1], waves[i][2]);
		}
		tSampler_init(&mySamplers[i], &myWaves[i], &leaf);
		tSampler_setMode(&mySamplers[i], PlayNormal);
	}

    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



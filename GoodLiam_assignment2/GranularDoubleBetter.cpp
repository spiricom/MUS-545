
#include "daisy_pod.h"
#include "leaf.h"

using namespace daisy;

LEAF leaf;
char leafMemory[455355];

//for more recording space, we need to use the external 64 MB SDRAM chip instead of the internal memory
//to do that, set up a new variable that is the size of the memory amount you want to use
//and use the "DSY_SDRAM_BSS" macro after defining the variable (but before the semicolon) to place this array in the SDRAM memory
char bigMemory[67108864] DSY_SDRAM_BSS; //64 MB is 64 * 1024 * 1024 = 67108864

//now we need to make a LEAF "memorypool" object that will organize that large memory chunk
tMempool bigPool;
DaisyPod hw;

//we need a buffer to store the sample (just an array that holds the sample values)
tBuffer  myBuffer;
tSampler mySampler;
tADSRS   myEnvelope;
tSampler mySampler2;
tADSRS   myEnvelope2;

int bufferSize   = 0; // store how long the sample is
int grainSize      = 8000;
int grainSize2     = 8000;
int grainPosition  = 0;
int grainPosition2 = 0;

int currentGrainSize = grainSize;

float playbackRate     = 1.0;
float crossfadePercent = 0.0;
float crossfadeLength  = 0.0;

bool isFading1 = 0;
bool isFading2 = 0;

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    //now here's the per sample stuff
    for(size_t i = 0; i < size; i++)
    {
        //get the next sample from the left channel of the input (right channel would be in[1][i])
        float myInputSample = in[0][i];

        if(int freqShift = hw.encoder.Increment())
        {
            playbackRate += 0.02 * freqShift;
            playbackRate = LEAF_clip(0.1, playbackRate, 4.0);

            tSampler_setRate(&mySampler, playbackRate);
            tSampler_setRate(&mySampler2, playbackRate);
        }

        crossfadePercent = hw.knob1.Value() / 2.0;

        float newGrainSizeRaw
            = (hw.knob2.Value() * 0.98 + 0.02) * (bufferSize / 2);
        float newGrainSize = newGrainSizeRaw * playbackRate;
        if(newGrainSize > bufferSize)
            newGrainSize = bufferSize;

        if(hw.button1.RisingEdge())
        {
            tBuffer_setRecordPosition(&myBuffer, 0);
            tBuffer_record(&myBuffer);
        }
        else if(hw.button1.FallingEdge())
        {
            tBuffer_stop(&myBuffer);
            bufferSize = tBuffer_getRecordPosition(&myBuffer);
        }

        int correction = 100 * playbackRate;

        if(!isFading1
           && mySampler->idx
                  >= mySampler->end - grainSize * crossfadePercent - correction)
        {
            tADSRS_off(&myEnvelope);
            isFading1 = true;

            grainSize2 = newGrainSize;
            grainPosition2
                = fmodf(randomNumber(), 1.0) * (bufferSize - grainSize2);

            tSampler_setStart(&mySampler2, grainPosition2);
            tSampler_setLength(&mySampler2, grainSize2);

            float crossfadeLength = (crossfadePercent * grainSize2) / 48.;
            tADSRS_setAttack(&myEnvelope2, crossfadeLength);
            tADSRS_setRelease(&myEnvelope2, crossfadeLength);

            tSampler_play(&mySampler2);
            tADSRS_on(&myEnvelope2, 1.0f);

            isFading2 = false;

            hw.led2.Set(randomNumber(), randomNumber(), randomNumber());
        }

        if(!isFading2
           && mySampler2->idx >= mySampler2->end - grainSize2 * crossfadePercent
                                     - correction)
        {
            tADSRS_off(&myEnvelope2);
            isFading2 = true;

            grainSize = newGrainSize;
            grainPosition
                = fmodf(randomNumber(), 1.0) * (bufferSize - grainSize);

            tSampler_setStart(&mySampler, grainPosition);
            tSampler_setLength(&mySampler, grainSize);

            float crossfadeLength = (crossfadePercent * grainSize) / 48.;
            tADSRS_setAttack(&myEnvelope, crossfadeLength);
            tADSRS_setRelease(&myEnvelope, crossfadeLength);

            tSampler_play(&mySampler);
            tADSRS_on(&myEnvelope, 1.0f);

            isFading1 = false;

            hw.led1.Set(randomNumber(), randomNumber(), randomNumber());
        }

        // button 2 manually triggers a new grain
        if(hw.button2.RisingEdge())
        {
            tSampler_setStart(&mySampler, 0);
            tSampler_setLength(&mySampler, grainSize);

            tADSRS_setAttack(&myEnvelope, crossfadeLength);
            tADSRS_setRelease(&myEnvelope, crossfadeLength);
            tSampler_play(&mySampler);
            tADSRS_on(&myEnvelope, 1.0f);
            hw.led1.Set(1.0, 0, 0);
            isFading1 = false;
        }

        hw.UpdateLeds();

        //now tick the buffer with the input coming in so the buffer gets filled
        tBuffer_tick(&myBuffer, myInputSample);

        //then tick the sampler so it reads the buffer and sets mySample to the output value
        float mySample1 = tSampler_tick(&mySampler) * tADSRS_tick(&myEnvelope);
        float mySample2
            = tSampler_tick(&mySampler2) * tADSRS_tick(&myEnvelope2);

        // float mySample = (mySample1 + mySample2) / 2.0f;

        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = mySample1;
        out[1][i] = mySample2;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();
    LEAF_init(&leaf, 48000, leafMemory, 455355, randomNumber);

    //first, initialize that large memory pool to be ready for usage
    tMempool_init(&bigPool, bigMemory, 67108864, &leaf);
    //then place the buffer into that memory pool instead of the default pool, using the "initToPool" function instead of "init"
    tBuffer_initToPool(
        &myBuffer,
        480000,
        &bigPool); //five-second buffer, set up a segment of memory to store it

    //the tSampler object doesn't need to go into the large pool, it's just a small thing that references the data in the buffer
    tSampler_init(
        &mySampler,
        &myBuffer,
        &leaf); //make a "sampler" object that uses that buffer for memory
    tSampler_setMode(
        &mySampler,
        PlayNormal); //other options are Play Normal (no looping), and PlayBackAndForth

    tADSRS_init(&myEnvelope, 0, 0, 1.0, 0, &leaf);

    tSampler_init(
        &mySampler2,
        &myBuffer,
        &leaf); //make a "sampler" object that uses that buffer for memory
    tSampler_setMode(
        &mySampler2,
        PlayNormal); //other options are Play Normal (no looping), and PlayBackAndForth

    tADSRS_init(&myEnvelope2, 0, 0, 1.0, 0, &leaf);


    hw.StartAudio(AudioCallback);


    while(1)
    {
        hw.UpdateLeds();
    }
}

#include "daisy_pod.h"
#include "leaf.h"

using namespace daisy;

/*  
    "we have
            MUTABLE INSTRUMENTS CLOUDS
                                      at home!"
                                      
    Author: Liam Esparraguera
    Controls:
        L Button - manually trigger grain
        L Knob   - grain crossfade
        R Knob   - grain size
        Encoder  - pitch shift
*/

LEAF leaf;
char leafMemory[455355];

char bigMemory[67108864] DSY_SDRAM_BSS; //64 MB is 64 * 1024 * 1024 = 67108864

tMempool bigPool;
DaisyPod hw;

tBuffer  myBuffer;
tSampler mySampler;
tADSRS   myEnvelope;
tSampler mySampler2;
tADSRS   myEnvelope2;

// size of looping recording buffer, 5 sec
int bufferSize = 240000;

// how long is the current grain, in samples
int grainSize  = 8000;
int grainSize2 = 8000;
// at which sample did the last grain start?
int grainPosition  = 0;
int grainPosition2 = 0;

float playbackRate = 1.0; // playback rate of sampler

// how long, relative to each grain, is the envelope's attack/release? max is 0.5 = triangle envelope.
float crossfadePercent = 0.0;

// are we currently in the release portion of the envelope?
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
            = (hw.knob2.Value() * 0.98 + 0.02) * bufferSize * 0.5;

        float newGrainSize = newGrainSizeRaw * playbackRate;
        // clamp grain size to sample length
        if(newGrainSize > bufferSize)
            newGrainSize = bufferSize;

        int correction = 100 * playbackRate;

        // if we have hit fade out for grain 1, play grain 2
        if(!isFading1
           && mySampler->idx
                  >= mySampler->end - grainSize * crossfadePercent - correction)
        {
            tADSRS_off(&myEnvelope);
            isFading1 = true;

            // randomize grain position but make sure it can play all the way through
            grainSize2 = newGrainSize;
            grainPosition2
                = fmodf(randomNumber(), 1.0) * (bufferSize - grainSize2);

            tSampler_setStart(&mySampler2, grainPosition2);
            tSampler_setEndUnsafe(&mySampler2, mySampler2->start + grainSize2);

            float crossfadeLength = (crossfadePercent * grainSize2) / 48.;
            tADSRS_setAttack(&myEnvelope2, crossfadeLength);
            tADSRS_setRelease(&myEnvelope2, crossfadeLength);

            tSampler_play(&mySampler2);
            tADSRS_on(&myEnvelope2, 1.0f);

            isFading2 = false;

            hw.led2.Set(randomNumber(), randomNumber(), randomNumber());
        }

        // if we have hit fade out for grain 2, play grain 1
        if(!isFading2
           && mySampler2->idx >= mySampler2->end - grainSize2 * crossfadePercent
                                     - correction)
        {
            tADSRS_off(&myEnvelope2);
            isFading2 = true;

            // randomize grain position but make sure it can play all the way through
            grainSize = newGrainSize;
            grainPosition
                = fmodf(randomNumber(), 1.0) * (bufferSize - grainSize);

            tSampler_setStart(&mySampler, grainPosition);
            tSampler_setEndUnsafe(&mySampler, mySampler->start + grainSize);

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

            float crossfadeLength = (crossfadePercent * grainSize) / 48.;

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
        bufferSize,
        &bigPool); //five-second buffer, set up a segment of memory to store it
    tBuffer_setRecordMode(&myBuffer, RecordLoop);

    //make a "sampler" object that uses that buffer for memory
    tSampler_init(&mySampler, &myBuffer, &leaf);
    tSampler_setMode(&mySampler, PlayNormal);

    tADSRS_init(&myEnvelope, 0, 0, 1.0, 0, &leaf);

    tSampler_init(&mySampler2, &myBuffer, &leaf);
    tSampler_setMode(&mySampler2, PlayNormal);

    tADSRS_init(&myEnvelope2, 0, 0, 1.0, 0, &leaf);

    tBuffer_setRecordPosition(&myBuffer, 0);
    tBuffer_record(&myBuffer);

    hw.StartAudio(AudioCallback);

    while(1)
    {
        hw.UpdateLeds();
    }
}

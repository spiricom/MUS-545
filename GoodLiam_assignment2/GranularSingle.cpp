
#include "daisy_pod.h"
#include "leaf.h"

using namespace daisy;

LEAF leaf;
char leafMemory[455355];

//for more recording space, we need to use the external 64 MB SDRAM chip instead of the internal memory
//to do that, set up a new variable that is the size of the memory amount you want to use
//and use the "DSY_SDRAM_BSS" macro after defining the variable (but before the semicolon) to place this array in the SDRAM memory
char bigMemory[67108864] DSY_SDRAM_BSS; //64 MB is 64 * 1024 * 1024 = 67108864

tMempool bigPool;
DaisyPod hw;

tBuffer  myBuffer;
tSampler mySampler;
tADSRS   myEnvelope;

int bufferSize  = 1;    // how long is the sample, in samples
int grainSize     = 8000; // how long is the next grain, in samples
int grainPosition = 0;    // at which sample did the last grain start?
int currentGrainSize
    = grainSize; // how long is the currently-played grain, in samples?

float playbackRate = 1.0; // playback rate of sampler
float crossfadePercent
    = 0.0; // how long, relative to each grain, is the envelope's attack/release? max is 0.5 = triangle envelope.
float crossfadeLength
    = 0.0; // how long are the grain's attack and release, in milliseconds

bool isFading
    = false; // are we currently in the release portion of the envelope?

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
        float mySample = in[0][i];

        float grainSizeRaw
            = (hw.knob2.Value() * 0.98 + 0.02) * (bufferSize / 2);

        crossfadePercent = hw.knob1.Value() / 2.0;
        crossfadeLength  = (crossfadePercent * grainSize) / 48.;

        if(int freqShift = hw.encoder.Increment())
        {
            playbackRate += 0.02 * freqShift;
            playbackRate = LEAF_clip(0.1, playbackRate, 4.0);

            tSampler_setRate(&mySampler, playbackRate);
        }

        grainSize = grainSizeRaw * playbackRate;
        // clamp grain size to sample length
        if(grainSize > bufferSize)
            grainSize = bufferSize;


        // RECORDING: record while button 1 is pressed.
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

        // if we have hit fade out for grain, turn off / release ADSR envelope
        if(!isFading
           && mySampler->idx
                  >= mySampler->end - currentGrainSize * crossfadePercent)
        {
            isFading = true;
            tADSRS_off(&myEnvelope);
            hw.led1.Set(0.0, 0, 1.0);
        }

        // if we have finished playing the grain, play a new one.
        if(mySampler->idx
           >= mySampler->end
                  - 100 * playbackRate) // magic number. no idea why it works, seems like sampler ends early.
        {
            float randomCoefficient = fmodf(randomNumber(), 1.0);
            currentGrainSize        = grainSize;

            // randomize grain position but make sure it can play all the way through
            grainPosition
                = randomCoefficient * 0.99 * (bufferSize - grainSize);

            tSampler_setStart(&mySampler, grainPosition);
            tSampler_setLength(&mySampler, grainSize);

            tADSRS_setAttack(&myEnvelope, crossfadeLength);
            tADSRS_setRelease(&myEnvelope, crossfadeLength);

            tSampler_play(&mySampler);

            tADSRS_on(&myEnvelope, 1.0f);

            // set led 1 to red, signaling grain attack and sustain
            hw.led1.Set(1.0, 0, 0);
            isFading = false;

            // change led 2 to random color for new grain
            hw.led2.Set(randomNumber(), randomNumber(), randomNumber());
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
            isFading = false;
        }

        // tick the buffer with the input coming in so the buffer gets filled
        tBuffer_tick(&myBuffer, mySample);

        // tick the sampler so it reads the buffer and sets mySample to the output value
        mySample = tSampler_tick(&mySampler) * tADSRS_tick(&myEnvelope);

        hw.UpdateLeds();

        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = mySample;
        out[1][i] = mySample;
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

    // first, initialize that large memory pool to be ready for usage
    tMempool_init(&bigPool, bigMemory, 67108864, &leaf);

    // then place the buffer into that memory pool instead of the default pool, using the "initToPool" function instead of "init"
    tBuffer_initToPool(
        &myBuffer,
        480000,
        &bigPool); //five-second buffer, set up a segment of memory to store it

    // the tSampler object doesn't need to go into the large pool, it's just a small thing that references the data in the buffer
    tSampler_init(
        &mySampler,
        &myBuffer,
        &leaf);
    tSampler_setMode(
        &mySampler,
        PlayNormal);

    tADSRS_init(&myEnvelope, 0, 0, 1.0, 0, &leaf);

    hw.StartAudio(AudioCallback);

    while(1) {}
}

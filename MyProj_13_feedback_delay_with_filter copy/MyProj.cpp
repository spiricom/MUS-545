#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[400000]; //notice I increased this, because tRetune needs more memory
DaisyPod hw;

tSimpleRetune mySimpleRetune;

// A PITCH SHIFTER EXAMPLE
// this example takes audio into the left line in channel.
// It pitch shifts that audio by a factor from 0-2 (with 2 being up an octave and 0 being way pitched down to stopped - it's a ratio)
// this uses the tRetune object. tRetune needs a little more memory and a larger block size so I increased the memory in two places (I noted it in comments) and increased the block size in the setAudioBlockSize function call.


float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //get the next sample from the left channel of the input (right channel would be in[1][i])
        float mySample = in[0][i];

        tSimpleRetune_tuneVoice(&mySimpleRetune, 0, 2.0f * hw.knob1.Value());
        mySample = tSimpleRetune_tick(&mySimpleRetune, mySample);


        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(64); // I also had to increase this to make retune work - 4 samples didn't give it enough time because it has some "bursty" processing
    //use a power of two size for the block on the Daisy and the hardware will be happier (2, 4, 8, 16, 32, 64, 128, 256, 512)
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber); //notice I increased this, because tRetune needs more memory
    tSimpleRetune_init(&mySimpleRetune, 1, mtof(42.0f), mtof(84.0f), 1024, &leaf);

    hw.StartAudio(AudioCallback);


    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



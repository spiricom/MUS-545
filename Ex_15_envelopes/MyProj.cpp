#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[400000];
DaisyPod hw;

tADSRS env;
tCycle mySine;

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

        tCycle_setFreq(&mySine, 220.0f);
        
        if (hw.button1.RisingEdge())
        {
            tADSRS_on(&env, 1.0f);
        }

        mySample = tCycle_tick(&mySine);
        mySample = mySample * tADSRS_tick(&env);
        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(64); 
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber); 
    //arguments are attack time in milliseconds, decay time in ms, sustain level (0-1), and release time in ms)
    tADSRS_init(&env, 10, 500, 0., 500, &leaf);
    tCycle_init(&mySine, &leaf);

    hw.StartAudio(AudioCallback);


    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



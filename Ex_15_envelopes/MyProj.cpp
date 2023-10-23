#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[400000]; //notice I increased this, because tRetune needs more memory
DaisyPod hw;

tADSR env;
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
            tADSR_on(&env, 1.0f);
        }

        mySample = tCycle_tick(&mySine);
        mySample = mySample * tADSR_tick(&env);
        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(64); // I also had to increase this to make retune work - 4 samples didn't give it enough time because it has some "bursty" processing
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber); //notice I increased this, because tRetune needs more memory
    tADSR_init(&env, 10, 500, 0., 500, &leaf);
    tCycle_init(&mySine, &leaf);

    hw.StartAudio(AudioCallback);


    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



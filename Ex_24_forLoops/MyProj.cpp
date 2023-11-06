#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;

const int howMany = 100;
tCycle mySine[howMany];

//MANY MANY SINE WAVES

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();

    //now here's the per sample stuff
    for (int i = 0; i < size; i++)
    {
        //get the next sample from the sine wave object mySine and store it in a temporary variable of type float, called "mySample"
        float mySample = 0.0f;

        for (int j = 0; j < howMany; j++)
        {
            mySample = mySample + (tCycle_tick(&mySine[j]) / howMany);
        }

        
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
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);
    for (int i = 0; i < howMany; i++)
    {
        tCycle_init(&mySine[i], &leaf);
        tCycle_setFreq(&mySine[i], mtof(randomNumber() * 127.0f));
    }

    
    //let's also use the buttons to set the RGB LEDs on the pod.
    while(1) {
        //to make the pod LEDs work, you need to call "update" at the end of this main while loop, like this:
        hw.UpdateLeds();
    }
}



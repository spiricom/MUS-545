#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;

//SIMPLE AUDIO INPUT
// this example takes audio into the left line in channel and allows knob1 to control the volume. 



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

        mySample = mySample * hw.GetKnobValue(hw.KNOB_1); //knob1 gives values from 0-1 and multiplying a signal by a value between 0 and 1 changes it's amplitude (volume) between totally off and fully on. 
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
    hw.StartAudio(AudioCallback);
    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);


    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



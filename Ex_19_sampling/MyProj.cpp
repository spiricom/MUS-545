#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[455355];
DaisyPod hw;

tDattorroReverb myReverb;

//A simple reverb. Reverb is pretty expensive so you can't combine it with many other things.


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

        
        float reverbLevel = (hw.knob1.Value());
        //mix crossfades between dry to wet
        tDattorroReverb_setMix(&myReverb, reverbLevel);
        //size lengthens or shortens the reverb time
        tDattorroReverb_setSize(&myReverb, (hw.knob2.Value() * 2.0f) + 0.01f);

        //if button1 is pressed, freeze the contents of the reverb
        tDattorroReverb_setFreeze(&myReverb, hw.button1.Pressed());
        mySample = tDattorroReverb_tick(&myReverb, mySample);

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

    tDattorroReverb_init(&myReverb, &leaf);
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



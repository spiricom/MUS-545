#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[400000];
DaisyPod hw;

tTapeDelay delay;
float delayFeedback = 0.0f;

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

        tTapeDelay_setDelay(&delay, hw.knob1.Value() * 24000.0f);
        float delayOut = tTapeDelay_tick(&delay, mySample + (delayFeedback * hw.knob2.Value())); //knob1 gives values from 0-1 and multiplying a signal by a value between 0 and 1 changes it's amplitude (volume) between totally off and fully on. 
        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        delayFeedback = delayOut;

        
        out[0][i] = mySample + delayOut;
        out[1][i] = mySample + delayOut;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber);
    tTapeDelay_init(&delay, 24000, 24000, &leaf);
    hw.StartAudio(AudioCallback);
    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



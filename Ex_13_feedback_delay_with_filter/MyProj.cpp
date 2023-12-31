#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[400000];
DaisyPod hw;

//let's add a delay object from LEAF
tTapeDelay myDelay;

tSVF myFilter;

float myFeedbackSample;
//MORE COMPLEX DELAY ON AUDIO INPUT - adds feedback and a filter
// this example takes audio into the left line in channel.
// it also adds a delay, and the delay time is controlled by knob 1 and the delay feedback amount is controlled by knob 2
// a lowpass filter in the feedback loop helps it not go out of control in the high frequencies.



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

        float delaySpeed = hw.GetKnobValue(hw.KNOB_1) * 12000.0f;
        tTapeDelay_setDelay(&myDelay, delaySpeed);
        float sampleForDelay = mySample + (myFeedbackSample * hw.GetKnobValue(hw.KNOB_2));
        sampleForDelay = tSVF_tick(&myFilter, sampleForDelay);
        float myDelayedSample = tTapeDelay_tick(&myDelay, sampleForDelay);

        myFeedbackSample = myDelayedSample; //knob1 gives values from 0-1 and multiplying a signal by a value between 0 and 1 changes it's amplitude (volume) between totally off and fully on. 
        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        
        mySample = mySample + myDelayedSample;

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

    //init leaf stuff before calling hw.StartAudio()
    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber);
    tTapeDelay_init(&myDelay, 12000, 12000, &leaf);
    tSVF_init(&myFilter, SVFTypeLowpass, 6000.0f, 0.7f, &leaf);

    //now start audio
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



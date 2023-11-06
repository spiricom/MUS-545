#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[455355];
DaisyPod hw;

//let's add a delay object from LEAF
tTapeDelay myDelay;

tSimpleRetune mySimpleRetune;

float myFeedbackSample;

//SIMPLE DELAY ON AUDIO INPUT
// this example takes audio into the left line in channel.
// it also adds a delay, and the delay time is controlled by knob 1 and the delay mix is controlled by knob 2



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

         //tSimpleRetune wants a ratio for the pitch shifting - 0.5 would be octave down, 2 would be octave up
        //the second argument is "which voice" because the object lets you set up multiple voices to pitch-shift one input signal to multiple pitches.
        // we put in voice 0 because we're only using one and tSimpleRetune starts counting at zero (like most computery things)
        tSimpleRetune_tuneVoice(&mySimpleRetune, 0, 2.0f * hw.knob2.Value());
        mySample = tSimpleRetune_tick(&mySimpleRetune, mySample);

        float delaySpeed = hw.knob1.Value() * 12000.0f;
        tTapeDelay_setDelay(&myDelay, delaySpeed);
        float myDelayedSample = tTapeDelay_tick(&myDelay, mySample + (myFeedbackSample * 0.7));

        myFeedbackSample = myDelayedSample;

        mySample = mySample + myDelayedSample;


        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(64); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber);
    tSimpleRetune_init(&mySimpleRetune, 1, mtof(42.0f), mtof(84.0f), 1024, &leaf);
    tTapeDelay_init(&myDelay, 12000, 12000, &leaf);

    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



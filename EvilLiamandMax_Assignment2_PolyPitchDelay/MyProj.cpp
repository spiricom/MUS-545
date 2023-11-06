#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[400000];
DaisyPod hw;

//let's add a delay object from LEAF
tTapeDelay myDelay;
tTapeDelay myDelay2;

tSVF myFilter;
tSVF myFilter2;

float myFeedbackSample;
float myFeedbackSample2;

bool buttonTog = 0;
float delaySpeed;
float delaySpeed2;

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();

    for (size_t i = 0; i < size; i++)
    {
        //get the next sample from the left channel of the input (right channel would be in[1][i])
        float mySample = in[0][i];
        float mySample2 = in[0][i];

        if (buttonTog == 1)
        {
             delaySpeed = hw.GetKnobValue(hw.KNOB_1) * 48000.0f;
             delaySpeed2 = hw.GetKnobValue(hw.KNOB_2) * 48000.0f;
        }
        if (buttonTog == 0)
        {  
            int tempDelay = hw.GetKnobValue(hw.KNOB_1) * 12;
            delaySpeed = tempDelay * 4000;
            int tempDelay2 = hw.GetKnobValue(hw.KNOB_2) * 12;
            delaySpeed2 = tempDelay2 * 4000;
        }

        //float delaySpeed = hw.GetKnobValue(hw.KNOB_1) * 12000.0f;
       // float delaySpeed2 = hw.GetKnobValue(hw.KNOB_2) * 12000.0f;

        if (hw.button1.Pressed()) // the Pressed() function returns 1 if pressed and 0 if not pressed
        {        
            buttonTog = 0;
            hw.led1.Set(0, 0, 0);
        }
        if (hw.button2.Pressed()) // the Pressed() function returns 1 if pressed and 0 if not pressed
        {        
            buttonTog = 1;
            hw.led1.Set(1, 1, 1);
        }


        tTapeDelay_setDelay(&myDelay, delaySpeed);
        tTapeDelay_setDelay(&myDelay2, delaySpeed2);
        float sampleForDelay = mySample + (myFeedbackSample * 0.9);          //hw.GetKnobValue(hw.KNOB_2));
        float sampleForDelay2 = mySample2 + (myFeedbackSample2 * 0.9);
        sampleForDelay = tSVF_tick(&myFilter, sampleForDelay);
        sampleForDelay2 = tSVF_tick(&myFilter2, sampleForDelay2);
        float myDelayedSample = tTapeDelay_tick(&myDelay, sampleForDelay);
        float myDelayedSample2 = tTapeDelay_tick(&myDelay2, sampleForDelay2);

        myFeedbackSample = myDelayedSample;
        myFeedbackSample2 = myDelayedSample2;

        mySample = mySample + myDelayedSample;
        mySample2 = mySample2 + myDelayedSample2;

        out[0][i] = mySample;
        out[1][i] = mySample2;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(64); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    //init leaf stuff before calling hw.StartAudio()
    LEAF_init(&leaf, 48000, leafMemory, 400000, randomNumber);
    tTapeDelay_init(&myDelay, 48000, 48000, &leaf);
    tTapeDelay_init(&myDelay2, 48000, 48000, &leaf);
    tSVF_init(&myFilter, SVFTypeLowpass, 6000.0f, 0.7f, &leaf);
    tSVF_init(&myFilter2, SVFTypeLowpass, 6000.0f, 0.7f, &leaf);

    //now start audio
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}
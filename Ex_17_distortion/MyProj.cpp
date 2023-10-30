#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[455355];
DaisyPod hw;

//SIMPLEST DISTORTION IS JUST TAKING THE HYPERBOLIC TANGENT OF A SIGNAL
//WATCH OUT IT CAN GET LOUD AND FEEDBACK!!


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

        //tanh is a function that gives the hyperbolic tangent of an input
        //tanhf does it for single-precision floats, which is a little faster than double-precision floats.
        //check out this webpage: https://raphlinus.github.io/audio/2018/09/05/sigmoid.html
        //functions like this are called "sigmoid" because they make an "S" shaped curve when you plot them for -1 to 1
        //tanh is a normal function of the built-in math library in C. So you don't need a LEAF object.
        
        //use knob 1 for a gain control. We change the range to 0-30 instead of 0-1 so that we are boosting the input signal and pushing the clipping to happen.
        float myGain = (hw.knob1.Value() * 30.0f);
        mySample = mySample * myGain;
        mySample = tanhf(mySample);


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

    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



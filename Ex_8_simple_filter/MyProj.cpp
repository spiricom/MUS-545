#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;

//make a sawtooth wave object
tPBSaw mySaw;

//make a filter object
tSVF myFilter; // SVF stands for "state-variable filter". It's likely the most flexible and useful filter in LEAF.

//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;


//ADDING FILTERS TO THE SYNTHESIS 
// we use a sawtooth waveform, which has a lot of harmonics, and filter it to get a changing timbre


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
        // now pass myFreq as the value we pass to the setFreq function on the mySaw object
        tPBSaw_setFreq(&mySaw, myFreq);
        
        tSVF_setFreq(&myFilter, hw.knob1.Value()*3000.0f + 60.0f);
        tSVF_setQ(&myFilter, hw.knob2.Value()*3.0f + 0.3f);
        
        //get the next sample from the sine wave object mySaw and store it in a temporary variable of type float, called "mySample"
        float mySample = tPBSaw_tick(&mySaw);
        //now pass that sawtooth output through the lowpass filter object and store the result back into mySample
        mySample = tSVF_tick(&myFilter, mySample * 0.5f); // dropping the volume a bit so it doesn't distort if the Q is high


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
    tPBSaw_init(&mySaw, &leaf);
    tSVF_init(&myFilter, SVFTypeLowpass, 3000.0f, 0.6f, &leaf);

    while(1) {

        //the RGB leds on the pod need to be updated with the "update" function here to work
        hw.UpdateLeds();
    }
}



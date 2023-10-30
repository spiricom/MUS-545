#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[455355];
DaisyPod hw;

//let's add a pitch tracker object from LEAF    
tPitchDetector myPitchDetector;
tEnvelopeFollower myEnvelopeFollower;

tPBSaw myOsc;
//TRACK THE PITCH AND AMPLITUDE OF INPUT AND APPLY THEM TO A SYNTH SOUND

volatile float myDetectedPitch;

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

        tPitchDetector_tick(&myPitchDetector, mySample);
        myDetectedPitch = tPitchDetector_getFrequency(&myPitchDetector);
        float myDetectedEnvelope = tEnvelopeFollower_tick(&myEnvelopeFollower, mySample);
       
        tPBSaw_setFreq(&myOsc, myDetectedPitch);
        mySample = tPBSaw_tick(&myOsc) * myDetectedEnvelope;

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

    tPBSaw_init(&myOsc, &leaf);
    tPitchDetector_init(&myPitchDetector, 80.0f, 1000.0f, &leaf);
    tEnvelopeFollower_init(&myEnvelopeFollower, 0.01f, 0.999f, &leaf);
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



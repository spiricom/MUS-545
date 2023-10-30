#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[455355];

//for more recording space, we need to use the external 64 MB SDRAM chip instead of the internal memory
//to do that, set up a new variable that is the size of the memory amount you want to use
//and use the "DSY_SDRAM_BSS" macro after defining the variable (but before the semicolon) to place this array in the SDRAM memory
char bigMemory[67108864] DSY_SDRAM_BSS; //64 MB is 64 * 1024 * 1024 = 67108864

//now we need to make a LEAF "memorypool" object that will organize that large memory chunk
tMempool bigPool;

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

        tDattorroReverb_setMix(&myReverb, hw.knob1.Value());
        tDattorroReverb_setSize(&myReverb, (hw.knob2.Value() * 2.0f) + 0.1f);
        mySample = tDattorroReverb_tick(&myReverb, mySample);


        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(512); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();
    LEAF_init(&leaf, 48000, leafMemory, 455355, randomNumber);
    //first, initialize that large memory pool to be ready for usage
    tMempool_init(&bigPool, bigMemory, 67108864, &leaf);
    //reverb fails unless clear on allocation is set to 1 (so that the delay line doesn't have garbage in it at startup)
    leaf.clearOnAllocation = 1;
    //then place the reverb into that memory pool instead of the default pool, using the "initToPool" function instead of "init"
    tDattorroReverb_initToPool(&myReverb, &bigPool);

    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



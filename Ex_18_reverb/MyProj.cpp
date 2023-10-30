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

//we need a buffer to store the sample (just an array that holds the sample values)
tBuffer myBuffer;
tSampler mySampler;


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

        //set buffer position to the beginning on the beginning of a button1 press
        if (hw.button1.RisingEdge())
        {
            tBuffer_setRecordPosition(&myBuffer, 0);
            tBuffer_record(&myBuffer);
        }
        //stop recording when button1 is let go of
        else if (hw.button1.FallingEdge())
        {
            tBuffer_stop(&myBuffer);
        }


        //button 2 makes sample play back
        if (hw.button2.RisingEdge())
        {
            tSampler_setStart(&mySampler, 0);
            tSampler_play(&mySampler);
        }

        //letting go of button stops it
        else if (hw.button2.FallingEdge())
        {
            tSampler_stop(&mySampler);
        }

        //knob1 controls sample speed
        tSampler_setRate(&mySampler, (hw.knob1.Value() * 4.0f) - 2.0f); // scaling 0-1 range to -2-2 so it goes up to double speed both forwards and backwards

        //now tick the buffer with the input coming in so the buffer gets filled
        tBuffer_tick(&myBuffer, mySample);

        //then tick the sampler so it reads the buffer and sets mySample to the output value
        mySample = tSampler_tick(&mySampler);

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

    //first, initialize that large memory pool to be ready for usage
    tMempool_init(&bigPool, bigMemory, 67108864, &leaf);
    //then place the buffer into that memory pool instead of the default pool, using the "initToPool" function instead of "init"
    tBuffer_initToPool(&myBuffer, 240000, &bigPool); //five-second buffer, set up a segment of memory to store it
   
    //the tSampler object doesn't need to go into the large pool, it's just a small thing that references the data in the buffer
    tSampler_init(&mySampler, &myBuffer, &leaf); //make a "sampler" object that uses that buffer for memory
   
   
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



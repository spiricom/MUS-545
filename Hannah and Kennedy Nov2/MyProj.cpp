#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[455355];

typedef struct _sequenceParams
{

    float rate; 
    float vol;
} sequenceParams;

const int sequenceLength = 16;
sequenceParams mySequence[sequenceLength] = {{-1.f, .5f},{1.5f, .5f},{-.5f,.9f},{1.33f, .5f},{2.f, .7f},{-1.25f, .5f},{1.875f, .5f},{-2.f, .8f},{.75f, .4f},{-1.5f, .5f},{1.2f,.2f},{1.33f, .5f},{-1.125f, .5f},{1.25f, .6f},{-.66f, .5f},{-1.4f, .3f}};

float myFreq = 220.0f;
int sampleCounter = 0; // this will store the current count of how many samples have been processed since the last reset of the counter
int sampleCounterMaxValue = 48000; //we'll set this to 48000 for now, meaning the counter resets every second
float myTempo; // we'll use this as a variable to store what tempo we are trying to click the metronome counter at


//let's make an array of notes that will loop instead of a constant stream of random values
int whichNoteInSequence = 0;//which note are we currently on in the sequence


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

int sampleLength = 1; // store how long the sample is

//A simple reverb. Reverb is pretty expensive so you can't combine it with many other things.


float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)


{
    hw.ProcessAllControls();
    //now here's the per sample stuff

     myTempo = hw.GetKnobValue(hw.KNOB_2) * 190.0f + 10.0f; // will result in a range from 10 to 200
    //now let's figure out what the "reset value" of the counter needs to be to make the metronome click at the tempo we want
    //we are counting samples, and there are 48000 of those per second (because our sample rate is 48k)
    //so if we want a certain number of beats per minute, we need to figure out how many beats per second first
    float tempBeatsPerSecond = myTempo / 60.0f; //there are 60 seconds per minute. I'm keep this floats because it might be a low number and wouldn't want to truncate to integer and lose precision yet.
    //now let's figure out how many samples will happen in one beat of the metronome at the tempo we want
    float tempSamplesPerBeat =  48000.0f / tempBeatsPerSecond;
    //that will do it - think it through in your head, if tempo is 60, then tempBeatsPerSecond is 1.0 and tempSamplesPerBeat is 48000.0. If it's 120, then beatspersecond is 2.0 and samplesperbeat is 24000.0

    //now we can just use that tempSamplesPerBeat as the maxSampleCounterValue
    // sampleCounterMaxValue is an int, it will truncate tempSamplesPerBeat (for instance 48000.5 to 48000), but the range of numbers we will be in means that'll be fine.
    //let's divide the number of samples per beat by 4 to get 1/16th-notes instead of 1/4-notes.
    sampleCounterMaxValue = tempSamplesPerBeat /4;

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
            //figure out how long you recorded for
            sampleLength = tBuffer_getRecordPosition(&myBuffer);
            //now use that timing to set the sampler to know how long into the sample to read
            tSampler_setLength(&mySampler, sampleLength);
        }


        //button 2 makes sample play back
        if (hw.button2.RisingEdge())
        {
            tSampler_setStart(&mySampler, 0);
            tSampler_play(&mySampler);
            //since now we're going to have an if statement control structure in here, let's define "mySample" outside of the if so it already exists and doesn't depend on the conditional situation
            float mySample = 0.0f;
            sampleCounter = 0;
        }
        else if (hw.button2.Pressed())
        {
             //when sampleCounter is zero, it's a metronome pulse. At the bottom of the for loop we are in is the code that resets it
            if (sampleCounter == 0)
            {
                whichNoteInSequence++;
                //if we go over the number of elements in the array, loop back to 0
                if (whichNoteInSequence >= sequenceLength)
                {
                    whichNoteInSequence = 0;
                }
            }        

            //increment the sampleCounter variable so we can keep track of how many samples have passed. 
            //it will count 48000 samples per second because that's our "sample rate"
            sampleCounter = sampleCounter + 1;
            //we'll reset it to zero when it reaches sampleCounterMaxValue, then we can check for a value of zero to know 
            //when our metronome has ticked (if we set sampleCounterMaxValue so that it will reach it at the time interval we want)
            if (sampleCounter >= sampleCounterMaxValue)
            {
                sampleCounter = 0;
            }
    
        }
        //letting go of button stops it
        else if (hw.button2.FallingEdge())
        {
            tSampler_stop(&mySampler);
        }

        //knob1 controls sample speed
        tSampler_setRate(&mySampler, mySequence[whichNoteInSequence].rate); // scaling 0-1 range to -2-2 so it goes up to double speed both forwards and backwards

        //now tick the buffer with the input coming in so the buffer gets filled
        tBuffer_tick(&myBuffer, mySample);

        //then tick the sampler so it reads the buffer and sets mySample to the output value
        mySample = tSampler_tick(&mySampler);

        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = mySequence[whichNoteInSequence].vol * mySample;
        out[1][i] = mySequence[whichNoteInSequence].vol * mySample;
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
    tSampler_setMode(&mySampler, PlayLoop); //other options are Play Normal (no looping), and PlayBackAndForth
   
    hw.StartAudio(AudioCallback);

    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



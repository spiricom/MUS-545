#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;

//SUPER SIMPLE SINE WAVE WITH KNOB FREQUENCY CONTROL
//this example makes a fun siren bloop if you hold down button1 on the pod and turn knob1 to change the speed

float randomNumber()
{
    //call the daisy function to get a random number between 0 and 1 (what leaf wants)
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    
    //use the "myFreq" variable we declared up top here to temporarily store a value that is
    //set to be something between 220 and 880 (two octaves), depending on the knob position.
    // we do this by taking the knob value, which comes in as something between 0.0 and 1.0 
    //(which I got by calling the function GetKnobValue() in the hw object, passing in the hw object's KNOB_1 as the input to the function)
    //and multiplying it by the range we want (880-220, or 660), 
    //and then offsetting it by adding 220 so it starts where we want it to start. (otherwise it would start at frequency zero)
    //the little f I put after numbers makes them a float literal, meaning it tells the compiler to use "single-precision floats", instead of "double-precision floats".
    // that's not necessary, it'll work if I just write 660.0 instead of 660.0f, but because of the type of chip on the daisy (STM32H7),
    // adding the f at the end of floating point numbers means they will be computed faster (at the expense of not being super precise).
    // I just write the f by habit, you don't have to. All internal computations in leaf are in single-precision floats. 
    myFreq = (660.0f * hw.GetKnobValue(hw.KNOB_1)) + 220.0f;

    // now pass myFreq as the value we pass to the setFreq function on the mySine object
    tCycle_setFreq(&mySine, myFreq);

    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //get the next sample from the sine wave object mySine and store it in a temporary variable of type float, called "mySample"
        float mySample = tCycle_tick(&mySine);

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
    tCycle_init(&mySine, &leaf);
    tCycle_setFreq(&mySine, 440.0f); //this will be written over by the audio callback loop immediately when we read in that knob value, so we could remove it without any problem.
    
    //blink that light! this endless while loop will be interrupted by the audio callback when the audio chip needs new data (every 4 samples, because audio block size was set to 4 above)
    while(1) {
        hw.seed.SetLed(1); //turn on the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
        hw.seed.SetLed(0); //turn off the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
    }
}



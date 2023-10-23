#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;
tPBSaw mySaw; // the PB stands for polyBLEP, the technique I'm using to generate the signal. Or peanut butter, whichever you prefer.
tPBPulse myPulse;
tPBTriangle myTriangle;

//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;

int whichWaveform = 0; 

//OTHER WAVEFORMS
//some other waveforms to try
//knob 1 sets frequency. knob2 sets which waveform (of 4 waveform shapes)

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



    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //since now we're going to have an if statement control structure in here, let's define "mySample" outside of the if so it already exists and doesn't depend on the conditional situation
        float mySample = 0.0f;

        whichWaveform = hw.GetKnobValue(hw.KNOB_2) * 3.999f; // there are 4 waveforms, so starting at 0 we get 0 1 2 3 
        
        if (whichWaveform == 0)
        {
            //sine wave
            tCycle_setFreq(&mySine, myFreq);
            //note that since we defined mySample above, we don't put "float" here
            mySample = tCycle_tick(&mySine);
            //we can also set the LED stuff here if we want
            hw.led1.Set(1.0f, 1.0f, 1.0f); //led1 white (RGB all on)
        }
        else if (whichWaveform == 1)
        {
             //saw wave
            tPBSaw_setFreq(&mySaw, myFreq);
            //note that since we defined mySample above, we don't put "float" here
            mySample = tPBSaw_tick(&mySaw);
            hw.led1.Set(1.0f, 0.0f, 0.0f); //led1 red
        }
        else if (whichWaveform == 2)
        {
             //pulse wave
            tPBPulse_setFreq(&myPulse, myFreq);
            //note that since we defined mySample above, we don't put "float" here
            mySample = tPBPulse_tick(&myPulse);
            hw.led1.Set(0.0f, 1.0f, 0.0f); //led1 green
        }
        else if (whichWaveform == 3)
        {
             //triangle wave
            tPBTriangle_setFreq(&myTriangle, myFreq);
            //note that since we defined mySample above, we don't put "float" here
            mySample = tPBTriangle_tick(&myTriangle);
            hw.led1.Set(0.0f, 0.0f, 1.0f); //led1 blue
        }

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
    //need to initialize all of the oscillators
    tCycle_init(&mySine, &leaf);
    tPBSaw_init(&mySaw, &leaf);
    tPBPulse_init(&myPulse, &leaf);
    tPBTriangle_init(&myTriangle, &leaf);
   
    //blink that light! this endless while loop will be interrupted by the audio callback when the audio chip needs new data (every 4 samples, because audio block size was set to 4 above)
    while(1) {
        //you need to call this here to use the RGB LEDs on the pod.
        hw.UpdateLeds();
    }
}



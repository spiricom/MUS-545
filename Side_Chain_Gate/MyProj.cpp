#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tPBSaw mySaw;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;
tSVF myFilt;
tEnvelopeFollower EnvF;
int onoff = 1;
float gain = 1;
float thresh = .1;
tADSRS Env;
tNReverb Verb;

int onOff = 0;

//SUPER SIMPLE SINE WAVE WITH KNOB FREQUENCY CONTROL
//use knob1 to control the frequency of a sine wave

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


    // now pass myFreq as the value we pass to the setFreq function on the mySine object
    myFreq = 200;
    tPBSaw_setFreq(&mySaw, myFreq);

    //now here's the per sample stuff

    for (size_t i = 0; i < size; i++)
    {
   gain = (hw.GetKnobValue(hw.KNOB_1));
   thresh = (hw.GetKnobValue(hw.KNOB_2)) * .1;
    float myInput = in[0][i];
    float myADSR = tADSRS_tick(&Env);

        //get the next sample from the sine wave object mySine and store it in a temporary variable of type float, called "mySample"
        float mySample = tPBSaw_tick(&mySaw);
        float myEnv = tEnvelopeFollower_tick(&EnvF, myInput) * gain * 100;
        if ((myEnv > thresh) && (onOff == 0))
        {
            tADSRS_on(&Env, 1);
            onOff = 1;
        }
        else if (((myEnv <= thresh) && (onOff == 1)))
        {
            tADSRS_off(&Env);
            onOff = 0;
        }
    float myGated = mySample * myADSR;
    float myVerb = tNReverb_tick(&Verb, myGated);
        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = myVerb;
        out[1][i] = myVerb;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();
    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);
    tPBSaw_init(&mySaw, &leaf);
    tSVF_init(&myFilt, SVFTypeLowpass, 1000.0f, 0.7f, &leaf);
    tEnvelopeFollower_init(&EnvF, 0.0f, 0.95f, &leaf);
    tADSRS_init(&Env, 50, 50, 50, 50, &leaf);
    tNReverb_init(&Verb, 2, &leaf);
    hw.StartAudio(AudioCallback);


//this will be written over by the audio callback loop immediately when we read in that knob value, so we could remove it without any problem.
    
    //blink that light! this endless while loop will be interrupted by the audio callback when the audio chip needs new data (every 4 samples, because audio block size was set to 4 above)
    while(1) {
        hw.seed.SetLed(1); //turn on the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
        hw.seed.SetLed(0); //turn off the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
    }
}



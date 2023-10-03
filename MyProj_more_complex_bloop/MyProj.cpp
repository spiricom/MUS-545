#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;

//SIREN BLOOPS
//this example makes a fun siren bloop if you hold down button1 on the pod and turn knob1 to change the speed

float randomNumber()
{
    //calls the daisy function that gets a random number between 0-1 (which is what LEAF wants)
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    
    //use the "myFreq" variable we declared up top here to temporarily store a value that is
    // increased each time this audio callback happens. It's increased by 10 * the value of the knob (which comes in as 0.0-1.0) each time. 
    // so that means it's increased by 0.0-10.0 depending on the knob position. 
    // this makes a fun siren bloop bloop sound where the knob is changing the speed (the higher the knob value the faster it will increase)
    // because myFreq is global (defined above things outside of the functions) it's value is remembered between calls to this AudioCallback function. 
    myFreq = myFreq + (10.0f * hw.GetKnobValue(hw.KNOB_1));

    // now pass myFreq as the value we pass to the setFreq function on the mySine object
    tCycle_setFreq(&mySine, myFreq);

    //if we just did the above, it would soon go out of audio range. So let's cap it at some value and loop it around - if it's bigger than 1000, set it back down to 220.
    if (myFreq >= 1000.0f)
    {
        myFreq = 220.0f;
    }

    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //get the next sample from the sine wave object mySine and store it in a temporary variable of type float, called "mySample"
        float mySample = tCycle_tick(&mySine);

        //multiply mySample by the value returned by the "Pressed()" function for button1 on the pod. This will be 0 or 1 depending on whether the button is pressed.
        //so if it's not pressed, it's zero, and the sample will be multiplied by zero and silenced. 
        //this makes it so there's no sound unless you press button 1 and hold it down.
        mySample = mySample * hw.button1.Pressed();

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
    tCycle_setFreq(&mySine, 440.0f);
    //blink that light! this endless while loop will be interrupted by the audio callback when the audio chip needs new data (every 4 samples, because audio block size was set to 4 above)
    while(1) {
        hw.seed.SetLed(1); //turn on the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
        hw.seed.SetLed(0); //turn off the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
    }
}



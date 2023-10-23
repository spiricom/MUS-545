#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;

//READ THEM BBUTTONZZ

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();

    
    //if button1 is pressed, set the frequency to 440.0. 
    if (hw.button1.Pressed()) // the Pressed() function returns 1 if pressed and 0 if not pressed
    {
        myFreq = 440.0f;
    }
    //if button1 isn't pressed, check if button2 is pressed, and if so, set the frequency to 880.0.
    else if (hw.button2.Pressed()) // this condition will only be checked if the "if" statement above was false.
    {
        myFreq = 880.0f;
    }
    //otherwise, set the frequency to 220.0
    else
    {
        myFreq = 220.0f;
    }

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
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);
    tCycle_init(&mySine, &leaf);
    
    //let's also use the buttons to set the RGB LEDs on the pod.
    while(1) {
        //if button1 is down, make led1 green
        if (hw.button1.Pressed())
        {
            hw.led1.SetGreen(1);
        }
        else
        {
            hw.led1.SetGreen(0);
        }

        //if button2 is down, make led2 green
        if (hw.button2.Pressed())
        {
            hw.led2.SetGreen(1);
        }
        else
        {
            hw.led2.SetGreen(0);
        }

        //to make the pod LEDs work, you need to call "update" at the end of this main while loop, like this:
        hw.UpdateLeds();
    }
}



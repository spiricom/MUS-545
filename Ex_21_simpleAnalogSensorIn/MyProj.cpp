#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
dsy_gpio_pin A1Pin;
dsy_gpio_pin A7Pin;

//SIMPLE SENSOR INPUT
//CONNECT SENSORS TO A1 and A7 pins on the Daisy Pod

tPBPulse myOsc;


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
        tPBPulse_setFreq(&myOsc, hw.seed.adc.GetFloat(0) * 440.0f + 40.0f);
        tPBPulse_setWidth(&myOsc, hw.seed.adc.GetFloat(1) * 0.9f + 0.05f);
        float mySample = tPBPulse_tick(&myOsc);

        mySample = mySample * hw.GetKnobValue(hw.KNOB_1); //knob1 gives values from 0-1 and multiplying a signal by a value between 0 and 1 changes it's amplitude (volume) between totally off and fully on. 
        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = mySample;
        out[1][i] = mySample;
    }
}

int main(void)
{
    hw.Init();
   
   //here's some code that sets up those additional analog sensor inputs
   //I've copied out the internal Daisy code that sets up the knobs, and added the analog inputs to that
   //we can now use the knobs as usual
   // but also we can get the new sensor channels as hw.seed.adc.GetFloat(0) and hw.seed.adc.GetFloat(1)

    AdcChannelConfig ext_knob_init[4];
    A1Pin = dsy_pin(DSY_GPIOA, 3);
    A7Pin = dsy_pin(DSY_GPIOA, 5);
    ext_knob_init[0].InitSingle(A1Pin);
    ext_knob_init[1].InitSingle(A7Pin);
    ext_knob_init[2].InitSingle(hw.seed.GetPin(21));
    ext_knob_init[3].InitSingle(hw.seed.GetPin(15));
    // Initialize with the knob init struct w/ 4 members
    // Set Oversampling to 32x
    hw.seed.adc.Init(ext_knob_init, 4);
    // Make an array of pointers to the knobs.
    hw.knobs[0] = &hw.knob1;
    hw.knobs[1] = &hw.knob2;
    for(int i = 0; i < 2; i++)
    {
        hw.knobs[i]->Init(hw.seed.adc.GetPtr(i+2), hw.seed.AudioCallbackRate());
    }
    
    //OK that's done
    //now back to the usual boilerplate stuff

    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    Random::Init();
    hw.StartAdc();

    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);

    //for this example, let's set up a pulse wave
    tPBPulse_init(&myOsc, &leaf);

    hw.StartAudio(AudioCallback);
    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



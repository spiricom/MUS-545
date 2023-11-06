#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
dsy_gpio_pin A1Pin;
dsy_gpio_pin A7Pin;

//MORE COMPLEX SENSOR INPUT EXAMPLE
//CONNECT SENSORS TO A1 and A7 pins on the Daisy Pod
//this one adds the knobs doing FM (frequency modulation) to the pulse wave with a sawtooth wave

tPBPulse myOsc;
tPBSaw myMod;

//let's make some variables so we can peek at the sensor values in the debugger. Volatile so the compiler doesn't optimize them away
volatile float sensor1;
volatile float sensor2;

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
        float modFreq = hw.GetKnobValue(hw.KNOB_1) * 440.0f;
        tPBSaw_setFreq(&myMod, modFreq);
        float sawOutput = tPBSaw_tick(&myMod);
        float modAmp = hw.GetKnobValue(hw.KNOB_2) * 2000.0f;
        sensor1 = hw.seed.adc.GetFloat(0);
        sensor2 = hw.seed.adc.GetFloat(1);
        tPBPulse_setFreq(&myOsc, sensor1 * 440.0f + (sawOutput * modAmp));
        tPBPulse_setWidth(&myOsc, sensor2 * 0.9f + 0.05f);
        float mySample = tPBPulse_tick(&myOsc);
        
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
    tPBSaw_init(&myMod, &leaf);

    hw.StartAudio(AudioCallback);
    while(1) {
        //the RGB leds on the pod need to be updated with the "update" function here to work

        hw.UpdateLeds();
    }
}



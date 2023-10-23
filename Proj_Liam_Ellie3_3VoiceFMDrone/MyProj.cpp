#include "daisy_pod.h"
#include "leaf.h"

using namespace daisy;

enum EncoderMode
{
    Frequency,
    Timbre
};

LEAF     leaf;
char     leafMemory[65535];
DaisyPod hw;

tCycle myCarriers[3];
tCycle myModulators[3];

int   selectedVoice = 0;
float myFreqs[3]    = {440, 440, 440};
float myOffsets[3]  = {0};
int   myNotes[3]    = {60, 60, 60};
float myVolumes[3]  = {0};

float myModStrength[3] = {0};
float myModRatios[3]   = {0};

EncoderMode inputMode = Frequency;

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    // when "apply" button pressed, update values from current knob state
    if(hw.button2.Pressed())
    {
        if(inputMode)
        {
            myModRatios[selectedVoice]   = hw.GetKnobValue(hw.KNOB_1) * 6.0f;
            myModStrength[selectedVoice] = hw.GetKnobValue(hw.KNOB_2);
        }
        else
        {
            myOffsets[selectedVoice] = hw.GetKnobValue(hw.KNOB_1) * 2 - 1;
            myVolumes[selectedVoice] = hw.GetKnobValue(hw.KNOB_2);
        }
    }

    // calculate + update frequency of carrier
    myFreqs[selectedVoice]
        = mtof(myNotes[selectedVoice] + myOffsets[selectedVoice]);


    // update frequency of modulating operator
    tCycle_setFreq(&myModulators[selectedVoice],
                   myFreqs[selectedVoice] * myModRatios[selectedVoice]);


    // now here's the per sample stuff
    for(size_t i = 0; i < size; i++)
    {
        float mySample = 0.0f;

        for(int j = 0; j < 3; j++)
        {
            // sample the modulating operator
            float myModSample
                = myModStrength[j] * tCycle_tick(&myModulators[j]);

            // update carrier frequency and sample
            tCycle_setFreq(&myCarriers[j], myFreqs[j] * (myModSample + 1));
            mySample += tCycle_tick(&myCarriers[j]) * myVolumes[j];
        }

        // keep gain at 0-1
        mySample /= 3.0f;

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

    for(int i = 0; i < 3; i++)
    {
        tCycle_init(&myCarriers[i], &leaf);
        tCycle_init(&myModulators[i], &leaf);
    }

    while(1)
    {
        hw.button1.Debounce();
        hw.encoder.Debounce();

        // button 1 cycles through selected voices
        if(hw.button1.RisingEdge())
        {
            selectedVoice += 1;
            if(selectedVoice > 2)
            {
                selectedVoice = 0;
            }
        }

        // encoder button switches knob modes
        if(hw.encoder.RisingEdge())
        {
            switch(inputMode)
            {
                case Frequency: inputMode = Timbre; break;
                case Timbre: inputMode = Frequency; break;
            }
        }

        // encoder wheel shifts midi note of voice
        myNotes[selectedVoice] += hw.encoder.Increment();

        // update LED 2 based on input mode
        // tuning/volume mode = magenta
        // fm/timbre mode     = yellow
        switch(inputMode)
        {
            case Frequency: hw.led2.Set(1, 0, 1); break;
            case Timbre: hw.led2.Set(1, 1, 0); break;
        }

        // update LED 1 based on selected voice
        // voice 0 = red
        // voice 1 = green
        // voice 2 = blue
        switch(selectedVoice)
        {
            case 0: hw.led1.Set(1, 0, 0); break;
            case 1: hw.led1.Set(0, 1, 0); break;
            case 2: hw.led1.Set(0, 0, 1); break;
        }

        hw.UpdateLeds();
    }
}

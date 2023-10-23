#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF     leaf;
char     leafMemory[65535];
DaisyPod hw;

tCycle      myCycles[3];
tPBSaw      mySaws[3];
tPBPulse    myPulses[3];
tPBTriangle myTris[3];


int   selectedVoice = 0;
float myFreqs[3]    = {440, 440, 440};
float myOffsets[3]  = {0};
int   myNotes[3]    = {60, 60, 60};
float myVolumes[3]  = {0};

int myWaveform[3] = {0};

bool inputMode = 0;

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    if(hw.button2.Pressed())
    {
        float offset             = hw.GetKnobValue(hw.KNOB_1) * 2 - 1;
        myVolumes[selectedVoice] = hw.GetKnobValue(hw.KNOB_2);

        myFreqs[selectedVoice]    = mtof(myNotes[selectedVoice] + offset);
    }

    // //now here's the per sample stuff
    for(size_t i = 0; i < size; i++)
    {
        float mySample = 0.0f;

        for(int j = 0; j < 3; j++)
        {
            switch(myWaveform[j])
            {
                case 0:
                    tCycle_setFreq(&myCycles[j], myFreqs[j]);
                    mySample += tCycle_tick(&myCycles[j]) * myVolumes[j];
                    break;
                case 1:
                    tPBSaw_setFreq(&mySaws[j], myFreqs[j]);
                    mySample += tPBSaw_tick(&mySaws[j]) * myVolumes[j];
                    break;
                case 2:
                    tPBPulse_setFreq(&myPulses[j], myFreqs[j]);
                    mySample += tPBPulse_tick(&myPulses[j]) * myVolumes[j];
                    break;
                case 3:
                    tPBTriangle_setFreq(&myTris[j], myFreqs[j]);
                    mySample += tPBTriangle_tick(&myTris[j]) * myVolumes[j];
                    break;
            }
        }

        mySample /= 3.0f;

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

    for(int i = 0; i < 3; i++)
    {
        tCycle_init(&myCycles[i], &leaf);
        tPBSaw_init(&mySaws[i], &leaf);
        tPBPulse_init(&myPulses[i], &leaf);
        tPBTriangle_init(&myTris[i], &leaf);
    }
    //let's also use the buttons to set the RGB LEDs on the pod.
    while(1)
    {
        //if button1 is down, make led1 green
        hw.button1.Debounce();
        hw.encoder.Debounce();

        if(hw.button1.RisingEdge())
        {
            selectedVoice += 1;
            if(selectedVoice > 2)
            {
                selectedVoice = 0;
            }
        }

        if(hw.encoder.RisingEdge())
        {
            inputMode = !inputMode;
        }

        int encoderResult = hw.encoder.Increment();

        int selectedWaveform = myWaveform[selectedVoice];

        if(inputMode)
        {
            selectedWaveform += hw.encoder.Increment();
            if(selectedWaveform < 0)
            {
                selectedWaveform = 3;
            }
            else if(selectedWaveform > 3)
            {
                selectedWaveform = 0;
            }
            myWaveform[selectedVoice] = selectedWaveform;     
        }
        else
        {
            myNotes[selectedVoice] += encoderResult;
        }

        if(inputMode)
        {
            hw.led2.Set(0, 0, 1);
        }
        else
        {
            hw.led2.Set(1, 0, 0);
        }

        switch(selectedVoice)
        {
            case 0: hw.led1.Set(1, 0, 0); break;
            case 1: hw.led1.Set(0, 1, 0); break;
            case 2: hw.led1.Set(0, 0, 1); break;
        }

        //to make the pod LEDs work, you need to call "update" at the end of this main while loop, like this:
        hw.UpdateLeds();
    }
}

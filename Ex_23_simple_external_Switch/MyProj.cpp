#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;

Switch externalSwitch;
dsy_gpio_pin extSwitchPin;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;

//READ AN EXTERNAL BUTTON CONNECTED BETWEEN SDA AND GND

float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    //when you've got external switches, call the debounce function right after the ProcessAllControls function
    externalSwitch.Debounce();

    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        float switchOnOrOff = externalSwitch.Pressed(); //store a 0 or 1 depending on if the switch is pressed, we'll use it as amplitude control
        //get the next sample from the sine wave object mySine and store it in a temporary variable of type float, called "mySample"
        float mySample = tCycle_tick(&mySine) * switchOnOrOff; // multiply by the stored amplitude control

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

    //tell it which pin the external switch is connected on. B9 (DSY_GPIOB, 9) is the one marked SDA on the board
    //the pins are SDA=B9, SCL=B8, MO=B5, MI=B4, SCK=G11, SS=G10 
    extSwitchPin = dsy_pin(DSY_GPIOB, 9);
    externalSwitch.Init(extSwitchPin);

    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);
    tCycle_init(&mySine, &leaf);
    tCycle_setFreq(&mySine, 220.0f);

    hw.StartAudio(AudioCallback);


    //let's also use the buttons to set the RGB LEDs on the pod.
    while(1) {

        //to make the pod LEDs work, you need to call "update" at the end of this main while loop, like this:
        hw.UpdateLeds();
    }
}



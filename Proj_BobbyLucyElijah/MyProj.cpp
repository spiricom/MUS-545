#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;

//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;
int sampleCounter = 0; // this will store the current count of how many samples have been processed since the last reset of the counter
int sampleCounterMaxValue = 48000; //we'll set this to 48000 for now, meaning the counter resets every second
float myTempo; // we'll use this as a variable to store what tempo we are trying to click the metronome counter at

// initializing index for chord sweeps (chord degree)
int x = 0;


//program that lets you sweep major, minor, or augmented chords depending on whether you are holding down a button

//major chord
int majorChord[5] = {0, 4, 7, 11, 12};

//minor chord
int minorChord[5] = {0, 3, 7, 10, 12};

//augmented
int domChord[5] = {0, 4, 7, 10, 12};

//diminished
int dimChord[5] = {0, 3, 6, 9, 12};

//DOING SOMETHING WITH A METRONOME
//turn knob1 to change the speed of a metronome, which is making the frequency of a sine wave pick a new random value each metronome click
//the computer is thinking!!! bloooppbeeeeppbeeeoeeepeoepepeoe


float randomNumber()
{
    //call the daisy function to get a random number between 0 and 1 (what leaf wants)
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();

    int startingNote = hw.GetKnobValue(hw.KNOB_2) * 12.99f;
    startingNote = startingNote + 60;

    //lets use knob1 for the tempo control
    myTempo = hw.GetKnobValue(hw.KNOB_1) * 390.0f + 10.0f; // will result in a range from 10 to 200
    //now let's figure out what the "reset value" of the counter needs to be to make the metronome click at the tempo we want
    //we are counting samples, and there are 48000 of those per second (because our sample rate is 48k)
    //so if we want a certain number of beats per minute, we need to figure out how many beats per second first
    float tempBeatsPerSecond = myTempo / 60.0f; //there are 60 seconds per minute. I'm keep this floats because it might be a low number and wouldn't want to truncate to integer and lose precision yet.
    //now let's figure out how many samples will happen in one beat of the metronome at the tempo we want
    float tempSamplesPerBeat =  48000.0f / tempBeatsPerSecond;
    //that will do it - think it through in your head, if tempo is 60, then tempBeatsPerSecond is 1.0 and tempSamplesPerBeat is 48000.0. If it's 120, then beatspersecond is 2.0 and samplesperbeat is 24000.0

    //now we can just use that tempSamplesPerBeat as the maxSampleCounterValue
    // sampleCounterMaxValue is an int, it will truncate tempSamplesPerBeat (for instance 48000.5 to 48000), but the range of numbers we will be in means that'll be fine.
    //let's divide the number of samples per beat by 4 to get 1/16th-notes instead of 1/4-notes.
    sampleCounterMaxValue = tempSamplesPerBeat / 4.0f;


    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //since now we're going to have an if statement control structure in here, let's define "mySample" outside of the if so it already exists and doesn't depend on the conditional situation
        float mySample = 0.0f;

        //when sampleCounter is zero, it's a metronome pulse. At the bottom of the for loop we are in is the code that resets it
        if (sampleCounter == 0)
        {
            //metronome pulse happened! do something!!!!!!!
            //let's set the frequency to some random value between 100 and 2000
            //we can use the randomNumber function that's defined above this AudioCallback function

            //both buttons pressed - dim 7 chord
            if (hw.button1.Pressed() && hw.button2.Pressed())
                  {
                    myFreq = mtof(startingNote + dimChord[x]);
                    tCycle_setFreq(&mySine, myFreq);
                 }

            //only button 1 pressed - Minor 7 chord
            else if (hw.button1.Pressed() && !(hw.button2.Pressed())) // this condition will only be checked if the "if" statement above was false.
                {
                    myFreq = mtof(startingNote + minorChord[x]);
                    tCycle_setFreq(&mySine, myFreq);
                  }

            //only button 2 pressed - dominant 7 chord
             else if (hw.button2.Pressed() && !(hw.button1.Pressed())) // this condition will only be checked if the "if" statement above was false.
                {
                    myFreq = mtof(startingNote + domChord[x]);
                    tCycle_setFreq(&mySine, myFreq);
                  }

            //no buttons pressed - major 7 chord
              else
                  {
                    myFreq = mtof(startingNote + majorChord[x]);
                    tCycle_setFreq(&mySine, myFreq);
                  }

            //now the frequency will only change when the sample counter equals 0, which only happens when the counter resets because it reached a maximum limit we set, based on the tempo we want
        }
        
        //note that since we defined mySample above, we don't put "float" here
        mySample = tCycle_tick(&mySine);
        //we can also set the LED stuff here if we want
        hw.led1.Set(randomNumber(), randomNumber(), randomNumber()); //set led1 to a random color
        hw.led2.Set(randomNumber(), randomNumber(), randomNumber()); //set led2 to a random color


        //increment the sampleCounter variable so we can keep track of how many samples have passed. 
        //it will count 48000 samples per second because that's our "sample rate"
        sampleCounter = sampleCounter + 1;
        //we'll reset it to zero when it reaches sampleCounterMaxValue, then we can check for a value of zero to know 
        //when our metronome has ticked (if we set sampleCounterMaxValue so that it will reach it at the time interval we want)
        if (sampleCounter >= sampleCounterMaxValue)
        {
            //increments the index so it sweeps through the chord
            x = x + 1;
            sampleCounter = 0;

            //resets to 0 after going through the whole chord
            if (x == 5) {
            x = 0;
            }
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
    tCycle_init(&mySine, &leaf);
   
    //blink that light! this endless while loop will be interrupted by the audio callback when the audio chip needs new data (every 4 samples, because audio block size was set to 4 above)
    while(1) {

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
        //you need to call this here to use the RGB LEDs on the pod.
        hw.UpdateLeds();
    }
}



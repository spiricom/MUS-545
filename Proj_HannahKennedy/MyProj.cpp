#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;
tCycle mySine2;
tCycle mySine3;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;
float myFreq2 = 440.0f;
//sequence variables
float myTempo = 200.0f;
int sampleCounter = 0;
int sampleCounterMaxValue = 48000;

int mySequence[16] = {60,54,72,85,93,43,80,66,60,57,72,85,93,46,80,66};
int whichNoteInSequence = 0;

int majorScale[8] = {0,2,4,5,7,9,11,12};


//switch chooses between behaviors - added super low sine to major scale thing

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
    if (hw.button1.Pressed())
    {
        for (size_t i = 0; i < size; i++)
        {
            float mySample = 0.0f;

            if (sampleCounter == 0)
            {
                float tempMidiNote = mySequence[whichNoteInSequence];
                whichNoteInSequence = whichNoteInSequence + 1;

                if (whichNoteInSequence >= 16)
                {
                    whichNoteInSequence = 0;
                }
                myFreq = mtof(tempMidiNote);
                tCycle_setFreq(&mySine, myFreq);
                float ledValue = (tempMidiNote - 48.0f) / 24.0f;
                hw.led1.Set(ledValue, 0.0f, 0.0f);
                hw.led2.Set(ledValue, 0.0f, 0.0f);
            }
             mySample = tCycle_tick(&mySine);
             sampleCounter = sampleCounter + 1;
             if (sampleCounter >= sampleCounterMaxValue)
             {
                sampleCounter = 0;
             }
             out[0] [i] = mySample;
             out[1] [i] = mySample;

        } 

    }
    
    else
    {
    myFreq = (660.0f * hw.GetKnobValue(hw.KNOB_1)) + 220.0f;
    myFreq2 = (440.0f * hw.GetKnobValue(hw.KNOB_2)) + 100.0f;

    //let's make a variable we'll use as the midi note value.
    int theNote;
    int theNote2;

    //now let's try to get the knob data to be a number between zero and 7. We've got 8 notes (counting the octave at the top)
    // so let's turn our 0-1 values from the knob into 0-7. If we multiply by 7, that would sort of work, but the top note would just barely 
    // appear at the extreme end of the knob's range. Instead, let's multiply 0-1 by 7.99. Then we can truncate it and lose the part after the decimal point, and we get more even
    // distribution of the numbers. Let's make another temporary variable to store that, just to keep things straight for ourselves.
    int scaleDegree = hw.GetKnobValue(hw.KNOB_1) * 7.999f;
    //how can we trust it'll get truncated to just the integer values? by storing it in an integer type, we know the things after the decimal place will get lopped off. 
    //note that it doesn't round. It just loses everything after the decimal point. So 7.9999 is 7.

    //now let's use that "scaleDegree" as an index into the array we defined up top. 
    //how do we do that?
    // you can retrieve a value from an element of an array by typing arrayName[whichIndex]. 
    //So majorScale[0] will give us 0, and majorScale[1] will give us 2, and majorScale[2] will give us 4, and so on. 
    int scaleNote = majorScale[scaleDegree];
    //now theNote has a value that is either 0, 2, 4, 5, 7, 9, 11 or 12, depending on the position of knob1. 

    // let's add some offset to this to set what pitch our major scale starts on. midinote 60 is middle C, so maybe that's a good starting point.
    int startingNote = hw.GetKnobValue(hw.KNOB_2) * 12.99f;
    //this one will give us a chromatic scale within an octave. 
    //let's add 60 to that so it starts on middle c and goes up to high c.
    //note that we don't put the "int" in front of startingNote this time because it's already been defined.
    startingNote = startingNote + 60;

    //now let's put the starting note and the scale note together to get our final midinote
    theNote = 60 + scaleNote;
    theNote2 = startingNote;
    //OK, but it's still not a frequency. Let's make it one with the mtof function that's built into LEAF. We pass "theNote" in as input and get a frequency out, which we then use to set the value of myFreq.
    myFreq = mtof(theNote);
    myFreq2 = mtof(theNote2);

    // now pass myFreq as the value we pass to the setFreq function on the mySine object
    tCycle_setFreq(&mySine, myFreq);
    tCycle_setFreq(&mySine2, myFreq2);

    //now here's the per sample stuff
    for (size_t i = 0; i < size; i++)
    {
        //get the next sample from the sine wave object mySine and store it in a temporary variable of type float, called "mySample"
        float mySample1 = tCycle_tick(&mySine);
        float mySample2 = tCycle_tick(&mySine2);

        //now set the audio outputs (left is [0] and right is [1] to be whatever value has been computed and stored in the mySample variable.
        out[0][i] = (mySample1*.5) + (mySample2*.5);
        out[1][i] = (mySample1*.5) + (mySample2*.5);
    }
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
    tCycle_init(&mySine2, &leaf);
    tCycle_setFreq(&mySine, 440.0f); //this will be written over by the audio callback loop immediately when we read in that knob value, so we could remove it without any problem.
    tCycle_setFreq(&mySine2, 220.0f);

    //blink that light! this endless while loop will be interrupted by the audio callback when the audio chip needs new data (every 4 samples, because audio block size was set to 4 above)
    while(1) {
        hw.seed.SetLed(1); //turn on the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
        hw.seed.SetLed(0); //turn off the red led on the Seed
        hw.DelayMs(500); //wait 500 milliseconds (0.5 seconds)
    }
}



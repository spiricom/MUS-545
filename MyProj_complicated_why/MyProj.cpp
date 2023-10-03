#include "daisy_pod.h"
#include "leaf.h"
using namespace daisy;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;
//let's define a global variable to store the frequency so we can easily mess with it in our program.
float myFreq = 220.0f;
//let's store the semitone offsets for the notes of a major (counting semitones from the tonic for each note)
//we are storing it in an "array" of integers, which we define by using the [] square bracket notation in the name
// majorScale[8], and the number in the square brackets means how many integers in the array.
// then we can, in the same line if we want, set the values of the integers in the array by writing them in a list
// separated by commas, and framed by curly braces. ={x,y,z}  and end it with a semicolon as usual
int majorScale[8] = {0,2,4,5,7,9,11,12};

//COMPLICATED WHY WOULD YOU DO THAT
//this example uses a knob to play a scale instead of just a glissando.
// I wanted to demonstrate using midi-to-frequency conversion and arrays.


float randomNumber()
{
    return Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    
    //let's make a temporary variable to figure out what note to play. 
    //how about knob1 sets which note of the major scale, and knob2 sets what the root of the scale is (offsets it). 
    //we can calculate things using midi notes (just treating the piano notes as numbers). Then we can use a midi-to-frequency function
    //to turn that into frequencies, which the tCycle object needs. 

    //let's make a variable we'll use as the midi note value.
    int theNote;

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
    theNote = startingNote + scaleNote;

    //OK, but it's still not a frequency. Let's make it one with the mtof function that's built into LEAF. We pass "theNote" in as input and get a frequency out, which we then use to set the value of myFreq.
    myFreq = mtof(theNote);

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
    Random::Init();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    LEAF_init(&leaf, 48000, leafMemory, 65535, randomNumber);
    tCycle_init(&mySine, &leaf);

    float blueness = 0.0f;

    //this time, let's do something stranger with the LEDs. 
    while(1) {
        //set the red value of led1 to knob1 value, the green value to knob2 value, and the blue value to whatever "blueness" currently is 
        hw.led1.Set(hw.GetKnobValue(hw.KNOB_1), hw.GetKnobValue(hw.KNOB_2), blueness);
        //make led2 just be the inverse of each value, so when knob1 red is 0 knob2 red will be 1, and so on.
        hw.led2.Set(1.0f - hw.GetKnobValue(hw.KNOB_1), 1.0f - hw.GetKnobValue(hw.KNOB_2), 1.0f - blueness);
        blueness = blueness + 0.000005f; //increment blueness a little bit each loop
        //if blueness gets to the maximum (1) then loop back around to 0
        if (blueness > 1.0f)
        {
            blueness = 0.0f;
        }

        //the RGB leds on the pod need to be updated with the "update" function here to work
        hw.UpdateLeds();
    }
}



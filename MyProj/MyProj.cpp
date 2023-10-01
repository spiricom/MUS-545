#include "daisy_pod.h"
//#include "daisysp.h"
#include "leaf.h"
using namespace daisy;
//using namespace daisysp;
LEAF leaf;
char leafMemory[65535];
DaisyPod hw;
tCycle mySine;

float randomNumber()
{
	Random::GetFloat(0.0f, 1.0f);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++)
	{
		float mySample = tCycle_tick(&mySine);
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
	while(1) {
	    hw.led1.Set(0.0, 0.0, 1.0);
        hw.UpdateLeds();
	}
}


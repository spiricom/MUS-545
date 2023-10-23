#include "daisy_pod.h"
//#include "daisysp.h"
#include "leaf-analysis.h"
#include "leaf.h"

using namespace daisy;
//using namespace daisysp;

DaisyPod hw;
LEAF leaf;
char leafMemory [65535];
tCycle mySineRoot;
tCycle mySineThird;
tCycle mySineFifth;

float myFreq = 220.0f;

float myFreq2 = 220.0f;

float myFreq3 = 200.0f;


volatile float _knob = 0.f;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();

	myFreq = 220.f + (hw.GetKnobValue(hw.KNOB_1) * 1000.f);

	myFreq2 = 220.f + (hw.GetKnobValue(hw.KNOB_2) * 1000.f);

	tCycle_setFreq(&mySineRoot, myFreq);
	tCycle_setFreq(&mySineThird, myFreq2);
	

	if (hw.button1.Pressed())
	{
		tCycle_setFreq(&mySineFifth, 500.0f);
		
	
	}
	else if (hw.button2.Pressed())
	{
		tCycle_setFreq(&mySineFifth, 900.0f);

	}
	else
	{
		tCycle_setFreq(&mySineFifth, 200.0f);
	}

	for (size_t i = 0; i < size; i++)
	{
		float sample = tCycle_tick(&mySineRoot) * 0.33;
		float sample2 = tCycle_tick(&mySineThird) * 0.33;
		float sample3 = tCycle_tick(&mySineFifth) * 0.33;
		out[0][i] = sample + sample2 + sample3;
		out[1][i] = sample + sample2 + sample3;
	}

}	

float _random(void)
{
	return 0.0;
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAdc();
	LEAF_init(&leaf, 48000.0, leafMemory, 65535, _random);
	tCycle_init (&mySineRoot, &leaf);

	tCycle_init (&mySineThird, &leaf);

	tCycle_init (&mySineFifth, &leaf);

	hw.StartAudio(AudioCallback);
	while(1) {
		hw.seed.SetLed(1);
		hw.DelayMs(200);
		hw.seed.SetLed(0);
		hw.DelayMs(200);
	}
}







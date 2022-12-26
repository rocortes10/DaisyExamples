#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed   hw;
Flanger     flanger1;
ReverbSc    reverb;
Switch      button1;
Switch      button2;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
        //Debounce the button
        button1.Debounce();
        button2.Debounce();

        if(!button1.Pressed())
            out[0][i] = in[0][i];
        else
            out[0][i] = flanger1.Process(in[0][i]);

        if(!button2.Pressed())
            out[1][i] = in[1][i];
        else
            out[1][i] = flanger1.Process(in[1][i]);
	}
}

//Interleaved
static void AudioCallback2(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        //Debounce the button
        button1.Debounce();
        button2.Debounce();

        float in1 = in[i];
        float in2 = in[i + 1];
        if(button1.Pressed())
        {
            reverb.Process(in1, in2, &out[i], &out[i + 1]);
            out[i] += in1;
            out[i+1] += in2;
        }
        else
        {
            out[i] = in1;
            out[i+1] = in2;
        }

    }
}


int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	float sample_rate = hw.AudioSampleRate();

    //Set button to pin 27 and 28, to be updated at a 1kHz samplerate
    button1.Init(hw.GetPin(27), 1000);
    button2.Init(hw.GetPin(28), 1000);

    //Flanger
    flanger1.Init(sample_rate);
    flanger1.SetLfoDepth(0.8f);
    flanger1.SetLfoFreq(.33f);
    flanger1.SetFeedback(.93f);

    //Reverb
    reverb.Init(sample_rate);
    reverb.SetFeedback(0.6f);
    reverb.SetLpFreq(18000.0f);

	hw.StartAudio(AudioCallback2);
	while(1) {}
}

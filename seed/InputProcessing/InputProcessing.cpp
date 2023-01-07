#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed   hw;
Flanger     flanger1;
ReverbSc    reverb;
Switch      button1;
Switch      button2;

bool toggled;
int counter;
#define BUFF_SIZE 1048576
float DSY_SDRAM_BSS buffer[BUFF_SIZE];

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

        if(i < BUFF_SIZE)
        {
            buffer[i] = in[0][i];
        }
	}
}

//Interleaved
static void AudioCallback2(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    button2.Debounce();
    if(button2.RisingEdge())
    {
        toggled = !toggled;
    }

    if(toggled)
    {
        for(size_t i = 0; i < size; i += 2)
        {
            float in1 = in[i];
            float in2 = in[i + 1];
            reverb.Process(in1, in2, &out[i], &out[i + 1]);
            out[i] += in1;
            out[i+1] += in2;

            if(i < BUFF_SIZE/2-1)
            {
                buffer[i] = in[i];
                buffer[i+1] = in[i+1];
            }
        }
    }
    else
    {
        for(size_t i = 0; i < size; i += 2)
        {
            out[i] = in[i];
            out[i+1] = in[i+1];

            if(i < BUFF_SIZE/2-1)
            {
                //out[i] += buffer[i]*0.8f;
                //out[i+1] += buffer[i+1]*0.8f;
            }
        }

    }

}

float gFrequency = 440.0;	// Frequency of the sine wave in Hz
float gAmplitude = 0.6;		// Amplitude of the sine wave (1.0 is maximum)
float gDuration = 10.0;		// Length of file to generate, in seconds
int gNumSamples = 0;

void calculate_sine(int numSamples, float sampleRate)
{
	// Generate the sine wave sample-by-sample for the whole duration
	for (int n = 0; n < numSamples; n++) {
	    // Calculate one sample of the sine wave
		float out = gAmplitude * sin(2.0 * M_PI * n * gFrequency / sampleRate);
		
		// Store the sample in the buffer
		buffer[n] = out;
	}
}

size_t pos_now;
size_t pos_end;
//SINE
static void AudioCallback3(/*AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	for (size_t i = 0; i < size; i++)
    {
        out[0][i]=buffer[pos_now];
        out[1][i]=buffer[pos_now];

        if(pos_now<BUFF_SIZE)
            pos_now++;
        else
            pos_now=0;
    }*/

    AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    button1.Debounce();
    if(button1.RisingEdge())
    {
        toggled = 0;
        pos_now = 0;
        pos_end = 0;
    }


    button2.Debounce();
    if(button2.RisingEdge())
    {
        toggled = !toggled;
        pos_now = 0;
    }

    if(toggled)
    {
        for(size_t i = 0; i < size; i += 2)
        {
            out[i] = in[i];
            out[i+1] = in[i + 1];

            buffer[pos_now] = in[i];
            buffer[pos_now+1] = in[i+1];

            if(pos_now < BUFF_SIZE/2-1)
                pos_now++;
            else
                pos_now=0;

            if(pos_end < BUFF_SIZE/2-1)
                pos_end++;
        }
    }
    else
    {
        for(size_t i = 0; i < size; i += 2)
        {
            if(pos_end != 0)
            {
                out[i] = buffer[pos_now];
                out[i+1] = buffer[pos_now+1];

                if(pos_now < pos_end)
                    pos_now++;
                else
                    pos_now=0;
            }
            else
            {
                out[i] = in[i];
                out[i+1] = in[i+1];
            }
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
    button1.Init(hw.GetPin(27), 1000, Switch::TYPE_TOGGLE, Switch::POLARITY_INVERTED, Switch::PULL_UP);
    button2.Init(hw.GetPin(28), 1000, Switch::TYPE_TOGGLE, Switch::POLARITY_INVERTED, Switch::PULL_UP);

    for(size_t i=0; i<BUFF_SIZE; i++)
        buffer[i] = 0;
    //Flanger
    flanger1.Init(sample_rate);
    flanger1.SetLfoDepth(0.8f);
    flanger1.SetLfoFreq(.33f);
    flanger1.SetFeedback(.93f);

    //Reverb
    reverb.Init(sample_rate);
    reverb.SetFeedback(0.6f);
    reverb.SetLpFreq(18000.0f);

    gNumSamples = gDuration * sample_rate;
    //calculate_sine(gNumSamples, sample_rate);
	hw.StartAudio(AudioCallback3);
	while(1) {}
}

#include "SimpleSidMixer.h"

#include <sidplayfp/sidplayfp.h>

#include <cmath>
#include <cstdint>

static constexpr float VOLUME_FACTOR_1SID = 1.0f;
static const float VOLUME_FACTOR_2SID = 1.0f / std::sqrt(2.0f);
static const float VOLUME_FACTOR_3SID = 1.0f / std::sqrt(3.0f);

SimpleSidMixer::SimpleSidMixer(sidplayfp& sidEngine, unsigned int outChannels) :
	_sidEngine(sidEngine),
	_numSidChips(_sidEngine.installedSIDs()),
	_sidVolumeFactor((_numSidChips == 1) ? VOLUME_FACTOR_1SID : (_numSidChips == 2) ? VOLUME_FACTOR_2SID : VOLUME_FACTOR_3SID),
	_outChannels(outChannels)
{
	_sidEngine.buffers(_sidChipsBuffers);
}

void SimpleSidMixer::FillBuffer(void* buffer, unsigned long framesPerBuffer)
{
	short* const out = static_cast<short*>(buffer);
	const unsigned long capacity = (_outChannels == 2) ? framesPerBuffer << 1 : framesPerBuffer; // *2 for stereo

	unsigned int written = 0;
	while (written < capacity)
	{
		_samplesLen = (_samplesLen == 0) ? _sidEngine.play(LIBSIDPLAYFP_PLAY_CYCLES) : _samplesLen;

		for (; _samplesPos < _samplesLen; ++_samplesPos)
		{
			for (unsigned int channel = 0; channel < _outChannels; ++channel)
			{
				const float panning = _channelMatrix[_numSidChips][0][channel];
				int_least32_t tmp = std::lrintf(_sidChipsBuffers[0][_samplesPos] * _sidVolumeFactor) * panning; // SID 1

				for (unsigned int chip = 1; chip < _numSidChips; ++chip) // SID 2 & 3 (if used)
				{
					const float panning = _channelMatrix[_numSidChips][chip][channel];
					tmp += std::lrintf(_sidChipsBuffers[chip][_samplesPos] * _sidVolumeFactor) * panning;
				}

				out[written++] = static_cast<short>(tmp);

				if (written == capacity) [[unlikely]]
				{
					++_samplesPos;
					return; // overflow pending
				}
			}
		}

		_samplesPos = 0;
		_samplesLen = 0;
	}
}

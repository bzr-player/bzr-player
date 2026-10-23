#pragma once

#include <array>

class sidplayfp;

class SimpleSidMixer
{
public:
	static constexpr unsigned int LIBSIDPLAYFP_PLAY_CYCLES = 3000; // Roughly 3ms buffer granularity.

public:
	SimpleSidMixer() = delete;
	SimpleSidMixer(SimpleSidMixer&) = delete;
	SimpleSidMixer& operator=(const SimpleSidMixer&) = delete;

	explicit SimpleSidMixer(sidplayfp& sidEngine, unsigned int outChannels);

public:
	void FillBuffer(void* buffer, unsigned long framesPerBuffer);

private:
	sidplayfp& _sidEngine;

	const unsigned int _numSidChips = 0;
	const float _sidVolumeFactor = 0;
	const unsigned int _outChannels = 0;
	short* _sidChipsBuffers[3];

	int _samplesPos = 0;
	int _samplesLen = 0;

	const std::array<std::array<std::array<float, 2>, 3>, 4> _channelMatrix =
	{{
		{{ // Dummy offset (zero-based array)
			{{1.0f, 1.0f}},
			{{1.0f, 1.0f}},
			{{1.0f, 1.0f}}
		}},
		{{ // 1SID
			{{1.0f, 1.0f}}, // SID 1
			{{1.0f, 1.0f}}, // dummy SID 2
			{{1.0f, 1.0f}}  // dummy SID 3
		}},
		{{ // 2SID
			{{1.0, 0.5}},  // SID 1
			{{0.5, 1.0}},  // SID 2
			{{1.0f, 1.0f}} // dummy SID 3
		}},
		{{ // 3SID
			{{1.0, 0.5}}, // SID 1
			{{1.0, 1.0}}, // SID 2
			{{0.5, 1.0}}  // SID 3
		}}
	}};
};

#include <cstring>
#include <fstream>
#include "jxs.h"
#include "fmod_errors.h"
#include "info.h"
#include "logger.h"
#include "plugins.h"

static FMOD_RESULT F_CALL open(FMOD_CODEC_STATE *codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo);

static FMOD_RESULT F_CALL close(FMOD_CODEC_STATE *codec);

static FMOD_RESULT F_CALL read(FMOD_CODEC_STATE *codec, void *buffer, unsigned int size, unsigned int *read);

static FMOD_RESULT F_CALL getLength(FMOD_CODEC_STATE *codec, unsigned int *length, FMOD_TIMEUNIT lengthtype);

static FMOD_RESULT F_CALL setPosition(FMOD_CODEC_STATE *codec, int subsound, unsigned int position,
                                      FMOD_TIMEUNIT postype);

FMOD_CODEC_DESCRIPTION codecDescription =
{
    FMOD_CODEC_PLUGIN_VERSION,
    PLUGIN_jaytrax_NAME, // name.
    0x00010000, // version 0xAAAABBBB   A = major, B = minor.
    1, // whether or not force everything using this codec to be a stream
    // the time formats we would like to accept into setposition/getposition
    FMOD_TIMEUNIT_MS,
    &open, // open callback
    &close, // close callback.
    &read, // read callback
    // getlength callback (If not specified FMOD returns the length in FMOD_TIMEUNIT_PCM, FMOD_TIMEUNIT_MS or FMOD_TIMEUNIT_PCMBYTES units based on the lengthpcm member of the FMOD_CODEC structure)
    &getLength,
    &setPosition, // setposition callback
    // getposition callback (only used for timeunit types that are not FMOD_TIMEUNIT_PCM, FMOD_TIMEUNIT_MS and FMOD_TIMEUNIT_PCMBYTES)
    nullptr,
    nullptr, // sound create callback (don't need it)
    nullptr // getwaveformat
};

class pluginJaytrax {
    FMOD_CODEC_STATE *_codec;

public:
    pluginJaytrax(FMOD_CODEC_STATE *codec) {
        _codec = codec;
        memset(&waveformat, 0, sizeof(waveformat));
    }

    static uint32_t msToSamples(const uint32_t ms) {
        const uint64_t samples = static_cast<uint64_t>(ms) * sampleRate * channels / 1000;
        return static_cast<uint32_t>(samples);
    }

    ~pluginJaytrax() {
    }

    static constexpr unsigned int sampleRate = 44100;
    static constexpr unsigned int channels = 2;
    Info *info;
    FMOD_CODEC_WAVEFORMAT waveformat;
    JT1Player *jay;
    int32_t songLength;
    uint32_t renderingPosition = 0;
};

/*
    FMODGetCodecDescription is mandatory for every fmod plugin. This is the symbol the registerplugin function searches for.
    Must be declared with F_API to make it export as stdcall.
    MUST BE EXTERN'ED AS C! C++ functions will be mangled incorrectly and not load in fmod.
*/
#ifdef __cplusplus
extern "C" {
#endif

F_EXPORT FMOD_CODEC_DESCRIPTION * F_CALL FMODGetCodecDescription() {
    return &codecDescription;
}

#ifdef __cplusplus
}
#endif

static FMOD_RESULT F_CALL open(FMOD_CODEC_STATE *codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo) {
    logDebug("Try", PLUGIN_jaytrax_NAME);

    auto *plugin = new pluginJaytrax(codec);
    plugin->info = static_cast<Info *>(userexinfo->userdata);

    JT1Song *song;

    if (jxsfile_readSongMem(plugin->info->fileBuffer, plugin->info->filesize, &song) != 0) {
        delete plugin;
        return FMOD_ERR_FORMAT;
    }

    plugin->jay = jaytrax_init();

    if (!plugin->jay) {
        delete plugin;
        return FMOD_ERR_FORMAT;
    }

    plugin->jay->song = song;

    string filename = plugin->info->userPath + PLUGIN_CONFIGS_DIR "/" CONFIG_FILENAME;
    ifstream ifs(filename.c_str());
    bool useDefaults = false;

    if (ifs.fail()) {
        // the file could not be opened
        useDefaults = true;
    }

    // defaults
    plugin->info->isContinuousPlaybackActive = false;

    if (!useDefaults) {
        string line;
        while (getline(ifs, line)) {
            auto i = line.find_first_of('=');
            if (i == string::npos) continue;

            string word = line.substr(0, i);
            string value = line.substr(i + 1);

            if (word == "continuousPlayback") {
                plugin->info->isContinuousPlaybackActive = plugin->info->isPlayModeRepeatSongEnabled && value == "true";
            }
        }

        ifs.close();
    }

    plugin->waveformat.format = FMOD_SOUND_FORMAT_PCM16;
    plugin->waveformat.channels = pluginJaytrax::channels;
    plugin->waveformat.frequency = pluginJaytrax::sampleRate;
    plugin->waveformat.pcmblocksize = plugin->waveformat.format * plugin->waveformat.channels;
    plugin->waveformat.lengthpcm = -1;

    codec->waveformat = &plugin->waveformat;
    codec->numsubsounds = 0;
    // number of 'subsounds' in this sound.  For most codecs this is 0, only multi sound codecs such as FSB or CDDA have subsounds
    codec->plugindata = plugin; // user data value

    plugin->songLength = static_cast<int32_t>(
        static_cast<int64_t>(jaytrax_getLength(plugin->jay, plugin->info->currentSubsong, 1, pluginJaytrax::sampleRate))
        * 1000 / pluginJaytrax::sampleRate);

    jaytrax_changeSubsong(plugin->jay, plugin->info->currentSubsong);

    plugin->info->numSubsongs = plugin->jay->song->nrofsongs;
    plugin->info->plugin = PLUGIN_jaytrax;
    plugin->info->pluginName = PLUGIN_jaytrax_NAME;
    plugin->info->fileFormat = "Jaytrax";
    plugin->info->setSeekable(true);

    return FMOD_OK;
}

static FMOD_RESULT F_CALL close(FMOD_CODEC_STATE *codec) {
    const auto *plugin = static_cast<pluginJaytrax *>(codec->plugindata);

    if (plugin != nullptr) {
        jaytrax_free(plugin->jay);
    }

    delete plugin;
    return FMOD_OK;
}

static FMOD_RESULT F_CALL read(FMOD_CODEC_STATE *codec, void *buffer, unsigned int size, unsigned int *read) {
    auto *plugin = static_cast<pluginJaytrax *>(codec->plugindata);

    jaytrax_renderChunk(plugin->jay, static_cast<int16_t *>(buffer), static_cast<int32_t>(size),
                        pluginJaytrax::sampleRate, 0);
    plugin->renderingPosition += size;

    *read = size;
    return FMOD_OK;
}

static FMOD_RESULT F_CALL getLength(FMOD_CODEC_STATE *codec, unsigned int *length, FMOD_TIMEUNIT lengthtype) {
    const auto *plugin = static_cast<pluginJaytrax *>(codec->plugindata);

    if (lengthtype == FMOD_TIMEUNIT_MS_REAL) {
        *length = plugin->songLength;
        return FMOD_OK;
    }

    return FMOD_ERR_UNSUPPORTED;
}

static FMOD_RESULT F_CALL setPosition(FMOD_CODEC_STATE *codec, int subsound, unsigned int position,
                                      FMOD_TIMEUNIT postype) {
    auto *plugin = static_cast<pluginJaytrax *>(codec->plugindata);

    if (postype == FMOD_TIMEUNIT_MS) {
        bool shouldReinit;
        bool shouldSeek = false;
        uint32_t seekPosition = 0;

        if (position == 0) {
            shouldReinit = plugin->renderingPosition != 0;
        } else {
            seekPosition = pluginJaytrax::msToSamples(position) & ~1;
            shouldReinit = seekPosition < plugin->renderingPosition;
            shouldSeek = true;
        }

        if (shouldReinit) {
            jaytrax_changeSubsong(plugin->jay, plugin->info->currentSubsong);
            plugin->renderingPosition = 0;
        }

        if (shouldSeek) {
            const uint32_t samplesToSkip = seekPosition - plugin->renderingPosition;
            vector<short int> dummyBuffer(samplesToSkip);

            jaytrax_renderChunk(plugin->jay, dummyBuffer.data(),
                                static_cast<int>(samplesToSkip / pluginJaytrax::channels), pluginJaytrax::sampleRate,
                                1);
            plugin->renderingPosition += samplesToSkip;
        }

        return FMOD_OK;
    }

    return FMOD_ERR_UNSUPPORTED;
}

/*
 *  Copyright (C) 2011, 2012 Igalia S.L
 *  Copyright (C) 2014 Sebastian Dröge <sebastian@centricular.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioDestinationGStreamer.h"

#include "AudioSourceProvider.h"
#include "AudioUtilities.h"
#include "GStreamerCommon.h"
#include "GStreamerQuirks.h"
#include "WebKitWebAudioSourceGStreamer.h"
#include <gst/audio/gstaudiobasesink.h>
#include <gst/gst.h>
#include <wtf/PrintStream.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/glib/RunLoopSourcePriority.h>
#include <wtf/text/MakeString.h>
#include <wtf/text/StringToIntegerConversion.h>

namespace WebCore {

GST_DEBUG_CATEGORY(webkit_audio_destination_debug);
#define GST_CAT_DEFAULT webkit_audio_destination_debug

static void initializeAudioDestinationDebugCategory()
{
    ensureGStreamerInitialized();
    registerWebKitGStreamerElements();

    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        GST_DEBUG_CATEGORY_INIT(webkit_audio_destination_debug, "webkitaudiodestination", 0, "WebKit WebAudio Destination");
    });
}

static unsigned long maximumNumberOfOutputChannels()
{
    initializeAudioDestinationDebugCategory();

    static int count = 0;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        auto maxFromEnvironment = StringView::fromLatin1(g_getenv("WEBKIT_GST_MAX_NUMBER_OF_AUDIO_OUTPUT_CHANNELS"));
        if (!maxFromEnvironment.isEmpty()) {
            if (auto value = WTF::parseInteger<int>(maxFromEnvironment)) {
                count = *value;
                return;
            }
        }

        auto monitor = adoptGRef(gst_device_monitor_new());
        auto caps = adoptGRef(gst_caps_new_empty_simple("audio/x-raw"));
        gst_device_monitor_add_filter(monitor.get(), "Audio/Sink", caps.get());
        bool started = gst_device_monitor_start(monitor.get());
        auto* devices = gst_device_monitor_get_devices(monitor.get());
        while (devices) {
            auto device = adoptGRef(GST_DEVICE_CAST(devices->data));
            auto caps = adoptGRef(gst_device_get_caps(device.get()));
            unsigned size = gst_caps_get_size(caps.get());
            for (unsigned i = 0; i < size; i++) {
                auto* structure = gst_caps_get_structure(caps.get(), i);
                if (gstStructureGetName(structure) != "audio/x-raw"_s)
                    continue;
                if (auto value = gstStructureGet<int>(structure, "channels"_s))
                    count = std::max(count, *value);
            }
            devices = g_list_delete_link(devices, devices);
        }
        GST_DEBUG("maximumNumberOfOutputChannels: %d", count);
        if (started)
            gst_device_monitor_stop(monitor.get());
    });

    return count;
}

Ref<AudioDestination> AudioDestination::create(const CreationOptions& options)
{
    initializeAudioDestinationDebugCategory();
    // FIXME: make use of inputDeviceId as appropriate.

    // FIXME: Add support for local/live audio input.
    if (options.numberOfInputChannels)
        WTFLogAlways("AudioDestination::create(%u, %u, %f) - unhandled input channels", options.numberOfInputChannels, options.numberOfOutputChannels, options.sampleRate);

    return adoptRef(*new AudioDestinationGStreamer(options));
}

float AudioDestination::hardwareSampleRate()
{
    return 44100;
}

unsigned long AudioDestination::maxChannelCount()
{
    return maximumNumberOfOutputChannels();
}

AudioDestinationGStreamer::AudioDestinationGStreamer(const CreationOptions& options)
    : AudioDestination(options)
    , m_renderBus(AudioBus::create(options.numberOfOutputChannels, AudioUtilities::renderQuantumSize, false))
{
    static Atomic<uint32_t> pipelineId;
    m_pipeline = gst_pipeline_new(makeString("audio-destination-"_s, pipelineId.exchangeAdd(1)).ascii().data());
    registerActivePipeline(m_pipeline);
    connectSimpleBusMessageCallback(m_pipeline.get(), [this](GstMessage* message) {
        this->handleMessage(message);
    });

    m_src = GST_ELEMENT_CAST(g_object_new(WEBKIT_TYPE_WEB_AUDIO_SRC, "rate", options.sampleRate,
        "destination", this, "frames", AudioUtilities::renderQuantumSize, nullptr));

    webkitWebAudioSourceSetBus(WEBKIT_WEB_AUDIO_SRC(m_src.get()), m_renderBus);

    auto& quirksManager = GStreamerQuirksManager::singleton();
    GRefPtr<GstElement> audioSink = quirksManager.createWebAudioSink();
    m_audioSinkAvailable = audioSink;
    if (!audioSink) {
        GST_ERROR("Failed to create GStreamer audio sink element");
        return;
    }

    // Probe platform early on for a working audio output device in autoaudiosink.
    auto nameView = StringView::fromLatin1(GST_OBJECT_NAME(audioSink.get()));
    if (nameView.startsWith("autoaudiosink"_s)) {
        g_signal_connect(audioSink.get(), "child-added", G_CALLBACK(+[](GstChildProxy*, GObject* object, gchar*, gpointer) {
            if (GST_IS_AUDIO_BASE_SINK(object))
                g_object_set(GST_AUDIO_BASE_SINK(object), "buffer-time", static_cast<gint64>(100000), nullptr);
        }), nullptr);

        // Autoaudiosink does the real sink detection in the GST_STATE_NULL->READY transition
        // so it's best to roll it to READY as soon as possible to ensure the underlying platform
        // audiosink was loaded correctly.
        GstStateChangeReturn stateChangeReturn = gst_element_set_state(audioSink.get(), GST_STATE_READY);
        if (stateChangeReturn == GST_STATE_CHANGE_FAILURE) {
            GST_ERROR("Failed to change autoaudiosink element state");
            gst_element_set_state(audioSink.get(), GST_STATE_NULL);
            m_audioSinkAvailable = false;
            return;
        }
    }

    GstElement* audioConvert = makeGStreamerElement("audioconvert"_s);
    GstElement* audioResample = makeGStreamerElement("audioresample"_s);

    auto queue = gst_element_factory_make("queue", nullptr);
    g_object_set(queue, "max-size-buffers", 2, "max-size-bytes", 0, "max-size-time", static_cast<guint64>(0), nullptr);

    gst_bin_add_many(GST_BIN_CAST(m_pipeline.get()), m_src.get(), audioConvert, audioResample, queue, audioSink.get(), nullptr);

    // Link src pads from webkitAudioSrc to audioConvert ! audioResample ! [capsfilter !] queue ! autoaudiosink.
    gst_element_link_pads_full(m_src.get(), "src", audioConvert, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(audioConvert, "src", audioResample, "sink", GST_PAD_LINK_CHECK_NOTHING);

    if (!webkitGstCheckVersion(1, 20, 4)) {
        // Force audio conversion to 'interleaved' format (by audioconvert element).
        // 1) Some platform sinks don't support non-interleaved audio without special caps (rialtowebaudiosink).
        // 2) Interaudio sink/src doesn't fully support non-interleaved audio (webkit audio sink)
        // 3) audiomixer doesn't support non-interleaved audio in output pipeline (webkit audio sink)
        GstElement* capsFilter = makeGStreamerElement("capsfilter"_s);
        GRefPtr<GstCaps> caps = adoptGRef(gst_caps_new_simple("audio/x-raw", "layout", G_TYPE_STRING, "interleaved", nullptr));
        g_object_set(capsFilter, "caps", caps.get(), nullptr);
        gst_bin_add(GST_BIN_CAST(m_pipeline.get()), capsFilter);
        gst_element_link_pads_full(audioResample, "src", capsFilter, "sink", GST_PAD_LINK_CHECK_NOTHING);
        gst_element_link_pads_full(capsFilter, "src", queue, "sink", GST_PAD_LINK_CHECK_NOTHING);
    } else
        gst_element_link_pads_full(audioResample, "src", queue, "sink", GST_PAD_LINK_CHECK_NOTHING);

    gst_element_link_pads_full(queue, "src", audioSink.get(), "sink", GST_PAD_LINK_CHECK_NOTHING);
}

AudioDestinationGStreamer::~AudioDestinationGStreamer()
{
    GST_DEBUG_OBJECT(m_pipeline.get(), "Disposing");
    if (m_src) [[likely]]
        g_object_set(m_src.get(), "destination", nullptr, nullptr);
    unregisterPipeline(m_pipeline);
    disconnectSimpleBusMessageCallback(m_pipeline.get());
    gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);
    notifyStopResult(true);
}

unsigned AudioDestinationGStreamer::framesPerBuffer() const
{
    return AudioUtilities::renderQuantumSize;
}

bool AudioDestinationGStreamer::handleMessage(GstMessage* message)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        notifyIsPlaying(false);
        break;
    case GST_MESSAGE_LATENCY:
        gst_bin_recalculate_latency(GST_BIN_CAST(m_pipeline.get()));
        break;
    default:
        break;
    }
    return true;
}

void AudioDestinationGStreamer::start(Function<void(Function<void()>&&)>&& dispatchToRenderThread, CompletionHandler<void(bool)>&& completionHandler)
{
    webkitWebAudioSourceSetDispatchToRenderThreadFunction(WEBKIT_WEB_AUDIO_SRC(m_src.get()), WTFMove(dispatchToRenderThread));
    startRendering(WTFMove(completionHandler));
}

void AudioDestinationGStreamer::startRendering(CompletionHandler<void(bool)>&& completionHandler)
{
    ASSERT(m_audioSinkAvailable);
    m_startupCompletionHandler = WTFMove(completionHandler);
    GST_DEBUG_OBJECT(m_pipeline.get(), "Starting audio rendering, sink %s", m_audioSinkAvailable ? "available" : "not available");

    if (m_isPlaying) {
        notifyStartupResult(true);
        return;
    }

    if (!m_audioSinkAvailable) {
        notifyStartupResult(false);
        return;
    }

    notifyStartupResult(webkitGstSetElementStateSynchronously(m_pipeline.get(), GST_STATE_PLAYING, [this](GstMessage* message) -> bool {
        return handleMessage(message);
    }));
}

void AudioDestinationGStreamer::stop(CompletionHandler<void(bool)>&& completionHandler)
{
    stopRendering(WTFMove(completionHandler));
    webkitWebAudioSourceSetDispatchToRenderThreadFunction(WEBKIT_WEB_AUDIO_SRC(m_src.get()), nullptr);
}

void AudioDestinationGStreamer::stopRendering(CompletionHandler<void(bool)>&& completionHandler)
{
    ASSERT(m_audioSinkAvailable);
    m_stopCompletionHandler = WTFMove(completionHandler);
    GST_DEBUG_OBJECT(m_pipeline.get(), "Stopping audio rendering, sink %s", m_audioSinkAvailable ? "available" : "not available");

    if (!m_isPlaying) {
        GST_DEBUG_OBJECT(m_pipeline.get(), "Already stopped");
        notifyStopResult(true);
        return;
    }

    if (!m_audioSinkAvailable) {
        notifyStopResult(false);
        return;
    }

    notifyStopResult(webkitGstSetElementStateSynchronously(m_pipeline.get(), GST_STATE_READY, [this](GstMessage* message) -> bool {
        return handleMessage(message);
    }));
}

void AudioDestinationGStreamer::notifyStartupResult(bool success)
{
    if (success)
        notifyIsPlaying(true);

    callOnMainThreadAndWait([this, completionHandler = WTFMove(m_startupCompletionHandler), success]() mutable {
#ifdef GST_DISABLE_GST_DEBUG
        UNUSED_VARIABLE(this);
#endif
        GST_DEBUG_OBJECT(m_pipeline.get(), "Has start completion handler: %s", boolForPrinting(!!completionHandler));
        if (completionHandler)
            completionHandler(success);
    });
}

void AudioDestinationGStreamer::notifyStopResult(bool success)
{
    if (success)
        notifyIsPlaying(false);

    callOnMainThreadAndWait([this, completionHandler = WTFMove(m_stopCompletionHandler), success]() mutable {
#ifdef GST_DISABLE_GST_DEBUG
        UNUSED_VARIABLE(this);
#endif
        GST_DEBUG_OBJECT(m_pipeline.get(), "Has stop completion handler: %s", boolForPrinting(!!completionHandler));
        if (completionHandler)
            completionHandler(success);
    });
}

void AudioDestinationGStreamer::notifyIsPlaying(bool isPlaying)
{
    if (m_isPlaying == isPlaying)
        return;

    GST_DEBUG("Is playing: %s", boolForPrinting(isPlaying));
    m_isPlaying = isPlaying;
    if (m_callback)
        m_callback->isPlayingDidChange();
}

#undef GST_CAT_DEFAULT

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)

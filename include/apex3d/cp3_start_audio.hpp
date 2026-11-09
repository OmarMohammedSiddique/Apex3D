#pragma once
#include "generated/start_cues.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#endif

namespace apex3d::audio {

class CueMixer {
public:
    void trigger(int cue) { cue_ = cue; position_ = 0; }
    void render(float* output, std::size_t count, bool muted)
    {
        if (muted) cue_ = 0;
        for (std::size_t i = 0; i < count; ++i) {
            const auto* samples = cue_ == 2 ? goCue.data() : redCue.data();
            const auto length = cue_ == 2 ? goCue.size() : redCue.size();
            output[i] = cue_ != 0 && position_ < length ? samples[position_++] / 32768.0F * 0.22F : 0.0F;
        }
    }
private:
    int cue_{};
    std::size_t position_{};
};

// Short CC0 sample cues only. No engine sound, race audio or background loop.
class StartAudio {
public:
    StartAudio() = default;
    StartAudio(const StartAudio&) = delete;
    StartAudio& operator=(const StartAudio&) = delete;
    ~StartAudio() { stop(); }
    void mute(bool muted) { muted_.store(muted); }
    void trigger(int cue) { if (!muted_.load()) pending_.store(cue); }
    bool start()
    {
#ifdef __APPLE__
        AudioStreamBasicDescription format{};
        format.mSampleRate = 24000;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
        format.mBytesPerPacket = sizeof(float);
        format.mFramesPerPacket = 1;
        format.mBytesPerFrame = sizeof(float);
        format.mChannelsPerFrame = 1;
        format.mBitsPerChannel = 32;
        if (AudioQueueNewOutput(&format, callback, this, nullptr, nullptr, 0, &queue_) != noErr) {
            stop(); return false;
        }
        for (int i = 0; i < 3; ++i) {
            AudioQueueBufferRef buffer{};
            if (AudioQueueAllocateBuffer(queue_, 512 * sizeof(float), &buffer) != noErr) {
                stop(); return false;
            }
            fill(buffer);
            if (AudioQueueEnqueueBuffer(queue_, buffer, 0, nullptr) != noErr) {
                stop(); return false;
            }
        }
        if (AudioQueueStart(queue_, nullptr) == noErr) return true;
        stop();
#endif
        return false;
    }
private:
    void stop()
    {
#ifdef __APPLE__
        if (queue_) { AudioQueueStop(queue_, true); AudioQueueDispose(queue_, true); queue_ = nullptr; }
#endif
    }
#ifdef __APPLE__
    void fill(AudioQueueBufferRef buffer)
    {
        if (const int cue = pending_.exchange(0)) mixer_.trigger(cue);
        const auto count = buffer->mAudioDataBytesCapacity / sizeof(float);
        mixer_.render(static_cast<float*>(buffer->mAudioData), count, muted_.load());
        buffer->mAudioDataByteSize = static_cast<UInt32>(count * sizeof(float));
    }
    static void callback(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer)
    {
        static_cast<StartAudio*>(context)->fill(buffer);
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }
    AudioQueueRef queue_{};
    CueMixer mixer_;
#endif
    std::atomic<bool> muted_{true};
    std::atomic<int> pending_{};
};
} // namespace apex3d::audio

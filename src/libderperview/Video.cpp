#include "Video.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

extern "C"
{
    #include "libswscale/swscale.h"
}

using namespace DerperView;
using namespace std;

int SetupContextWorker(AVFormatContext *formatContext, AVCodecContext **codecContext, AVMediaType type, ostream& outputStream)
{
    auto result = av_find_best_stream(formatContext, type, -1, -1, nullptr, 0);
    if (result < 0)
    {
        outputStream << "Could not find stream for " << av_get_media_type_string (type) << ": " << GetErrorString(result) << endl;
        return result;
    }

    int streamIndex = result;

    AVStream *stream = formatContext->streams[streamIndex];
    const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder)
    {
        outputStream << "Failed to find codec: " << av_get_media_type_string(type) << ": " << GetErrorString(result) << endl;
        return -666;
    }

    *codecContext = avcodec_alloc_context3(decoder);
    if (!*codecContext)
    {
        outputStream << "Failed to allocation codec context: " << av_get_media_type_string(type) << ": " << GetErrorString(result) << endl;
        return -666;
    }

    avcodec_parameters_to_context(*codecContext, stream->codecpar);

    result = avcodec_open2(*codecContext, decoder, nullptr);
    if (result < 0)
    {
        outputStream << "Failed to open codec: " << av_get_media_type_string(type) << ": " << GetErrorString(result) << endl;
        return -666;
    }
    (*codecContext)->time_base = stream->time_base;

    return streamIndex;
}

int SetupVideoContext(AVFormatContext *formatContext, AVCodecContext **codecContext, ostream& outputStream)
{
    return SetupContextWorker(formatContext, codecContext, AVMediaType::AVMEDIA_TYPE_VIDEO, outputStream);
}

int SetupAudioContext(AVFormatContext *formatContext, AVCodecContext **codecContext, ostream& outputStream)
{
    return SetupContextWorker(formatContext, codecContext, AVMediaType::AVMEDIA_TYPE_AUDIO, outputStream);
}

InputVideoFile::InputVideoFile(string filename, ostream& outputStream) :
    filename_(filename),
    outputStream_(outputStream),
    formatContext_(nullptr), videoCodecContext_(nullptr), audioCodecContext_(nullptr),
    videoStreamIndex_(-1), audioStreamIndex_(-1),
    frame_(nullptr), packet_(nullptr), draining_(false), lastError_(0),
    videoFrameCount_(0)
{
#if LIBAVFORMAT_VERSION_MAJOR < 58
    // need to register all muxers, decoders, ... on ffmpeg versions before 58.x, see
    // documentation and https://stackoverflow.com/questions/51604582/ffmpeg-and-docker-result-in-averror-invaliddata/51624309 
    av_register_all();
#endif

    lastError_ = avformat_open_input(&formatContext_, filename_.c_str(), nullptr, nullptr);
    if (lastError_ < 0)
    {
        outputStream << "Error on opening file '" << filename_ << "': " << GetErrorString(lastError_) << endl;
        return;
    }

    lastError_ = avformat_find_stream_info(formatContext_, nullptr);
    if (lastError_ < 0) {
        outputStream << "Could not read streams: " << GetErrorString(lastError_) << endl;
        return;
    }

    videoStreamIndex_ = SetupVideoContext(formatContext_, &videoCodecContext_, outputStream_);
    audioStreamIndex_ = SetupAudioContext(formatContext_, &audioCodecContext_, outputStream_);

    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
}

InputVideoFile::~InputVideoFile()
{
    av_frame_free(&frame_);
    av_packet_free(&packet_);
    avcodec_free_context(&videoCodecContext_);
    avcodec_free_context(&audioCodecContext_);
    avformat_close_input(&formatContext_);
    avformat_free_context(formatContext_);
}

void InputVideoFile::Dump()
{
    // Stream index seems to be ignored here
    av_dump_format(formatContext_, 0, filename_.c_str(), 0);
}

AVFrame *InputVideoFile::GetNextFrame()
{
    if (draining_)
        return GetNextDrainFrame();

    lastError_ = AVERROR(EAGAIN);
    while (lastError_ == AVERROR(EAGAIN))
    {
        lastError_ = av_read_frame(formatContext_, packet_);
        if (lastError_ >= 0)
        {
            if (packet_->stream_index == videoStreamIndex_)
            {
                lastError_ = avcodec_send_packet(videoCodecContext_, packet_);
                if (lastError_ < 0)
                {
                    outputStream_ << "*** Error sending video packet to decoder: " << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
                lastError_ = avcodec_receive_frame(videoCodecContext_, frame_);
                if (lastError_ < 0 && lastError_ != AVERROR(EAGAIN))
                {
                    outputStream_ << "*** Error getting video frame from decoder: " << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
            }
            else if (packet_->stream_index == audioStreamIndex_)
            {
                lastError_ = avcodec_send_packet(audioCodecContext_, packet_);
                if (lastError_ < 0)
                {
                    outputStream_ << "*** Error sending audio packet to decoder" << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
                lastError_ = avcodec_receive_frame(audioCodecContext_, frame_);
                if (lastError_ < 0 && lastError_ != AVERROR(EAGAIN))
                {
                    outputStream_ << "*** Error getting audio frame from decoder: " << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
            }
            else
            {
                // Some wacky stream (GoPro has a 3rd stream for data, for example ...)
                // Discard it.
                lastError_ = AVERROR(EAGAIN);
            }
        }
        else if (lastError_ == AVERROR_EOF)
        {
            // Begin draining
            draining_ = true;
            lastError_ = avcodec_send_packet(videoCodecContext_, nullptr);
            if (audioCodecContext_ != nullptr)
                lastError_ = avcodec_send_packet(audioCodecContext_, nullptr);
            return GetNextDrainFrame();
        }
        av_packet_unref(packet_);
    }

    return frame_;
}

AVFrame *InputVideoFile::GetNextDrainFrame()
{
    lastError_ = avcodec_receive_frame(videoCodecContext_, frame_);
    if (lastError_ != AVERROR_EOF)
        return frame_;
    if (audioCodecContext_ != nullptr)
    {
        lastError_ = avcodec_receive_frame(audioCodecContext_, frame_);
        if (lastError_ != AVERROR_EOF)
            return frame_;
    }
    return nullptr;
}

VideoInfo InputVideoFile::GetVideoInfo()
{
    VideoInfo v;
    if (audioCodecContext_ != nullptr)
    {
        v.audioBitRate = audioCodecContext_->bit_rate;
        v.audioChannels = audioCodecContext_->ch_layout.nb_channels;
        v.audioChannelLayout = audioCodecContext_->ch_layout;
        v.audioSampleFormat = audioCodecContext_->sample_fmt;
        v.audioSampleRate = audioCodecContext_->sample_rate;
        v.audioTimeBase = audioCodecContext_->time_base;
    }
    v.bitRate = static_cast<int>(videoCodecContext_->bit_rate);
    v.frameRate = formatContext_->streams[videoStreamIndex_]->r_frame_rate;
    v.height = videoCodecContext_->height;
    v.pixelFormat = videoCodecContext_->pix_fmt;
    v.streamTimeBase = formatContext_->streams[videoStreamIndex_]->time_base;
    v.totalFrames = formatContext_->streams[videoStreamIndex_]->nb_frames;
    v.videoTimeBase = videoCodecContext_->time_base;
    v.width = videoCodecContext_->width;
    return v;
}

OutputVideoFile::OutputVideoFile(string filename, VideoInfo sourceInfo, ostream& outputStream) :
    filename_(filename),
    outputStream_(outputStream),
    formatContext_(nullptr), videoCodecContext_(nullptr), audioCodecContext_(nullptr),
    videoStream_(nullptr), audioStream_(nullptr),
    audioResampleContext_(nullptr),
    videoFrameCount_(0), audioFrameCount_(0), lastError_(0)
{
    lastError_ = avformat_alloc_output_context2(&formatContext_, nullptr, nullptr, filename.c_str());
    if (lastError_ < 0 || formatContext_ == nullptr)
    {
        outputStream_ << "Could not create format context: " << GetErrorString(lastError_) << endl;
        return;
    }

    const AVCodec *videoCodec = avcodec_find_encoder(formatContext_->oformat->video_codec);
    videoCodecContext_ = avcodec_alloc_context3(videoCodec);
    videoStream_ = avformat_new_stream(formatContext_, videoCodec);

    videoCodecContext_->profile = FF_PROFILE_H264_HIGH;
    videoCodecContext_->bit_rate = sourceInfo.bitRate;
    videoCodecContext_->width = sourceInfo.width;
    videoCodecContext_->height = sourceInfo.height;
    videoCodecContext_->gop_size = 12;
    videoCodecContext_->pix_fmt = sourceInfo.pixelFormat;
    videoCodecContext_->framerate = sourceInfo.frameRate;
    videoStream_->avg_frame_rate = sourceInfo.frameRate;
    videoStream_->r_frame_rate = sourceInfo.frameRate;

    videoCodecContext_->time_base = av_inv_q(sourceInfo.frameRate);
    //videoStream_->time_base = videoCodecContext_->time_base;
    //videoCodecContext_->time_base = sourceInfo.videoTimeBase;
    //videoStream_->time_base = sourceInfo.streamTimeBase;

    if (formatContext_->oformat->flags & AVFMT_GLOBALHEADER)
        videoCodecContext_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *opt = nullptr;
    lastError_ = avcodec_open2(videoCodecContext_, videoCodec, &opt);
    if (lastError_ < 0)
    {
        outputStream_ << "Error creating video codec" << endl;
        return;
    }
    av_dict_free(&opt);

    lastError_ = avcodec_parameters_from_context(videoStream_->codecpar, videoCodecContext_);
    if (lastError_ < 0)
        return;

    if (sourceInfo.audioChannels > 0)
    {
        const AVCodec* audioCodec = avcodec_find_encoder(formatContext_->oformat->audio_codec);
        audioCodecContext_ = avcodec_alloc_context3(audioCodec);
        audioStream_ = avformat_new_stream(formatContext_, audioCodec);

        // Populate codec data. Most of it comes from the audio source.
        // If the channel layout is missing, populate with a default based on number of channels
        // (seen in sowt codec on Runcam 5 Orange)
        audioCodecContext_->bit_rate = min(static_cast<int64_t>(128000), sourceInfo.audioBitRate);
        audioCodecContext_->sample_rate = sourceInfo.audioSampleRate;
        audioCodecContext_->sample_fmt = audioCodec->sample_fmts[0];
        av_channel_layout_default(&audioCodecContext_->ch_layout, sourceInfo.audioChannels);
        audioCodecContext_->time_base = sourceInfo.audioTimeBase;
        audioStream_->time_base = audioCodecContext_->time_base;

        opt = nullptr;
        lastError_ = avcodec_open2(audioCodecContext_, audioCodec, &opt);
        if (lastError_ < 0)
        {
            outputStream_ << "Error creating audio codec" << endl;
            return;
        }
        av_dict_free(&opt);

        lastError_ = avcodec_parameters_from_context(audioStream_->codecpar, audioCodecContext_);
        if (lastError_ < 0)
            return;
    }

    av_dump_format(formatContext_, 0, filename.c_str(), 1);

    lastError_ = avio_open(&formatContext_->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (lastError_ < 0)
    {
        outputStream_ << "Error opening output file" << endl;
        return;
    }
    lastError_ = avformat_write_header(formatContext_, &opt);
    if (lastError_ < 0)
    {
        outputStream_ << "Could not write container header: " << GetErrorString(lastError_) << endl;
    }

    // Create audio resampler, if needed
    if (audioCodecContext_ != nullptr)
    {
        if (sourceInfo.audioSampleFormat != audioCodecContext_->sample_fmt)
        {
            swr_alloc_set_opts2(
                &audioResampleContext_,
                &audioCodecContext_->ch_layout,
                audioCodecContext_->sample_fmt,
                audioCodecContext_->sample_rate,
                &sourceInfo.audioChannelLayout,
                sourceInfo.audioSampleFormat,
                sourceInfo.audioSampleRate,
                0, nullptr
            );

            lastError_ = swr_init(audioResampleContext_);
            if (lastError_ < 0)
            {
                outputStream_ << "Could not set up audio format resampling: " << GetErrorString(lastError_) << endl;
            }
        }
    }
}

OutputVideoFile::~OutputVideoFile()
{
    if (formatContext_->pb != nullptr)
    {
        av_write_trailer(formatContext_);
        avio_closep(&formatContext_->pb);
    }

    if (audioResampleContext_ != nullptr)
    {
        swr_free(&audioResampleContext_);
    }

    avcodec_free_context(&videoCodecContext_);
    avcodec_free_context(&audioCodecContext_);
    avformat_free_context(formatContext_);
}

int OutputVideoFile::WriteNextFrame(AVFrame *frame)
{
    AVCodecContext *codec = nullptr;
    AVStream *stream = nullptr;
    AVFrame *conversionFrame = nullptr;

    if (frame->width == 0)
    {
        codec = audioCodecContext_;
        stream = audioStream_;
        //frame->pts = av_rescale_q(frame->pts, AVRational { 1, audioCodecContext_->sample_rate }, audioCodecContext_->time_base);
        //frame->pts = audioFrameCount_;
        audioFrameCount_++;

        if (audioResampleContext_ != nullptr)
        {
            // the source layout may be 0. 0 is invalid input for the resampler. If we then pass
            // 0 in here, it complains that the format is incorrect, so we lie at both ends
            av_channel_layout_default(&frame->ch_layout, audioCodecContext_->ch_layout.nb_channels);

            conversionFrame = av_frame_alloc();
            conversionFrame->ch_layout = audioCodecContext_->ch_layout;
            conversionFrame->sample_rate = audioCodecContext_->sample_rate;
            conversionFrame->format = audioCodecContext_->sample_fmt;
            conversionFrame->pts = frame->pts;

            lastError_ = swr_convert_frame(audioResampleContext_, conversionFrame, frame);
            if (lastError_ < 0)
            {
                outputStream_ << "Could not resample audio frame: " << GetErrorString(lastError_) << endl;
                return lastError_;
            }
        }
    }
    else
    {
        codec = videoCodecContext_;
        stream = videoStream_;
        //frame->pts = videoFrameCount_;
        frame->pts = videoCodecContext_->frame_number;
        videoFrameCount_++;
    }

    if (conversionFrame != nullptr)
        lastError_ = avcodec_send_frame(codec, conversionFrame);
    else
        lastError_ = avcodec_send_frame(codec, frame);

    if (lastError_ < 0 && lastError_ != AVERROR(EAGAIN) && lastError_ != AVERROR_EOF)
    {
        outputStream_ << "Could not send frame to encoder: " << GetErrorString(lastError_) << endl;
        return lastError_;
    }

    AVPacket* packet = av_packet_alloc();

    int packetCount = 0;
    lastError_ = avcodec_receive_packet(codec, packet);
    while (lastError_ != AVERROR(EAGAIN))
    {
        if (lastError_ < 0)
        {
            outputStream_ << "Unable to encode packet: " << GetErrorString(lastError_) << endl;
            return lastError_;
        }
        av_packet_rescale_ts(packet, codec->time_base, stream->time_base);
        packet->stream_index = stream->index;
        lastError_ = av_interleaved_write_frame(formatContext_, packet);
        packetCount ++;
        lastError_ = avcodec_receive_packet(codec, packet);
    }

    // If we had to cook the books on audio frames, make sure we clean up after ourselves
    if (conversionFrame != nullptr)
    {
        av_frame_free(&conversionFrame);
    }

    av_packet_free(&packet);

    return packetCount;
}

void OutputVideoFile::Flush()
{
    auto packet = av_packet_alloc();

    // Flush video
    avcodec_send_frame(videoCodecContext_, nullptr);
    lastError_ = avcodec_receive_packet(videoCodecContext_, packet);
    while (lastError_ != AVERROR_EOF)
    {
        av_packet_rescale_ts(packet, videoCodecContext_->time_base, videoStream_->time_base);
        packet->stream_index = videoStream_->index;
        lastError_ = av_interleaved_write_frame(formatContext_, packet);
        lastError_ = avcodec_receive_packet(videoCodecContext_, packet);
    }

    // Flush audio
    if (audioCodecContext_ != nullptr)
    {
        avcodec_send_frame(audioCodecContext_, nullptr);
        lastError_ = avcodec_receive_packet(audioCodecContext_, packet);
        while (lastError_ != AVERROR_EOF)
        {
            av_packet_rescale_ts(packet, audioCodecContext_->time_base, audioStream_->time_base);
            packet->stream_index = audioStream_->index;
            lastError_ = av_interleaved_write_frame(formatContext_, packet);
            lastError_ = avcodec_receive_packet(audioCodecContext_, packet);
        }
    }

    av_packet_free(&packet);
}

string DerperView::GetErrorString(int errorCode)
{
    char errorBuffer[256];
    av_strerror(errorCode, errorBuffer, sizeof(errorBuffer));
    return string(errorBuffer);
}

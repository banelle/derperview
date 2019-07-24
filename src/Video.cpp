#include "Video.hpp"
#include <iostream>
#include <fstream>
#include <vector>

extern "C"
{
    #include "libswscale/swscale.h"
}

using namespace DerperView;
using namespace std;

int SetupContextWorker(AVFormatContext *formatContext, AVCodecContext **codecContext, AVMediaType type)
{
    auto result = av_find_best_stream(formatContext, type, -1, -1, nullptr, 0);
    if (result < 0)
    {
        cerr << "Could not find stream for " << av_get_media_type_string (type) << ": " << GetErrorString(result) << endl;
        return result;
    }

    int streamIndex = result;

    AVStream *stream = formatContext->streams[streamIndex];
    AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder)
    {
        cerr << "Failed to find codec: " << av_get_media_type_string(type) << ": " << GetErrorString(result) << endl;
        return -666;
    }

    *codecContext = avcodec_alloc_context3(decoder);
    if (!*codecContext)
    {
        cerr << "Failed to allocation codec context: " << av_get_media_type_string(type) << ": " << GetErrorString(result) << endl;
        return -666;
    }

    avcodec_parameters_to_context(*codecContext, stream->codecpar);

    result = avcodec_open2(*codecContext, decoder, nullptr);
    if (result < 0)
    {
        cerr << "Failed to open codec: " << av_get_media_type_string(type) << ": " << GetErrorString(result) << endl;
        return -666;
    }
    (*codecContext)->time_base = stream->time_base;

    return streamIndex;
}

int SetupVideoContext(AVFormatContext *formatContext, AVCodecContext **codecContext)
{
    return SetupContextWorker(formatContext, codecContext, AVMediaType::AVMEDIA_TYPE_VIDEO);
}

int SetupAudioContext(AVFormatContext *formatContext, AVCodecContext **codecContext)
{
    return SetupContextWorker(formatContext, codecContext, AVMediaType::AVMEDIA_TYPE_AUDIO);
}

InputVideoFile::InputVideoFile(std::string filename) :
    filename_(filename),
    formatContext_(nullptr), videoCodecContext_(nullptr), audioCodecContext_(nullptr),
    videoStreamIndex_(-1), audioStreamIndex_(-1),
    frame_(nullptr), draining_(false), lastError_(0)
{
    lastError_ = avformat_open_input(&formatContext_, filename_.c_str(), nullptr, nullptr);
    if (lastError_ < 0)
    {
        cerr << "Error on opening file '" << filename_ << "': " << GetErrorString(lastError_) << endl;
        return;
    }

    lastError_ = avformat_find_stream_info(formatContext_, nullptr);
    if (lastError_ < 0) {
        cerr << "Could not read streams: " << GetErrorString(lastError_) << endl;
        return;
    }

    videoStreamIndex_ = SetupVideoContext(formatContext_, &videoCodecContext_);
    audioStreamIndex_ = SetupAudioContext(formatContext_, &audioCodecContext_);

    frame_ = av_frame_alloc();
    av_init_packet(&packet_);
    packet_.data = nullptr;
    packet_.size = 0;
}

InputVideoFile::~InputVideoFile()
{
    //cout << "Input frames: " << videoCodecContext_->frame_number << endl;

    av_frame_free(&frame_);
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
        lastError_ = av_read_frame(formatContext_, &packet_);
        if (lastError_ >= 0)
        {
            if (packet_.stream_index == videoStreamIndex_)
            {
                lastError_ = avcodec_send_packet(videoCodecContext_, &packet_);
                if (lastError_ < 0)
                {
                    cout << "*** Error sending video packet to decoder: " << GetErrorString (lastError_) << endl;
                    return nullptr;
                }
                lastError_ = avcodec_receive_frame(videoCodecContext_, frame_);
                if (lastError_ < 0 && lastError_ != AVERROR(EAGAIN))
                {
                    cout << "*** Error getting video frame from decoder: " << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
            }
            else if (packet_.stream_index == audioStreamIndex_)
            {
                lastError_ = avcodec_send_packet(audioCodecContext_, &packet_);
                if (lastError_ < 0)
                {
                    cout << "*** Error sending audio packet to decoder" << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
                lastError_ = avcodec_receive_frame(audioCodecContext_, frame_);
                if (lastError_ < 0 && lastError_ != AVERROR(EAGAIN))
                {
                    cout << "*** Error getting audio frame from decoder: " << GetErrorString(lastError_) << endl;
                    return nullptr;
                }
            }
            else
                return nullptr;
        }
        else if (lastError_ == AVERROR_EOF)
        {
            // Begin draining
            draining_ = true;
            lastError_ = avcodec_send_packet(videoCodecContext_, nullptr);
            lastError_ = avcodec_send_packet(audioCodecContext_, nullptr);
            return GetNextDrainFrame();
        }
        av_packet_unref(&packet_);
    }

    return frame_;
}

AVFrame *InputVideoFile::GetNextDrainFrame()
{
    lastError_ = avcodec_receive_frame(videoCodecContext_, frame_);
    if (lastError_ != AVERROR_EOF)
        return frame_;
    lastError_ = avcodec_receive_frame(audioCodecContext_, frame_);
    if (lastError_ != AVERROR_EOF)
        return frame_;
    return nullptr;
}

VideoInfo InputVideoFile::GetVideoInfo()
{
    VideoInfo v;
    v.audioTimeBase = audioCodecContext_->time_base;
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

OutputVideoFile::OutputVideoFile(string filename, VideoInfo sourceInfo) :
    filename_(filename),
    formatContext_(nullptr), videoCodecContext_(nullptr), audioCodecContext_(nullptr),
    videoStream_(nullptr), audioStream_(nullptr),
    videoFrameCount_(0), audioFrameCount_(0), lastError_(0)
{
    lastError_ = avformat_alloc_output_context2(&formatContext_, nullptr, nullptr, filename.c_str());
    if (lastError_ < 0 || formatContext_ == nullptr)
    {
        cerr << "Could not create format context: " << GetErrorString(lastError_) << endl;
        return;
    }

    AVCodec *videoCodec = avcodec_find_encoder(formatContext_->oformat->video_codec);
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
        cerr << "Error creating video codec" << endl;
        return;
    }
    av_dict_free(&opt);

    lastError_ = avcodec_parameters_from_context(videoStream_->codecpar, videoCodecContext_);
    if (lastError_ < 0)
        return;

    AVCodec *audioCodec = avcodec_find_encoder(formatContext_->oformat->audio_codec);
    audioCodecContext_ = avcodec_alloc_context3(audioCodec);
    audioStream_ = avformat_new_stream(formatContext_, audioCodec);

    audioCodecContext_->bit_rate = 128000;
    audioCodecContext_->sample_rate = 48000;
    audioCodecContext_->sample_fmt = audioCodec->sample_fmts[0];
    audioCodecContext_->channel_layout = AV_CH_LAYOUT_STEREO;
    audioCodecContext_->channels = av_get_channel_layout_nb_channels(audioCodecContext_->channel_layout);
    audioCodecContext_->time_base = sourceInfo.audioTimeBase;
    audioStream_->time_base = audioCodecContext_->time_base;

    opt = nullptr;
    lastError_ = avcodec_open2(audioCodecContext_, audioCodec, &opt);
    if (lastError_ < 0)
    {
        cerr << "Error creating audio codec" << endl;
        return;
    }
    av_dict_free(&opt);

    lastError_ = avcodec_parameters_from_context(audioStream_->codecpar, audioCodecContext_);
    if (lastError_ < 0)
        return;

    av_dump_format(formatContext_, 0, filename.c_str(), 1);

    lastError_ = avio_open(&formatContext_->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (lastError_ < 0)
    {
        cerr << "Error opening output file" << endl;
        return;
    }
    lastError_ = avformat_write_header(formatContext_, &opt);
    if (lastError_ < 0)
    {
        cerr << "Could not write container header: " << GetErrorString(lastError_) << endl;
    }
}

OutputVideoFile::~OutputVideoFile()
{
    //cout << "Output frames: " << videoCodecContext_->frame_number << "(" << videoFrameCount_ << ")" << endl;

    if (formatContext_->pb != nullptr)
    {
        av_write_trailer(formatContext_);
        avio_closep(&formatContext_->pb);
    }

    avcodec_free_context(&videoCodecContext_);
    avcodec_free_context(&audioCodecContext_);
    avformat_free_context(formatContext_);
}

int OutputVideoFile::WriteNextFrame(AVFrame *frame)
{
    AVCodecContext *codec = nullptr;
    AVStream *stream = nullptr;
    if (frame->width == 0)
    {
        codec = audioCodecContext_;
        stream = audioStream_;
        //frame->pts = av_rescale_q(frame->pts, AVRational { 1, audioCodecContext_->sample_rate }, audioCodecContext_->time_base);
        //frame->pts = audioFrameCount_;
        audioFrameCount_++;
    }
    else
    {
        codec = videoCodecContext_;
        stream = videoStream_;
        //frame->pts = videoFrameCount_;
        frame->pts = videoCodecContext_->frame_number;
        videoFrameCount_++;
    }

    lastError_ = avcodec_send_frame(codec, frame);
    if (lastError_ < 0 && lastError_ != AVERROR(EAGAIN) && lastError_ != AVERROR_EOF)
    {
        cerr << "Could not send frame to encoder: " << GetErrorString(lastError_) << endl;
        return lastError_;
    }

    AVPacket packet = { 0 };
    av_init_packet(&packet);

    int packetCount = 0;
    lastError_ = avcodec_receive_packet(codec, &packet);
    while (lastError_ != AVERROR(EAGAIN))
    {
        if (lastError_ < 0)
        {
            cerr << "Unable to encode packet: " << GetErrorString(lastError_) << endl;
            return lastError_;
        }
        av_packet_rescale_ts(&packet, codec->time_base, stream->time_base);
        packet.stream_index = stream->index;
        lastError_ = av_interleaved_write_frame(formatContext_, &packet);
        packetCount ++;
        lastError_ = avcodec_receive_packet(codec, &packet);
    }

    return packetCount;
}

void OutputVideoFile::Flush()
{
    AVPacket packet = { 0 };
    av_init_packet(&packet);

    // Flush video
    avcodec_send_frame(videoCodecContext_, nullptr);
    lastError_ = avcodec_receive_packet(videoCodecContext_, &packet);
    while (lastError_ != AVERROR_EOF)
    {
        av_packet_rescale_ts(&packet, videoCodecContext_->time_base, videoStream_->time_base);
        packet.stream_index = videoStream_->index;
        lastError_ = av_interleaved_write_frame(formatContext_, &packet);
        lastError_ = avcodec_receive_packet(videoCodecContext_, &packet);
    }

    // Flush audio
    avcodec_send_frame(audioCodecContext_, nullptr);
    lastError_ = avcodec_receive_packet(audioCodecContext_, &packet);
    while (lastError_ != AVERROR_EOF)
    {
        av_packet_rescale_ts(&packet, audioCodecContext_->time_base, audioStream_->time_base);
        packet.stream_index = audioStream_->index;
        lastError_ = av_interleaved_write_frame(formatContext_, &packet);
        lastError_ = avcodec_receive_packet(audioCodecContext_, &packet);
    }
}

string DerperView::GetErrorString(int errorCode)
{
    char errorBuffer[256];
    av_strerror(errorCode, errorBuffer, sizeof(errorBuffer));
    return string(errorBuffer);
}

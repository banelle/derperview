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
    frame_(nullptr), draining_(false)
{
    auto result = avformat_open_input(&formatContext_, filename_.c_str(), nullptr, nullptr);
    if (result < 0)
    {
        cerr << "Error on opening file '" << filename_ << "': " << GetErrorString(result) << endl;
        return;
    }

    result = avformat_find_stream_info(formatContext_, nullptr);
    if (result < 0) {
        cerr << "Could not read streams: " << GetErrorString(result) << endl;
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

    int result = AVERROR(EAGAIN);
    while (result == AVERROR(EAGAIN))
    {
        result = av_read_frame(formatContext_, &packet_);
        if (result >= 0)
        {
            if (packet_.stream_index == videoStreamIndex_)
            {
                result = avcodec_send_packet(videoCodecContext_, &packet_);
                if (result < 0)
                {
                    cout << "*** Error sending video packet to decoder: " << GetErrorString (result) << endl;
                    return nullptr;
                }
                result = avcodec_receive_frame(videoCodecContext_, frame_);
                if (result < 0 && result != AVERROR(EAGAIN))
                {
                    cout << "*** Error getting video frame from decoder: " << GetErrorString(result) << endl;
                    return nullptr;
                }
            }
            else if (packet_.stream_index == audioStreamIndex_)
            {
                result = avcodec_send_packet(audioCodecContext_, &packet_);
                if (result < 0)
                {
                    cout << "*** Error sending audio packet to decoder" << GetErrorString(result) << endl;
                    return nullptr;
                }
                result = avcodec_receive_frame(audioCodecContext_, frame_);
                if (result < 0 && result != AVERROR(EAGAIN))
                {
                    cout << "*** Error getting audio frame from decoder: " << GetErrorString(result) << endl;
                    return nullptr;
                }
            }
            else
                return nullptr;
        }
        else if (result == AVERROR_EOF)
        {
            // Begin draining
            draining_ = true;
            result = avcodec_send_packet(videoCodecContext_, nullptr);
            result = avcodec_send_packet(audioCodecContext_, nullptr);
            return GetNextDrainFrame();
        }
        av_packet_unref(&packet_);
    }

    return frame_;
}

AVFrame *InputVideoFile::GetNextDrainFrame()
{
    int result = avcodec_receive_frame(videoCodecContext_, frame_);
    if (result != AVERROR_EOF)
        return frame_;
    result = avcodec_receive_frame(audioCodecContext_, frame_);
    if (result != AVERROR_EOF)
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
    videoFrameCount_(0), audioFrameCount_(0)
{
    auto result = avformat_alloc_output_context2(&formatContext_, nullptr, nullptr, filename.c_str());
    if (result < 0 || formatContext_ == nullptr)
    {
        cerr << "Could not create format context: " << GetErrorString(result) << endl;
        return;
    }

    AVCodec *videoCodec = avcodec_find_encoder(formatContext_->oformat->video_codec);
    videoCodecContext_ = avcodec_alloc_context3(videoCodec);
    videoStream_ = avformat_new_stream(formatContext_, videoCodec);

    videoCodecContext_->profile = FF_PROFILE_H264_MAIN;
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
    result = avcodec_open2(videoCodecContext_, videoCodec, &opt);
    av_dict_free(&opt);

    result = avcodec_parameters_from_context(videoStream_->codecpar, videoCodecContext_);

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
    result = avcodec_open2(audioCodecContext_, audioCodec, &opt);
    av_dict_free(&opt);

    result = avcodec_parameters_from_context(audioStream_->codecpar, audioCodecContext_);

    av_dump_format(formatContext_, 0, filename.c_str(), 1);

    result = avio_open(&formatContext_->pb, filename.c_str(), AVIO_FLAG_WRITE);
    result = avformat_write_header(formatContext_, &opt);
    if (result < 0)
    {
        cerr << "Could not write container header: " << GetErrorString(result) << endl;
    }
}

OutputVideoFile::~OutputVideoFile()
{
    //cout << "Output frames: " << videoCodecContext_->frame_number << "(" << videoFrameCount_ << ")" << endl;

    av_write_trailer(formatContext_);
    avio_closep(&formatContext_->pb);

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

    auto result = avcodec_send_frame(codec, frame);
    if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF)
    {
        cerr << "Could not send frame to encoder: " << GetErrorString(result) << endl;
        return result;
    }

    AVPacket packet = { 0 };
    av_init_packet(&packet);

    int packetCount = 0;
    result = avcodec_receive_packet(codec, &packet);
    while (result != AVERROR(EAGAIN))
    {
        if (result < 0)
        {
            cerr << "Unable to encode packet: " << GetErrorString(result) << endl;
            return result;
        }
        av_packet_rescale_ts(&packet, codec->time_base, stream->time_base);
        packet.stream_index = stream->index;
        result = av_interleaved_write_frame(formatContext_, &packet);
        packetCount ++;
        result = avcodec_receive_packet(codec, &packet);
    }

    return packetCount;
}

void OutputVideoFile::Flush()
{
    AVPacket packet = { 0 };
    av_init_packet(&packet);

    // Flush video
    avcodec_send_frame(videoCodecContext_, nullptr);
    int result = avcodec_receive_packet(videoCodecContext_, &packet);
    while (result != AVERROR_EOF)
    {
        av_packet_rescale_ts(&packet, videoCodecContext_->time_base, videoStream_->time_base);
        packet.stream_index = videoStream_->index;
        result = av_interleaved_write_frame(formatContext_, &packet);
        result = avcodec_receive_packet(videoCodecContext_, &packet);
    }

    // Flush audio
    avcodec_send_frame(audioCodecContext_, nullptr);
    result = avcodec_receive_packet(audioCodecContext_, &packet);
    while (result != AVERROR_EOF)
    {
        av_packet_rescale_ts(&packet, audioCodecContext_->time_base, audioStream_->time_base);
        packet.stream_index = audioStream_->index;
        result = av_interleaved_write_frame(formatContext_, &packet);
        result = avcodec_receive_packet(audioCodecContext_, &packet);
    }
}


#include "png.h"
#include <string>

struct PngBuffer
{
    PngBuffer(unsigned char *data) : Data(data), Position(0) { }

    unsigned char *Data;
    unsigned int Position;
};

void ReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
    PngBuffer *buffer = reinterpret_cast<PngBuffer *>(png_get_io_ptr(pngPtr));
    memcpy(reinterpret_cast<unsigned char *>(data), buffer->Data + buffer->Position, length);
    buffer->Position += length;
}

void LoadPng(vector<unsigned char> &data, string filename)
{
    ifstream input(filename, ios::binary);
    vector<unsigned char> fileBuffer(istreambuf_iterator<char>(input), {});

    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop infoPtr = png_create_info_struct(pngPtr);
    PngBuffer buffer(fileBuffer.data());
    png_set_read_fn(pngPtr, reinterpret_cast<png_voidp>(&buffer), ReadData);

    png_read_info(pngPtr, infoPtr);

    unsigned int width = infoPtr->width;
    unsigned int height = infoPtr->height;
    short bitDepth = infoPtr->bit_depth;
    short channels = infoPtr->channels;

    data.resize(width * height * bitDepth * channels / 8);
    unsigned char **rowPointers = new unsigned char *[height];

    for (unsigned int i = 0; i < height; i++)
        rowPointers[i] = &(data[i * width * bitDepth * channels / 8]);

    png_read_image(pngPtr, (png_bytepp)rowPointers);

    delete[] rowPointers;
}

void wtf_libpng(png_structp png_ptr, png_const_charp message)
{
    cout << "*** " << message << endl;
}

void PngWriteCallback(png_struct *png, png_byte *data, png_size_t length)
{
    auto out = reinterpret_cast<vector<unsigned char> *>(png_get_io_ptr(png));
    out->insert(out->end(), data, data + length);
}

void SavePng(vector<unsigned char> data, unsigned int width, unsigned int height, vector<unsigned char> &out)
{
    auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, wtf_libpng, wtf_libpng);
    auto info = png_create_info_struct(png);

    png_set_IHDR(png, info, width, height,
        8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_set_write_fn(png, &out, PngWriteCallback, nullptr);

    png_write_info(png, info);

    for (unsigned int i = 0; i < height; i++)
        png_write_row(png, &(data[i * width * 3]));

    png_write_end(png, nullptr);
}

bool DerperView::DumpToPng(AVFrame *frame, std::string filename)
{
    AVFrame *dumpFrame = av_frame_alloc();
    int dumpFrameSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frame->width, frame->height, 1);
    uint8_t* dumpBuffer = (uint8_t *)av_malloc(dumpFrameSize * sizeof(uint8_t));
    av_image_fill_arrays(dumpFrame->data, dumpFrame->linesize, dumpBuffer, AV_PIX_FMT_RGB24, frame->width, frame->height, 1);
    SwsContext *scaleContext = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
        frame->width, frame->height, AV_PIX_FMT_RGB24, SWS_POINT, nullptr, nullptr, nullptr);

    auto result = sws_scale(scaleContext, frame->data, frame->linesize, 0, frame->height, dumpFrame->data, dumpFrame->linesize);
    if (result < 0)
    {
        cerr << "Could not reformat picture: " << GetErrorString(result) << endl;
        return false;
    }
    else if (result != frame->height)
    {
        cerr << "Did not write all rows: " << result << " / " << frame->height << endl;
        return false;
    }
    else
    {
        auto foo = reinterpret_cast<unsigned char *>(dumpFrame->data[0]);
        auto bufferSize = dumpFrame->linesize[0] * frame->height;
        vector<unsigned char> data(foo, foo + bufferSize);

        vector<unsigned char> output;
        SavePng(data, frame->width, frame->height, output);

        ofstream of(filename + ".sv.png", ios::binary);
        of.write(reinterpret_cast<char *>(output.data()), output.size());
        of.close();
    }

    sws_freeContext(scaleContext);
    av_frame_free(&dumpFrame);

    return true;
}

string DerperView::GetErrorString(int errorCode)
{
    char errorBuffer[256];
    av_strerror(errorCode, errorBuffer, sizeof(errorBuffer));
    return string(errorBuffer);
}

# derperview

Performs a non-linear horizontal stretch to convert a 4:3 video to a 16:9 one, without the usual short-and-fat result.

This probably works best with visual aids, so in short it does this:

![ttiuwp](https://raw.githubusercontent.com/banelle/derperview/master/doc/0_original.jpg "yuck 4:3")
![ttiuwp](https://raw.githubusercontent.com/banelle/derperview/master/doc/1_derped.jpg "yummy derped 16:9")

## Download

[Derperview v0.5](https://github.com/banelle/derperview/releases/download/v0.5/derperview.zip)

## Usage

```derperview [--stfu] INPUT_FILE```

Output is always H264, AAC and MP4. Input should be more flexible (in theory it'll take anything that ffmpeg can read), but if you use something with a variable framerate then wacky things will occur.

The --stfu option suppresses the naturally chatty nature of libav. By default libav will dump a bunch of information that you might not care about, and can make derperview's error messages harder to see.

## Dependencies

- libav (the ffmpeg fork, not the libav one)

Included in the binary releases, but you'll need to download the libs if you want to build from source.

## Issues, Suggestions and Comments

If you have any problems getting the application to run, or want to contact me for any other reason then please use the following thread on IntoFPV:

[IntoFPV thread](https://intofpv.com/t-derperview-a-command-line-superview-alternative)

Please don't be offended if I don't implement your idea or fix your bug. This is - first and foremost - a tool that works for me. I have limited time to work on stuff, and I'd probably rather be spending it flying ;-)

## Known Issues

- The output framerate is normally off by a tiny amount. I suspect it's something to do with the way I'm handing timebases and frame timestamps, but I don't care enough to fix it. The editing package will take care of it eventually.

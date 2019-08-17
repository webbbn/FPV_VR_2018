# FPV_VR_2018
Latest open-sourced version of FPV_VR

I created a new repository because this is more a re-write than an update.

See FPV_VR for the old source code, mostly written in JAVA.

See "Changelog_4.0.pdf" in this branch for changes.

# 2019/08/16 - Initial 360 degree video support

Added initial support for display of 360 degree video from the Insta360 Air USB video camera. Currently only mono display is supported, but the video is displayed on a GL surface, so adding stereo/VR display should be straight forward.

The video must be 2560x1280, and can be streamed using the following ffmpeg command:

`ffmpeg -r 30 -copytb 0 -f v4l2 -vcodec h264 -s 2560x1280 -i /dev/video0 -vcodec copy -f h264 udp://<host>:5000`

Known issues:

- The seam between image halves could use more testing/adjustment
- The FOV is hard-coded, which is not necessary. It should be made a parameter.
- The OSD is scaled using the Scene Scale paramter. It should just fill the screen (IMO).
- The latency is currently (estimated) 250-400 ms (too long).
- The ffmpeg command above is only producing ~8Mb/s bitrate, which seems too low for such a large video.
  - The quality of the view window is relatively poor (compared to current standard of 720P with a similar bitrate).

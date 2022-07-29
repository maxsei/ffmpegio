# FFMPEGIO
This is a testing repo to play around with building a frame decoder that can be called from golang.

In order to use ffmpegio as a go package you must have `pkg-config` in your and valid pkg configuration files for [ffmpeg](https://ffmpeg.org/) available in your `PKG_CONFIG_PATH`.

# TODO:
- [ ] statically compile (ffmpeg, libc, etc included finaly binary)
- [ ] seeking packets
- [ ] test for memory leaks

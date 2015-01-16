dec provides a way to simulate the decode load needed for analytics.

    dec <input.mp4> <fps> <yes,no>

dec will decode only the key frames in input.mp4.

fps determines how many frames per second dec decodes.

The final argument is "yes" if you want dec to use VAKit for decoding or "no" if you want dec to use AVKit.


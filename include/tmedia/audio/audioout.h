#ifndef TMEDIA_AO_H
#define TMEDIA_AO_H

/**
 * Note that this currently isn't used anywhere, as tmedia
 * as of now only supports miniaudio as a library to play
 * audio through. It'll probably stay that way too, because
 * miniaudio does everything it needs to for cross-platform
 * audio playback. Actually, I should probably delete this.
*/
class AudioOut {
  public:
    virtual bool playing() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual double volume() const = 0;
    virtual bool muted() const = 0;
    virtual void set_volume(double volume) = 0;
    virtual void set_muted(bool muted) = 0;
};

#endif
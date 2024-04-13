#ifndef TMEDIA_AO_H
#define TMEDIA_AO_H

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
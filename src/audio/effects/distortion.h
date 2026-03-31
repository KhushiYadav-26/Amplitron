#pragma once

#include "audio/effect.h"
#include "audio/dsp/biquad.h"

namespace GuitarAmp {

class Distortion : public Effect {
public:
    Distortion();
    void process(float* buffer, int num_samples) override;
    void reset() override;
    const char* name() const override { return "Distortion"; }
    std::vector<EffectParam>& params() override { return params_; }

private:
    std::vector<EffectParam> params_;
    OnePole tone_lp_;
};

} // namespace GuitarAmp

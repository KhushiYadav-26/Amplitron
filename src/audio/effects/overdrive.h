#pragma once

#include "audio/effect.h"
#include "audio/dsp/biquad.h"

namespace GuitarAmp {

class Overdrive : public Effect {
public:
    Overdrive();
    void process(float* buffer, int num_samples) override;
    void reset() override;
    const char* name() const override { return "Overdrive"; }
    std::vector<EffectParam>& params() override { return params_; }

private:
    std::vector<EffectParam> params_;
    OnePole tone_lp_;
    OnePole dc_block_;
};

} // namespace GuitarAmp

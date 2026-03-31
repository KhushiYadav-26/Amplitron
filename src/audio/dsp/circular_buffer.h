#pragma once

#include <vector>
#include <algorithm>

namespace GuitarAmp {

/**
 * Fixed-size circular buffer for delay lines.
 * Used by Delay, Chorus, and Reverb.
 */
class CircularBuffer {
public:
    CircularBuffer() = default;

    void resize(int size) {
        buffer_.assign(size, 0.0f);
        write_pos_ = 0;
        size_ = size;
    }

    void write(float sample) {
        buffer_[write_pos_] = sample;
        write_pos_ = (write_pos_ + 1) % size_;
    }

    float read(int delay) const {
        int pos = write_pos_ - delay - 1;
        if (pos < 0) pos += size_;
        return buffer_[pos];
    }

    float read_at(int index) const {
        return buffer_[index % size_];
    }

    void write_at(int index, float sample) {
        buffer_[index % size_] = sample;
    }

    float read_linear(float delay) const {
        float read_pos_f = static_cast<float>(write_pos_) - delay - 1.0f;
        if (read_pos_f < 0.0f) read_pos_f += size_;

        int pos0 = static_cast<int>(read_pos_f) % size_;
        int pos1 = (pos0 + 1) % size_;
        float frac = read_pos_f - static_cast<int>(read_pos_f);

        return buffer_[pos0] * (1.0f - frac) + buffer_[pos1] * frac;
    }

    int write_pos() const { return write_pos_; }
    int size() const { return size_; }

    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        write_pos_ = 0;
    }

private:
    std::vector<float> buffer_;
    int write_pos_ = 0;
    int size_ = 0;
};

} // namespace GuitarAmp

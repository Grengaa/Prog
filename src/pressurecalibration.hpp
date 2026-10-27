#pragma once
#include <cmath>
#include <optional>

// Linear calibration belongs to the sensor/module, not to the HX710B alone.
class PressureCalibration {
public:
    bool configure(double raw1, double mmHg1, double raw2, double mmHg2) {
        valid = false;
        if (!std::isfinite(raw1) || !std::isfinite(raw2) || !std::isfinite(mmHg1) ||
            !std::isfinite(mmHg2) || raw1 == raw2 || mmHg1 == mmHg2) return false;
        slope = (mmHg2-mmHg1)/(raw2-raw1);
        originRaw = raw1; originPressure = mmHg1;
        valid = std::isfinite(slope) && slope != 0;
        return valid;
    }
    void invalidate() { valid = false; }
    bool isValid() const { return valid; }
    std::optional<double> convert(double raw) const {
        if (!valid || !std::isfinite(raw)) return std::nullopt;
        const double pressure = originPressure+(raw-originRaw)*slope;
        if (!std::isfinite(pressure)) return std::nullopt;
        return pressure;
    }
private:
    bool valid = false;
    double slope = 0, originRaw = 0, originPressure = 0;
};

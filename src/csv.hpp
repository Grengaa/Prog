#pragma once
#include <cmath>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

// Decimal point is always '.', independently of the Windows regional settings.
inline bool parseCsv(const std::string &line, std::vector<double> &values) {
    values.clear();
    std::size_t begin = 0;
    for (;;) {
        const auto end = line.find(',', begin);
        std::istringstream field(line.substr(begin, end - begin));
        field.imbue(std::locale::classic());
        double value;
        if (!(field >> value) || !std::isfinite(value)) { values.clear(); return false; }
        field >> std::ws;
        if (!field.eof() || values.size() >= 16) { values.clear(); return false; }
        values.push_back(value);
        if (end == std::string::npos) return true;
        begin = end + 1;
    }
}

#pragma once
#include "csv.hpp"
#include <QByteArray>
#include <QString>
#include <functional>

// Encapsulates partial serial lines; independent of the window and the port.
class SerialCsvBuffer {
public:
    using SampleHandler = std::function<void(const std::vector<double> &)>;
    using ErrorHandler = std::function<void(const QString &)>;
    void reset() { buffer.clear(); discardUntilNewline = false; }
    void feed(const QByteArray &bytes, const SampleHandler &onSample, const ErrorHandler &onError) {
        for (char c : bytes) {
            if (c == '\n') {
                if (!discardUntilNewline) {
                    std::vector<double> values;
                    if (parseCsv(buffer.toStdString(), values)) onSample(values);
                    else onError("Linha CSV inválida descartada.");
                }
                reset();
            } else if (!discardUntilNewline) {
                if (buffer.size() >= 4096) {
                    buffer.clear(); discardUntilNewline = true;
                    onError("Linha excedeu 4096 bytes e foi descartada.");
                } else buffer.append(c);
            }
        }
    }
private:
    QByteArray buffer;
    bool discardUntilNewline = false;
};

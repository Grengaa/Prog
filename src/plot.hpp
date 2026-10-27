#pragma once
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>

struct Sample { double time; std::vector<double> values; };
class Plot : public QWidget {
public:
    void setPressureRange(double minimum, double maximum) {
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum) return;
        minimumPressure = minimum; maximumPressure = maximum; update();
    }
    void clear() { samples.clear(); update(); }
    const std::deque<Sample> &data() const { return samples; }
    explicit Plot(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(600, 350); }
    void add(double time, const std::vector<double> &values) {
        samples.push_back({time, values});
        while (samples.size() > 1000) samples.pop_front();
    }
private:
    std::deque<Sample> samples;
    double minimumPressure = 0, maximumPressure = 300.025;
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#15202b"));
        const QRectF area(85, 30, width()-110, height()-95);
        p.setPen(QColor("#ccd6e0"));
        p.drawText(QRectF(5, 0, width()-10, 25), Qt::AlignCenter, "Pressão (mmHg) · últimas 1000 amostras · escala fixa");
        if (samples.empty()) {
            p.drawText(area, Qt::AlignCenter, "Calibre e conecte o Arduino ou inicie a demonstração");
            return;
        }
        const double low = minimumPressure, high = maximumPressure;
        std::size_t columns = 0;
        for (const auto &s : samples) {
            columns = std::max(columns, s.values.size());
        }
        // Scale before subtracting to avoid overflow for opposite large values.
        const double scale = std::max({1.0, std::abs(low), std::abs(high)});
        const double span = high/scale-low/scale;
        const double first = samples.front().time;
        const double duration = std::max(1.0, samples.back().time-first);
        for (int i=0; i<=4; ++i) {
            double fraction = i/4.0;
            double y = area.bottom()-fraction*area.height();
            p.setPen(QColor("#344454")); p.drawLine(QPointF(area.left(),y), QPointF(area.right(),y));
            p.setPen(QColor("#ccd6e0"));
            p.drawText(QRectF(0,y-10,78,20), Qt::AlignRight|Qt::AlignVCenter,
                       QString::number(low*(1-fraction)+high*fraction,'g',5));
        }
        const QColor colors[] = {QColor("#49c6ff"), QColor("#ffb454"), QColor("#8ae99b"), QColor("#ff7b9c"), QColor("#c3a6ff"), QColor("#fff080")};
        p.save(); p.setClipRect(area.adjusted(-2,-2,2,2));
        for (std::size_t column=0; column<columns; ++column) {
            QPainterPath path; bool started=false;
            p.setPen(QPen(colors[column%6],2));
            for (const auto &s : samples) {
                if (column >= s.values.size()) { started=false; continue; }
                QPointF point(area.left()+(s.time-first)/duration*area.width(),
                              area.bottom()-(s.values[column]/scale-low/scale)/span*area.height());
                if (started) path.lineTo(point); else { path.moveTo(point); started=true; }
                p.drawEllipse(point,1.5,1.5);
            }
            p.drawPath(path);
        }
        p.restore();
        p.setPen(QColor("#ccd6e0"));
        p.drawText(QRectF(area.left(),area.bottom()+5,area.width(),20), Qt::AlignCenter,
                   QString("Tempo desde a conexão (s): %1 — %2").arg(first,0,'f',1).arg(samples.back().time,0,'f',1));
        for (std::size_t i=0;i<columns;++i) {
            p.setPen(colors[i%6]);
            p.drawText(85+int(i%8)*130,height()-25+int(i/8)*18,QString("Pressão %1").arg(i+1));
        }
    }
};


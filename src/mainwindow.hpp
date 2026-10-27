#pragma once
#include "plot.hpp"
#include "serialcsvbuffer.hpp"
#include "pressurecalibration.hpp"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QVBoxLayout>

class MainWindow : public QWidget {
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    void reset();
    void refreshPorts();
    void stop();
    void ingest(const std::vector<double> &values);
    void showPressure(double mmHg, bool simulated);
    void applyCalibration();
    QComboBox ports, baud;
    QPushButton refresh{"Atualizar portas"}, connect{"Conectar"}, demo{"Demonstração"}, pause{"Pausar"}, clear{"Limpar"};
    Plot plot;
    QLabel status;
    QLabel currentPressure{"Pressão: aguardando calibração"};
    QLabel rawReading{"RAW: —"};
    QDoubleSpinBox raw1, pressure1, raw2, pressure2, axisMin, axisMax;
    QPushButton calibrate{"Aplicar calibração"}, applyRange{"Aplicar escala"};
    PressureCalibration calibration;
    double displayedMinimum = 0, displayedMaximum = 300.025;
    QSerialPort serial;
    SerialCsvBuffer lines;
    QElapsedTimer clock;
    QTimer simulator, redraw;
    quint64 accepted=0, rejected=0;
};

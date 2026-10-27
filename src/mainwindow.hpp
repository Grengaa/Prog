#pragma once
#include "plot.hpp"
#include "serialcsvbuffer.hpp"
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
    QComboBox ports, baud;
    QPushButton refresh{"Atualizar portas"}, connect{"Conectar"}, demo{"Demonstração"}, pause{"Pausar"}, clear{"Limpar"};
    Plot plot;
    QLabel status;
    QSerialPort serial;
    SerialCsvBuffer lines;
    QElapsedTimer clock;
    QTimer simulator, redraw;
    quint64 accepted=0, rejected=0;
};

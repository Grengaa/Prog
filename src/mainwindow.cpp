#include "mainwindow.hpp"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Arduino — gráfico serial"); resize(1000,650);
    auto *layout = new QVBoxLayout(this);
    auto *controls = new QHBoxLayout;
    baud.setEditable(true); baud.addItems({"9600","19200","38400","57600","115200"});
    pause.setCheckable(true);
    controls->addWidget(&ports,1); controls->addWidget(&refresh); controls->addWidget(&baud);
    controls->addWidget(&connect); controls->addWidget(&demo); controls->addWidget(&pause); controls->addWidget(&clear);
    layout->addLayout(controls);
    layout->addWidget(&plot,1);
    status.setText("Pronto. CSV: 10.5,20.3,30.1 seguido de nova linha. Decimal: ponto."); layout->addWidget(&status);
    clock.start();
    simulator.setInterval(50); redraw.setInterval(33);

    QObject::connect(&refresh,&QPushButton::clicked,this,[this] { refreshPorts(); });
    QObject::connect(&clear,&QPushButton::clicked,this,[this] { reset(); status.setText("Gráfico e contadores limpos."); });
    QObject::connect(&pause,&QPushButton::toggled,this,[this](bool paused) { pause.setText(paused?"Retomar":"Pausar"); });
    QObject::connect(&connect,&QPushButton::clicked,this,[this] {
        if (serial.isOpen()) { stop(); status.setText("Desconectado."); return; }
        stop();
        bool ok=false; const int speed=baud.currentText().toInt(&ok);
        if (ports.currentIndex()<0 || !ok || speed<=0) { status.setText("Selecione uma porta e um baud rate válido."); return; }
        serial.setPortName(ports.currentData().toString());
        if (!serial.setBaudRate(speed) || !serial.setDataBits(QSerialPort::Data8) ||
            !serial.setParity(QSerialPort::NoParity) || !serial.setStopBits(QSerialPort::OneStop) ||
            !serial.setFlowControl(QSerialPort::NoFlowControl) || !serial.open(QIODevice::ReadOnly)) {
            status.setText("Erro: "+serial.errorString()+". Feche o Monitor Serial da IDE Arduino."); return;
        }
        serial.setReadBufferSize(65536); reset(); connect.setText("Desconectar");
        ports.setEnabled(false); baud.setEnabled(false); refresh.setEnabled(false);
        status.setText("Conectado. Aguardando linhas CSV (o Arduino pode reiniciar ao abrir a porta).");
    });
    QObject::connect(&demo,&QPushButton::clicked,this,[this] {
        const bool running=simulator.isActive(); stop();
        if (running) { status.setText("Demonstração encerrada."); return; }
        reset(); simulator.start(); demo.setText("Parar demonstração");
    });
    QObject::connect(&simulator,&QTimer::timeout,this,[this] {
        const double t=clock.elapsed()/1000.0; ingest({std::sin(t),std::cos(t),0.5*std::sin(2*t)});
    });
    QObject::connect(&serial,&QSerialPort::readyRead,this,[this] {
        lines.feed(serial.readAll(),
            [this](const std::vector<double> &values) { ingest(values); },
            [this](const QString &message) { ++rejected; status.setText(message+QString(" Total: %1").arg(rejected)); });
    });
    QObject::connect(&serial,&QSerialPort::errorOccurred,this,[this](QSerialPort::SerialPortError error) {
        if (error==QSerialPort::ResourceError) { const QString message=serial.errorString(); stop(); status.setText("Conexão perdida: "+message); }
    });
    QObject::connect(&redraw,&QTimer::timeout,&plot,QOverload<>::of(&QWidget::update));
    refreshPorts(); redraw.start();
}

void MainWindow::reset() {
    plot.clear(); lines.reset(); accepted=rejected=0; clock.restart();
}

void MainWindow::refreshPorts() {
    const QString selected = ports.currentData().toString(); ports.clear();
    for (const auto &info : QSerialPortInfo::availablePorts())
        ports.addItem(info.portName()+" — "+info.description(),info.portName());
    const int index = ports.findData(selected); if (index>=0) ports.setCurrentIndex(index);
}

void MainWindow::stop() {
    serial.close(); simulator.stop(); connect.setText("Conectar"); demo.setText("Demonstração");
    ports.setEnabled(true); baud.setEnabled(true); refresh.setEnabled(true);
}

void MainWindow::ingest(const std::vector<double> &values) {
    ++accepted;
    if (!pause.isChecked()) plot.add(clock.elapsed()/1000.0,values);
    status.setText(QString("%1 | Recebidas: %2 | Inválidas: %3%4")
        .arg(simulator.isActive()?"Demonstração":serial.portName()).arg(accepted).arg(rejected)
        .arg(pause.isChecked()?" | Visualização pausada (dados descartados)":""));
}

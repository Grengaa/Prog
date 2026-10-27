#include "mainwindow.hpp"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("HX710B — pressão em mmHg"); resize(1100,750);
    auto *layout = new QVBoxLayout(this);
    auto *controls = new QHBoxLayout;
    baud.setEditable(true); baud.addItems({"9600","19200","38400","57600","115200"});
    pause.setCheckable(true);
    controls->addWidget(&ports,1); controls->addWidget(&refresh); controls->addWidget(&baud);
    controls->addWidget(&connect); controls->addWidget(&demo); controls->addWidget(&pause); controls->addWidget(&clear);
    layout->addLayout(controls);
    auto *calibrationControls = new QHBoxLayout;
    auto addField = [&](const QString &label, QDoubleSpinBox &field, const QString &name, bool raw) {
        field.setObjectName(name); field.setRange(-1e9, 1e9); field.setDecimals(raw?0:3);
        calibrationControls->addWidget(new QLabel(label, this)); calibrationControls->addWidget(&field);
    };
    addField("RAW 1", raw1, "raw1", true); addField("mmHg 1", pressure1, "pressure1", false);
    addField("RAW 2", raw2, "raw2", true); addField("mmHg 2", pressure2, "pressure2", false);
    calibrationControls->addWidget(&calibrate); layout->addLayout(calibrationControls);
    layout->addWidget(new QLabel("Informe dois pares RAW/pressão medidos com referência. A faixa 0–40 kPa não define a conversão RAW.", this));
    auto *rangeControls = new QHBoxLayout;
    axisMin.setObjectName("axisMin"); axisMax.setObjectName("axisMax");
    for (auto *field : {&axisMin, &axisMax}) { field->setRange(-100000,100000); field->setDecimals(3); }
    axisMax.setValue(300.025);
    rangeControls->addWidget(new QLabel("Escala mínima (mmHg)", this)); rangeControls->addWidget(&axisMin);
    rangeControls->addWidget(new QLabel("Máxima (mmHg)", this)); rangeControls->addWidget(&axisMax);
    rangeControls->addWidget(&applyRange); rangeControls->addWidget(&rawReading);
    layout->addLayout(rangeControls);
    currentPressure.setStyleSheet("font-size: 24px; font-weight: bold;"); layout->addWidget(&currentPressure);
    layout->addWidget(&plot,1);
    status.setText("Envie uma contagem RAW numérica por linha. Sem calibração, somente RAW será mostrado."); layout->addWidget(&status);
    clock.start();
    simulator.setInterval(50); redraw.setInterval(33);
    QObject::connect(&calibrate, &QPushButton::clicked, this, &MainWindow::applyCalibration);
    QObject::connect(&applyRange, &QPushButton::clicked, this, [this] {
        if(axisMin.value() >= axisMax.value()) { status.setText("A escala mínima precisa ser menor que a máxima."); return; }
        displayedMinimum=axisMin.value(); displayedMaximum=axisMax.value();
        plot.setPressureRange(displayedMinimum,displayedMaximum);
        status.setText("Escala fixa atualizada em mmHg.");
    });
    for(auto *field : {&raw1, &raw2, &pressure1, &pressure2}) {
        QObject::connect(field, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this] {
            calibration.invalidate(); reset(); status.setText("Referências alteradas. Clique em Aplicar calibração.");
        });
    }

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
        status.setText("Conectado. Aguardando RAW do HX710B, um número por linha.");
    });
    QObject::connect(&demo,&QPushButton::clicked,this,[this] {
        const bool running=simulator.isActive(); stop();
        if (running) { status.setText("Demonstração encerrada."); return; }
        reset(); simulator.start(); demo.setText("Parar demonstração");
    });
    QObject::connect(&simulator,&QTimer::timeout,this,[this] {
        const double t=clock.elapsed()/1000.0; ++accepted; showPressure(150+40*std::sin(t),true);
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
    currentPressure.setText("Pressão: aguardando nova amostra"); rawReading.setText("RAW: —");
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
    currentPressure.setText("Pressão: sem aquisição ativa"); rawReading.setText("RAW: —");
}

void MainWindow::ingest(const std::vector<double> &values) {
    if(values.size()!=1 || std::trunc(values.front())!=values.front()) {
        ++rejected; status.setText("Esperado um único inteiro RAW por linha (sem rótulos nem outras colunas)."); return;
    }
    ++accepted;
    rawReading.setText(QString("RAW: %1").arg(values.front(),0,'f',0));
    if(!calibration.isValid()) {
        currentPressure.setText("Pressão: não calibrada"); status.setText("RAW recebido. Informe duas referências e aplique a calibração."); return;
    }
    const auto pressure=calibration.convert(values.front());
    if(!pressure) { ++rejected; status.setText("Conversão inválida."); return; }
    showPressure(*pressure,false);
}

void MainWindow::applyCalibration() {
    if(!calibration.configure(raw1.value(),pressure1.value(),raw2.value(),pressure2.value())) {
        reset(); status.setText("Calibração inválida: use duas contagens RAW diferentes e duas pressões diferentes."); return;
    }
    reset(); status.setText("Calibração aplicada nesta sessão. Aguardando nova amostra.");
}

void MainWindow::showPressure(double mmHg, bool simulated) {
    if (!pause.isChecked()) {
        plot.add(clock.elapsed()/1000.0,{mmHg});
        currentPressure.setText(QString("%1: %2 mmHg").arg(simulated?"SIMULAÇÃO":"Pressão").arg(mmHg,0,'f',2));
    }
    status.setText(QString("%1 | Recebidas: %2 | Inválidas: %3%4")
        .arg(simulated?"SIMULAÇÃO, sem sensor":serial.portName()).arg(accepted).arg(rejected)
        .arg(pause.isChecked()?" | Visualização pausada (dados descartados)":""));
    if(mmHg<displayedMinimum || mmHg>displayedMaximum) status.setText(status.text()+" | Fora da escala exibida");
}

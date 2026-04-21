#include "MainWindow.h"
#include "KonfiguracjaARX.h"

#include <QDebug>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , arx({-0.4, 0.0, 0.0}, {0.6, 0.0, 0.0}, 1, 0.0)
    , pid(0.5, 5.0, 0.2)
    , gen(Generator::TypSygnalu::PROSTOKATNY, 0, 1, 10, 0.5, 1.0)
    , petla(arx, pid, gen, this)
    , oknoCzasowe(10.0)
    , m_aplikujeZdalna(false)
{
    konfigurujGUI();
    konfigurujWykresy();

    connect(&petla, &ProstyUAR::krokWykonany,
            this,   &MainWindow::onKrokWykonany);

    aktualizujGenerator();
    aktualizujOknoCzasowe();
    petla.setInterwal(spinInterwal->value());

    sieci.ustawParametry();
    connect(&sieci, &Sieci::polaczono,          this, &MainWindow::onSieciPolaczono);
    connect(&sieci, &Sieci::rozlaczono,         this, &MainWindow::onSieciRozlaczono);
    connect(&sieci, &Sieci::statusZmieniony,    this, &MainWindow::onSieciStatusZmieniony);
    connect(&sieci, &Sieci::odebranoKonfiguracje, this, &MainWindow::onSieciOdebranoKonfiguracje);

    aktualizujStanKontrolek();
    ustawWskaznikPolaczenia(false);
}


void MainWindow::onKrokWykonany(double czas, double w, double y, double e, double u)
{
    aktualizujDaneWykresow(czas, w, y, e, u);
}


void MainWindow::przelaczSymulacje()
{
    if (petla.getInterwal() > 0 && btnStartStop->text() == "Stop") {
        petla.stop();
        btnStartStop->setText("Start");
    } else {
        petla.start();
        btnStartStop->setText("Stop");
    }
    aktualizujStanKontrolek();
}

void MainWindow::zresetujSymulacje()
{
    petla.reset();
    btnStartStop->setText("Start");

    arx.zresetuj_stan();
    pid.zresetuj();

    seriaZadana->clear();
    seriaWyjscie->clear();
    seriaUchyb->clear();
    seriaSterowanie->clear();
    seriaP->clear();
    seriaI->clear();
    seriaD->clear();

    auto resetujOsX = [&](QValueAxis *ax) {
        ax->setRange(0, oknoCzasowe);
        ax->setTickCount(11);
    };
    resetujOsX(osXGlowny);
    resetujOsX(osXUchyb);
    resetujOsX(osXSterowanie);
    resetujOsX(osXPID);

    aktualizujGenerator();
    aktualizujStanKontrolek();
}

void MainWindow::aktualizujInterwal()
{
    petla.setInterwal(spinInterwal->value());
    wyslijKonfiguracjeJesliPolaczony();
}

void MainWindow::otworzKonfiguracjeARX()
{
    KonfiguracjaARX *okno = new KonfiguracjaARX(&arx, this);
    okno->setAttribute(Qt::WA_DeleteOnClose);
    connect(okno, &QDialog::finished, this, [this](int) {
        wyslijKonfiguracjeJesliPolaczony();
    });
    okno->show();
}

void MainWindow::aktualizujPID()
{
    pid.setKp(edycjaKp->text().toDouble());
    pid.setTi(edycjaTi->text().toDouble());
    pid.setTd(edycjaTd->text().toDouble());

    if (comboMetodaCalk->currentIndex() == 0)
        pid.setMetodaCalkowania(RegulatorPID::MetodaCalkowania::STALA_W_SUMIE);
    else
        pid.setMetodaCalkowania(RegulatorPID::MetodaCalkowania::STALA_PRZED_SUMA);

    wyslijKonfiguracjeJesliPolaczony();
}

void MainWindow::aktualizujGenerator()
{
    Generator::TypSygnalu typ = Generator::TypSygnalu::PROSTOKATNY;
    int idx = comboGenTyp->currentIndex();
    if (idx == 1) typ = Generator::TypSygnalu::SINUSOIDALNY;
    else if (idx == 2) typ = Generator::TypSygnalu::SKOK;

    bool czyOkresowy = (idx != 2);
    spinGenOkres->setEnabled(czyOkresowy);
    spinGenWypelnienie->setEnabled(czyOkresowy);

    gen.ustawParametry(typ,
                       spinGenOffset->value(),
                       spinGenAmp->value(),
                       spinGenOkres->value(),
                       spinGenWypelnienie->value(),
                       spinGenCzasAkt->value());

    wyslijKonfiguracjeJesliPolaczony();
}

void MainWindow::aktualizujOknoCzasowe()
{
    oknoCzasowe = spinOknoCzasowe->value();
}

void MainWindow::zresetujCalkePID()
{
    pid.zresetujCalke();
}

//kontrolki
void MainWindow::konfigurujGUI()
{
    QWidget *centralny = new QWidget(this);
    setCentralWidget(centralny);
    QHBoxLayout *glownyLayout = new QHBoxLayout(centralny);
    glownyLayout->setContentsMargins(5, 5, 5, 5);
    glownyLayout->setSpacing(5);

    QWidget *lewyPanelContainer = new QWidget();
    QVBoxLayout *panelSterowania = new QVBoxLayout(lewyPanelContainer);

    grpSym = new QGroupBox("Symulacja");
    QFormLayout *layoutSym = new QFormLayout();

    spinOknoCzasowe = new QDoubleSpinBox();
    spinOknoCzasowe->setRange(5.0, 50.0);
    spinOknoCzasowe->setValue(10.0);
    spinOknoCzasowe->setSuffix(" s");
    spinOknoCzasowe->setSingleStep(1.0);
    connect(spinOknoCzasowe, &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujOknoCzasowe);

    spinInterwal = new QSpinBox();
    spinInterwal->setRange(10, 1000);
    spinInterwal->setValue(200);
    spinInterwal->setSuffix(" ms");
    connect(spinInterwal, &QSpinBox::editingFinished, this, &MainWindow::aktualizujInterwal);

    btnStartStop = new QPushButton("Start");
    connect(btnStartStop, &QPushButton::clicked, this, &MainWindow::przelaczSymulacje);

    btnReset = new QPushButton("Pełny Reset");
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::zresetujSymulacje);

    btnArx = new QPushButton("Konfiguracja Modelu ARX...");
    connect(btnArx, &QPushButton::clicked, this, &MainWindow::otworzKonfiguracjeARX);

    layoutSym->addRow("Zakres Osi X:", spinOknoCzasowe);
    layoutSym->addRow("Interwał:", spinInterwal);
    layoutSym->addRow(btnStartStop, btnReset);
    layoutSym->addRow(btnArx);
    grpSym->setLayout(layoutSym);
    panelSterowania->addWidget(grpSym);

    grpPid = new QGroupBox("Regulator PID");
    QFormLayout *layoutPid = new QFormLayout();
    edycjaKp = new QLineEdit("0.5");
    edycjaTi = new QLineEdit("5.0");
    edycjaTd = new QLineEdit("0.2");
    connect(edycjaKp, &QLineEdit::editingFinished, this, &MainWindow::aktualizujPID);
    connect(edycjaTi, &QLineEdit::editingFinished, this, &MainWindow::aktualizujPID);
    connect(edycjaTd, &QLineEdit::editingFinished, this, &MainWindow::aktualizujPID);

    comboMetodaCalk = new QComboBox();
    comboMetodaCalk->addItem("Stała pod sumą", 0);
    comboMetodaCalk->addItem("Stała przed sumą", 1);
    connect(comboMetodaCalk, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::aktualizujPID);
    comboMetodaCalk->setCurrentIndex(1);

    btnResetI = new QPushButton("Reset Pamięci Całki");
    connect(btnResetI, &QPushButton::clicked, this, &MainWindow::zresetujCalkePID);

    layoutPid->addRow("Kp:", edycjaKp);
    layoutPid->addRow("Ti:", edycjaTi);
    layoutPid->addRow("Td:", edycjaTd);
    layoutPid->addRow("Metoda I:", comboMetodaCalk);
    layoutPid->addRow(btnResetI);
    grpPid->setLayout(layoutPid);
    panelSterowania->addWidget(grpPid);

    grpGen = new QGroupBox("Wartość Zadana");
    QFormLayout *layoutGen = new QFormLayout();
    comboGenTyp = new QComboBox();
    comboGenTyp->addItem("Prostokąt", 0);
    comboGenTyp->addItem("Sinus", 1);
    comboGenTyp->addItem("Skok jednostkowy", 2);
    connect(comboGenTyp, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::aktualizujGenerator);

    spinGenOffset = new QDoubleSpinBox(); spinGenOffset->setRange(-100, 100); spinGenOffset->setValue(0);
    spinGenAmp    = new QDoubleSpinBox(); spinGenAmp->setRange(-100, 100);    spinGenAmp->setValue(1);
    spinGenOkres  = new QDoubleSpinBox(); spinGenOkres->setRange(0.1, 1000);  spinGenOkres->setValue(10);
    spinGenWypelnienie = new QDoubleSpinBox(); spinGenWypelnienie->setRange(0.0, 1.0); spinGenWypelnienie->setValue(0.5); spinGenWypelnienie->setSingleStep(0.1);
    spinGenCzasAkt = new QDoubleSpinBox(); spinGenCzasAkt->setRange(0.0, 1000.0); spinGenCzasAkt->setValue(1.0);

    connect(spinGenOffset,      &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenAmp,         &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenOkres,       &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenWypelnienie, &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenCzasAkt,     &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);

    layoutGen->addRow("Kształt:", comboGenTyp);
    layoutGen->addRow("Składowa stała:", spinGenOffset);
    layoutGen->addRow("Amplituda:", spinGenAmp);
    layoutGen->addRow("Okres (s):", spinGenOkres);
    layoutGen->addRow("Wypełnienie (0-1):", spinGenWypelnienie);
    layoutGen->addRow("Czas aktywacji (s):", spinGenCzasAkt);
    grpGen->setLayout(layoutGen);
    panelSterowania->addWidget(grpGen);

    btnZapiszJson  = new QPushButton("Zapisz Konfigurację (JSON)");
    btnWczytajJson = new QPushButton("Wczytaj Konfigurację (JSON)");
    connect(btnZapiszJson,  &QPushButton::clicked, this, &MainWindow::zapiszKonfiguracje);
    connect(btnWczytajJson, &QPushButton::clicked, this, &MainWindow::wczytajKonfiguracje);
    panelSterowania->addWidget(btnZapiszJson);
    panelSterowania->addWidget(btnWczytajJson);

    QGroupBox *grpSieci = new QGroupBox("Sieci");
    QVBoxLayout *layoutSieci = new QVBoxLayout();
    QFormLayout *layoutAdres = new QFormLayout();
    edycjaIp = new QLineEdit("127.0.0.1");
    spinPort  = new QSpinBox(); spinPort->setRange(1024, 65535); spinPort->setValue(45763);
    layoutAdres->addRow("Adres IP:", edycjaIp);
    layoutAdres->addRow("Port:", spinPort);
    layoutSieci->addLayout(layoutAdres);

    QHBoxLayout *layoutTrybSieci = new QHBoxLayout();
    btnTrybRegulator = new QPushButton("Tryb regulatora");
    btnTrybObiekt    = new QPushButton("Tryb obiektu");
    btnRozlacz       = new QPushButton("Rozłącz");
    connect(btnTrybRegulator, &QPushButton::clicked, this, &MainWindow::wlaczTrybRegulator);
    connect(btnTrybObiekt,    &QPushButton::clicked, this, &MainWindow::wlaczTrybObiekt);
    connect(btnRozlacz,       &QPushButton::clicked, this, &MainWindow::rozlaczSiec);
    layoutTrybSieci->addWidget(btnTrybRegulator);
    layoutTrybSieci->addWidget(btnTrybObiekt);
    layoutSieci->addLayout(layoutTrybSieci);
    layoutSieci->addWidget(btnRozlacz);

    QHBoxLayout *layoutStatus = new QHBoxLayout();
    lblWskaznikPolaczenia = new QLabel();
    lblWskaznikPolaczenia->setFixedSize(18, 18);
    lblWskaznikPolaczenia->setStyleSheet("background-color: #b0b0b0; border-radius: 9px; border: 1px solid #555;");
    lblStatusSieci = new QLabel("Rozłączony");
    lblStatusSieci->setWordWrap(true);
    QFont statusFont = lblStatusSieci->font(); statusFont.setBold(true); lblStatusSieci->setFont(statusFont);
    layoutStatus->addWidget(lblWskaznikPolaczenia);
    layoutStatus->addWidget(lblStatusSieci, 1);
    layoutSieci->addLayout(layoutStatus);
    grpSieci->setLayout(layoutSieci);
    panelSterowania->addWidget(grpSieci);

    panelSterowania->addStretch();
    glownyLayout->addWidget(lewyPanelContainer);

    QGridLayout *siatkaWykresow = new QGridLayout();
    siatkaWykresow->setSpacing(0);
    siatkaWykresow->setContentsMargins(0, 0, 0, 0);

    auto dodajWykres = [&](QChart *chart, int r, int c, int rSpan, int cSpan) {
        QChartView *view = new QChartView(chart);
        view->setRenderHint(QPainter::Antialiasing);
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        siatkaWykresow->addWidget(view, r, c, rSpan, cSpan);
    };

    wykresGlowny    = new QChart();
    wykresUchyb     = new QChart();
    wykresSterowanie = new QChart();
    wykresPID       = new QChart();

    dodajWykres(wykresGlowny,     0, 0, 3, 1);
    dodajWykres(wykresSterowanie, 0, 1, 1, 1);
    dodajWykres(wykresUchyb,      1, 1, 1, 1);
    dodajWykres(wykresPID,        2, 1, 1, 1);

    siatkaWykresow->setColumnStretch(0, 8);
    siatkaWykresow->setColumnStretch(1, 4);
    siatkaWykresow->setRowStretch(0, 1);
    siatkaWykresow->setRowStretch(1, 1);
    siatkaWykresow->setRowStretch(2, 1);

    glownyLayout->addLayout(siatkaWykresow);
    glownyLayout->setStretch(0, 4);
    glownyLayout->setStretch(1, 12);
}

void MainWindow::konfigurujWykresy()
{
    auto setupChart = [](QChart *chart, QString tytul, QValueAxis *&axX, QValueAxis *&axY) {
        chart->setTitle(tytul);
        chart->legend()->hide();
        chart->setBackgroundRoundness(0);
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        chart->setMargins(QMargins(45, 25, 5, 30));
        axX = new QValueAxis(); axX->setTitleText("Czas [s]"); axX->setRange(0, 10);
        axY = new QValueAxis(); axY->setTickCount(11);
        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
    };

    setupChart(wykresGlowny, "Wartość Zadana vs Regulowana", osXGlowny, osYGlowny);
    wykresGlowny->legend()->show();
    wykresGlowny->legend()->setAlignment(Qt::AlignTop);

    seriaZadana  = new QLineSeries(); seriaZadana->setName("Zadana w(t)");
    seriaWyjscie = new QLineSeries(); seriaWyjscie->setName("Regulowana y(t)");
    wykresGlowny->addSeries(seriaZadana);  seriaZadana->attachAxis(osXGlowny);  seriaZadana->attachAxis(osYGlowny);
    wykresGlowny->addSeries(seriaWyjscie); seriaWyjscie->attachAxis(osXGlowny); seriaWyjscie->attachAxis(osYGlowny);

    setupChart(wykresUchyb, "Uchyb Regulacji e(t)", osXUchyb, osYUchyb);
    seriaUchyb = new QLineSeries();
    wykresUchyb->addSeries(seriaUchyb); seriaUchyb->attachAxis(osXUchyb); seriaUchyb->attachAxis(osYUchyb);

    setupChart(wykresSterowanie, "Sygnał Sterujący u(t)", osXSterowanie, osYSterowanie);
    seriaSterowanie = new QLineSeries();
    wykresSterowanie->addSeries(seriaSterowanie); seriaSterowanie->attachAxis(osXSterowanie); seriaSterowanie->attachAxis(osYSterowanie);

    setupChart(wykresPID, "Składowe Regulatora (P, I, D)", osXPID, osYPID);
    wykresPID->legend()->show(); wykresPID->legend()->setAlignment(Qt::AlignTop);
    seriaP = new QLineSeries(); seriaP->setName("P");
    seriaI = new QLineSeries(); seriaI->setName("I");
    seriaD = new QLineSeries(); seriaD->setName("D");
    wykresPID->addSeries(seriaP); seriaP->attachAxis(osXPID); seriaP->attachAxis(osYPID);
    wykresPID->addSeries(seriaI); seriaI->attachAxis(osXPID); seriaI->attachAxis(osYPID);
    wykresPID->addSeries(seriaD); seriaD->attachAxis(osXPID); seriaD->attachAxis(osYPID);
}

void MainWindow::aktualizujDaneWykresow(double t, double w, double y, double e, double u)
{
    seriaZadana->append(t, w);
    seriaWyjscie->append(t, y);
    seriaUchyb->append(t, e);
    seriaSterowanie->append(t, u);
    seriaP->append(t, pid.pobierzOstatnieP());
    seriaI->append(t, pid.pobierzOstatnieI());
    seriaD->append(t, pid.pobierzOstatnieD());

    auto przesunOs = [&](QValueAxis *ax) {
        ax->setRange(t > oknoCzasowe ? t - oknoCzasowe : 0, t > oknoCzasowe ? t : oknoCzasowe);
        ax->setTickCount(11);
    };
    przesunOs(osXGlowny); przesunOs(osXUchyb); przesunOs(osXSterowanie); przesunOs(osXPID);

    double minCzasWidoczny = (t > oknoCzasowe) ? t - oknoCzasowe : 0.0;
    double progUsuwania    = minCzasWidoczny - 1.0;
    auto usunStarePunkty = [progUsuwania](QLineSeries *s) {
        while (s->count() > 1 && s->at(0).x() < progUsuwania) s->remove(0);
    };
    usunStarePunkty(seriaZadana); usunStarePunkty(seriaWyjscie); usunStarePunkty(seriaUchyb);
    usunStarePunkty(seriaSterowanie); usunStarePunkty(seriaP); usunStarePunkty(seriaI); usunStarePunkty(seriaD);

    auto autoScale = [&](const QList<QLineSeries *> &serie, QValueAxis *ay) {
        double minV = 1e9, maxV = -1e9; bool znaleziono = false;
        for (auto s : serie) {
            for (int i = s->points().size() - 1; i >= 0; --i) {
                const QPointF &p = s->points()[i];
                if (p.x() < minCzasWidoczny) break;
                if (p.y() < minV) minV = p.y();
                if (p.y() > maxV) maxV = p.y();
                znaleziono = true;
            }
        }
        if (!znaleziono) { minV = -1.0; maxV = 1.0; }
        else if (std::abs(maxV - minV) < 1e-9) { maxV = minV + 0.1; minV = minV - 0.1; }
        double m = (maxV - minV) * 0.125;
        ay->setRange(minV - m, maxV + m); ay->setTickCount(11);
    };

    autoScale({seriaZadana, seriaWyjscie}, osYGlowny);
    autoScale({seriaUchyb}, osYUchyb);
    autoScale({seriaSterowanie}, osYSterowanie);
    autoScale({seriaP, seriaI, seriaD}, osYPID);
}

// ---- JSON ----

QJsonObject MainWindow::zbudujKonfiguracjeJson() const
{
    QJsonObject root;

    QJsonObject jArx;
    QJsonArray arrA, arrB;
    for (double v : arx.getA())
        arrA.append(v);
    for (double v : arx.getB())
        arrB.append(v);
    jArx["A"] = arrA;
    jArx["B"] = arrB;
    jArx["k"] = arx.getOpoznienie();
    jArx["szum"] = arx.getSzum();
    root["ARX"] = jArx;

    QJsonObject jPid;
    jPid["Kp"] = pid.getKp();
    jPid["Ti"] = pid.getTi();
    jPid["Td"] = pid.getTd();
    jPid["Metoda"] = (int) pid.getMetodaCalkowania();
    root["PID"] = jPid;

    QJsonObject jGen;
    jGen["Typ"] = (int) gen.getTyp();
    jGen["Pocz"] = gen.getPoczatkowa();
    jGen["Zm"] = gen.getZmiana();
    jGen["Czas"] = gen.getCzasZmiany();
    jGen["Wyp"] = gen.getWypelnienie();
    jGen["Akt"] = gen.getCzasAktywacji();
    root["Gen"] = jGen;

    root["Interwal"] = petla.getInterwal();
    root["OknoCzasowe"] = oknoCzasowe;

    return root;
}

void MainWindow::zastosujKonfiguracjeJson(const QJsonObject &root)
{
    // Flaga blokuje zwrotne wysłanie do partnera, unikamy pętli echo.
    m_aplikujeZdalna = true;

    if (root.contains("ARX")) {
        QJsonObject jArx = root["ARX"].toObject();
        std::vector<double> va, vb;
        for (auto v : jArx["A"].toArray())
            va.push_back(v.toDouble());
        for (auto v : jArx["B"].toArray())
            vb.push_back(v.toDouble());
        arx.setA(va);
        arx.setB(vb);
        arx.setOpoznienie(jArx["k"].toInt());
        arx.setSzum(jArx["szum"].toDouble());
    }

    if (root.contains("PID")) {
        QJsonObject jPid = root["PID"].toObject();
        edycjaKp->setText(QString::number(jPid["Kp"].toDouble()));
        edycjaTi->setText(QString::number(jPid["Ti"].toDouble()));
        edycjaTd->setText(QString::number(jPid["Td"].toDouble()));

        int zapisanaMetoda = jPid["Metoda"].toInt();
        int indeksDoUstawienia = 0;
        if ((int) RegulatorPID::MetodaCalkowania::STALA_PRZED_SUMA == zapisanaMetoda) {
            indeksDoUstawienia = 1;
        }
        comboMetodaCalk->setCurrentIndex(indeksDoUstawienia);
        aktualizujPID();
    }

    if (root.contains("Gen")) {
        QJsonObject jGen = root["Gen"].toObject();
        comboGenTyp->setCurrentIndex(jGen["Typ"].toInt());
        spinGenOffset->setValue(jGen["Pocz"].toDouble());
        spinGenAmp->setValue(jGen["Zm"].toDouble());
        spinGenOkres->setValue(jGen["Czas"].toDouble());
        spinGenWypelnienie->setValue(jGen["Wyp"].toDouble());
        spinGenCzasAkt->setValue(jGen["Akt"].toDouble());
        aktualizujGenerator();
    }

    if (root.contains("Interwal")) {
        spinInterwal->setValue(root["Interwal"].toInt());
        aktualizujInterwal();
    }

    if (root.contains("OknoCzasowe")) {
        oknoCzasowe = root["OknoCzasowe"].toDouble();
        spinOknoCzasowe->setValue(oknoCzasowe);
        aktualizujOknoCzasowe();
    }

    m_aplikujeZdalna = false;
}

void MainWindow::wyslijKonfiguracjeJesliPolaczony()
{
    if (m_aplikujeZdalna)
        return;
    if (!sieci.czyPolaczony())
        return;
    sieci.wyslijKonfiguracje(zbudujKonfiguracjeJson());
}

void MainWindow::zapiszKonfiguracje()
{
    QString sciezka = QFileDialog::getSaveFileName(this, "Zapisz Konfigurację", "", "JSON (*.json)");
    if (sciezka.isEmpty())
        return;

    QFile plik(sciezka);
    if (plik.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(zbudujKonfiguracjeJson());
        plik.write(doc.toJson());
        plik.close();
    } else {
        QMessageBox::critical(this, "Błąd", "Nie można zapisać pliku!");
    }
}

void MainWindow::wczytajKonfiguracje()
{
    QString sciezka = QFileDialog::getOpenFileName(this,
                                                   "Wczytaj Konfigurację",
                                                   "",
                                                   "JSON (*.json)");
    if (sciezka.isEmpty())
        return;

    QFile plik(sciezka);
    if (!plik.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Błąd", "Nie można otworzyć pliku!");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(plik.readAll());
    plik.close();

    zastosujKonfiguracjeJson(doc.object());

    wyslijKonfiguracjeJesliPolaczony();

    QMessageBox::information(this, "Sukces", "Konfiguracja wczytana.");
}

void MainWindow::wlaczTrybRegulator()
{
    sieci.ustawParametry(static_cast<quint16>(spinPort->value()), edycjaIp->text());
    sieci.set_tryb(Sieci::Tryb::Regulator);
    aktualizujStanKontrolek();
}

void MainWindow::wlaczTrybObiekt()
{
    sieci.ustawParametry(static_cast<quint16>(spinPort->value()), edycjaIp->text());
    sieci.set_tryb(Sieci::Tryb::Obiekt);
    aktualizujStanKontrolek();
}

void MainWindow::rozlaczSiec()
{
    sieci.rozlacz();
    aktualizujStanKontrolek();
}

void MainWindow::onSieciPolaczono()
{
    ustawWskaznikPolaczenia(true);
    aktualizujStanKontrolek();

    if (sieci.get_tryb() == Sieci::Tryb::Regulator) {
        sieci.wyslijKonfiguracje(zbudujKonfiguracjeJson());
    }
}

void MainWindow::onSieciRozlaczono()
{
    ustawWskaznikPolaczenia(false);
    aktualizujStanKontrolek();
}

void MainWindow::onSieciStatusZmieniony(const QString &opis)
{
    lblStatusSieci->setText(opis);
}

void MainWindow::onSieciOdebranoKonfiguracje(const QJsonObject &konfig)
{
    zastosujKonfiguracjeJson(konfig);
}


void MainWindow::aktualizujStanKontrolek()
{
    Sieci::Tryb tryb = sieci.get_tryb();
    bool polaczony = sieci.czyPolaczony();

    bool trybWybrany = (tryb != Sieci::Tryb::Nieokreslony);

    btnTrybRegulator->setEnabled(!trybWybrany);
    btnTrybObiekt->setEnabled(!trybWybrany);
    btnRozlacz->setEnabled(trybWybrany);

    edycjaIp->setEnabled(!trybWybrany);
    spinPort->setEnabled(!trybWybrany);

    const bool regulator = (tryb == Sieci::Tryb::Regulator);
    const bool obiekt = (tryb == Sieci::Tryb::Obiekt);

    bool mozePID = !trybWybrany || regulator;
    grpPid->setEnabled(mozePID);
    grpGen->setEnabled(mozePID);

    bool mozeARX = !trybWybrany || obiekt;
    btnArx->setEnabled(mozeARX);

    btnStartStop->setEnabled(mozePID && (!trybWybrany || polaczony));
    btnReset->setEnabled(mozePID);
    spinInterwal->setEnabled(mozePID);

    btnWczytajJson->setEnabled(mozePID);
}

void MainWindow::ustawWskaznikPolaczenia(bool polaczony)
{
    if (polaczony) {
        lblWskaznikPolaczenia->setStyleSheet(
            "background-color: #2ecc40; border-radius: 9px; border: 1px solid #186a1f;");
        lblWskaznikPolaczenia->setToolTip("Połączono");
    } else {
        lblWskaznikPolaczenia->setStyleSheet(
            "background-color: #b0b0b0; border-radius: 9px; border: 1px solid #555;");
        lblWskaznikPolaczenia->setToolTip("Rozłączono");
    }
}

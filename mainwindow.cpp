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
    ,
    // Inicjalizacja ARX: A, B, opóźnienie=1, szum=0
    arx({-0.4, 0.0, 0.0}, {0.6, 0.0, 0.0}, 1, 0.0)
    ,
    // PID: Kp=0.5, Ti=5.0, Td=0.2
    pid(0.5, 5.0, 0.2)
    ,
    // Generator: Prostokąt, Offset=0, Amp=1, T=10, Wyp=0.5, Akt=1.0
    gen(Generator::TypSygnalu::PROSTOKATNY, 0, 1, 10, 0.5, 1.0)
    , petla(arx, pid)
    , aktualnyCzas(0.0)
    , czasBazy(0.0)
    , // Inicjalizacja czasu bazy
    czyDziala(false)
    , interwalMs(200)
    , oknoCzasowe(10.0)
{
    konfigurujGUI();
    konfigurujWykresy();

    // Timer symulacji
    timerSymulacji = new QTimer(this);

    timerSymulacji->setTimerType(Qt::PreciseTimer);
    connect(timerSymulacji, &QTimer::timeout, this, &MainWindow::krokSymulacji);

    aktualizujGenerator();

    // Inicjalizacja zmiennych na podstawie wartości domyślnych kontrolek
    aktualizujOknoCzasowe();
    aktualizujInterwal();
}

void MainWindow::konfigurujGUI()
{
    QWidget *centralny = new QWidget(this);
    setCentralWidget(centralny);

    QHBoxLayout *glownyLayout = new QHBoxLayout(centralny);
    glownyLayout->setContentsMargins(5, 5, 5, 5);
    glownyLayout->setSpacing(5);

    //LEWY PANEL (KONTROLKI)
    QWidget *lewyPanelContainer = new QWidget();
    QVBoxLayout *panelSterowania = new QVBoxLayout(lewyPanelContainer);

    // Grupa: Sterowanie Symulacją
    QGroupBox *grpSym = new QGroupBox("Symulacja");
    QFormLayout *layoutSym = new QFormLayout();

    spinOknoCzasowe = new QDoubleSpinBox();
    spinOknoCzasowe->setRange(5.0, 50.0);
    spinOknoCzasowe->setValue(10.0);
    spinOknoCzasowe->setSuffix(" s");
    spinOknoCzasowe->setSingleStep(1.0);
    connect(spinOknoCzasowe,
            &QDoubleSpinBox::editingFinished,
            this,
            &MainWindow::aktualizujOknoCzasowe);

    spinInterwal = new QSpinBox();
    spinInterwal->setRange(10, 1000);
    spinInterwal->setValue(200);
    spinInterwal->setSuffix(" ms");
    connect(spinInterwal, &QSpinBox::editingFinished, this, &MainWindow::aktualizujInterwal);

    btnStartStop = new QPushButton("Start");
    connect(btnStartStop, &QPushButton::clicked, this, &MainWindow::przelaczSymulacje);

    QPushButton *btnReset = new QPushButton("Pełny Reset");
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::zresetujSymulacje);

    QPushButton *btnArx = new QPushButton("Konfiguracja Modelu ARX...");
    connect(btnArx, &QPushButton::clicked, this, &MainWindow::otworzKonfiguracjeARX);

    layoutSym->addRow("Zakres Osi X:", spinOknoCzasowe);
    layoutSym->addRow("Interwał:", spinInterwal);
    layoutSym->addRow(btnStartStop, btnReset);
    layoutSym->addRow(btnArx);
    grpSym->setLayout(layoutSym);
    panelSterowania->addWidget(grpSym);

    // Grupa: Regulator PID
    QGroupBox *grpPid = new QGroupBox("Regulator PID");
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
    connect(comboMetodaCalk,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::aktualizujPID);
    comboMetodaCalk->setCurrentIndex(1);

    QPushButton *btnResetI = new QPushButton("Reset Pamięci Całki");
    connect(btnResetI, &QPushButton::clicked, this, &MainWindow::zresetujCalkePID);

    layoutPid->addRow("Kp:", edycjaKp);
    layoutPid->addRow("Ti:", edycjaTi);
    layoutPid->addRow("Td:", edycjaTd);
    layoutPid->addRow("Metoda I:", comboMetodaCalk);
    layoutPid->addRow(btnResetI);
    grpPid->setLayout(layoutPid);
    panelSterowania->addWidget(grpPid);

    // Grupa: Generator
    QGroupBox *grpGen = new QGroupBox("Wartość Zadana");
    QFormLayout *layoutGen = new QFormLayout();

    comboGenTyp = new QComboBox();
    comboGenTyp->addItem("Prostokąt", 0);
    comboGenTyp->addItem("Sinus", 1);
    comboGenTyp->addItem("Skok jednostkowy", 2);

    connect(comboGenTyp,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::aktualizujGenerator);

    spinGenOffset = new QDoubleSpinBox();
    spinGenOffset->setRange(-100, 100);
    spinGenOffset->setValue(0);
    spinGenAmp = new QDoubleSpinBox();
    spinGenAmp->setRange(-100, 100);
    spinGenAmp->setValue(1);
    spinGenOkres = new QDoubleSpinBox();
    spinGenOkres->setRange(0.1, 1000);
    spinGenOkres->setValue(10);
    spinGenWypelnienie = new QDoubleSpinBox();
    spinGenWypelnienie->setRange(0.0, 1.0);
    spinGenWypelnienie->setValue(0.5);
    spinGenWypelnienie->setSingleStep(0.1);
    spinGenCzasAkt = new QDoubleSpinBox();
    spinGenCzasAkt->setRange(0.0, 1000.0);
    spinGenCzasAkt->setValue(1.0);

    connect(spinGenOffset, &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenAmp, &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenOkres, &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujGenerator);
    connect(spinGenWypelnienie,
            &QDoubleSpinBox::editingFinished,
            this,
            &MainWindow::aktualizujGenerator);
    connect(spinGenCzasAkt,
            &QDoubleSpinBox::editingFinished,
            this,
            &MainWindow::aktualizujGenerator);

    layoutGen->addRow("Kształt:", comboGenTyp);
    layoutGen->addRow("Składowa stała:", spinGenOffset);
    layoutGen->addRow("Amplituda:", spinGenAmp);
    layoutGen->addRow("Okres (s):", spinGenOkres);
    layoutGen->addRow("Wypełnienie (0-1):", spinGenWypelnienie);
    layoutGen->addRow("Czas aktywacji (s):", spinGenCzasAkt);

    grpGen->setLayout(layoutGen);
    panelSterowania->addWidget(grpGen);

    // JSON
    QPushButton *btnZapisz = new QPushButton("Zapisz Konfigurację (JSON)");
    QPushButton *btnWczytaj = new QPushButton("Wczytaj Konfigurację (JSON)");
    connect(btnZapisz, &QPushButton::clicked, this, &MainWindow::zapiszKonfiguracje);
    connect(btnWczytaj, &QPushButton::clicked, this, &MainWindow::wczytajKonfiguracje);

    panelSterowania->addWidget(btnZapisz);
    panelSterowania->addWidget(btnWczytaj);
    panelSterowania->addStretch();

    glownyLayout->addWidget(lewyPanelContainer);

    // PRAWY PANEL (WYKRESY)
    QGridLayout *siatkaWykresow = new QGridLayout();
    siatkaWykresow->setSpacing(0);
    siatkaWykresow->setContentsMargins(0, 0, 0, 0);

    auto dodajWykres = [&](QChart *chart, int r, int c, int rSpan, int cSpan) {
        QChartView *view = new QChartView(chart);
        view->setRenderHint(QPainter::Antialiasing);
        view->setContentsMargins(0, 0, 0, 0);
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        siatkaWykresow->addWidget(view, r, c, rSpan, cSpan);
    };

    wykresGlowny = new QChart();
    wykresUchyb = new QChart();
    wykresSterowanie = new QChart();
    wykresPID = new QChart();

    dodajWykres(wykresGlowny, 0, 0, 3, 1);
    dodajWykres(wykresSterowanie, 0, 1, 1, 1);
    dodajWykres(wykresUchyb, 1, 1, 1, 1);
    dodajWykres(wykresPID, 2, 1, 1, 1);

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

        axX = new QValueAxis();
        axX->setTitleText("Czas [s]");
        axY = new QValueAxis();

        axX->setRange(0, 10);
        axY->setTickCount(11);

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
    };

    // Wykres Główny
    setupChart(wykresGlowny, "Wartość Zadana vs Regulowana", osXGlowny, osYGlowny);
    wykresGlowny->legend()->show();
    wykresGlowny->legend()->setAlignment(Qt::AlignTop);

    seriaZadana = new QLineSeries();
    seriaZadana->setName("Zadana w(t)");
    seriaWyjscie = new QLineSeries();
    seriaWyjscie->setName("Regulowana y(t)");
    wykresGlowny->addSeries(seriaZadana);
    wykresGlowny->addSeries(seriaWyjscie);
    seriaZadana->attachAxis(osXGlowny);
    seriaZadana->attachAxis(osYGlowny);
    seriaWyjscie->attachAxis(osXGlowny);
    seriaWyjscie->attachAxis(osYGlowny);

    // Wykres Uchybu
    setupChart(wykresUchyb, "Uchyb Regulacji e(t)", osXUchyb, osYUchyb);
    seriaUchyb = new QLineSeries();
    seriaUchyb->setName("Uchyb");
    wykresUchyb->addSeries(seriaUchyb);
    seriaUchyb->attachAxis(osXUchyb);
    seriaUchyb->attachAxis(osYUchyb);

    // Wykres Sterowania
    setupChart(wykresSterowanie, "Sygnał Sterujący u(t)", osXSterowanie, osYSterowanie);
    seriaSterowanie = new QLineSeries();
    seriaSterowanie->setName("Sterowanie");
    wykresSterowanie->addSeries(seriaSterowanie);
    seriaSterowanie->attachAxis(osXSterowanie);
    seriaSterowanie->attachAxis(osYSterowanie);

    // Wykres PID
    setupChart(wykresPID, "Składowe Regulatora (P, I, D)", osXPID, osYPID);
    wykresPID->legend()->show();
    wykresPID->legend()->setAlignment(Qt::AlignTop);

    seriaP = new QLineSeries();
    seriaP->setName("P");
    seriaI = new QLineSeries();
    seriaI->setName("I");
    seriaD = new QLineSeries();
    seriaD->setName("D");

    wykresPID->addSeries(seriaP);
    wykresPID->addSeries(seriaI);
    wykresPID->addSeries(seriaD);
    seriaP->attachAxis(osXPID);
    seriaP->attachAxis(osYPID);
    seriaI->attachAxis(osXPID);
    seriaI->attachAxis(osYPID);
    seriaD->attachAxis(osXPID);
    seriaD->attachAxis(osYPID);
}

void MainWindow::krokSymulacji()
{
    double dt_gui = interwalMs / 1000.0;

    double dt_math = 1.0;

    double czasRzeczywistyTarget = czasBazy + (licznikCzasuRzeczywistego.elapsed() / 1000.0);

    double ost_w = 0.0;
    double ost_y = 0.0;
    double ost_e = 0.0;

    int maxKrokow = 20;
    int wykonaneKroki = 0;

    while (aktualnyCzas < czasRzeczywistyTarget && wykonaneKroki < maxKrokow) {
        aktualnyCzas += dt_gui;

        ost_w = gen.generuj(aktualnyCzas);

        ost_y = petla.wykonaj_krok(ost_w, dt_math);

        ost_e = ost_w - ost_y;

        wykonaneKroki++;
    }

    if (aktualnyCzas < czasRzeczywistyTarget - dt_gui) {
        aktualnyCzas = czasRzeczywistyTarget;
    }

    if (wykonaneKroki > 0) {
        double u = pid.pobierzOstatnieP() + pid.pobierzOstatnieI() + pid.pobierzOstatnieD();
        aktualizujDaneWykresow(aktualnyCzas, ost_w, ost_y, ost_e, u);
    }
}

void MainWindow::aktualizujDaneWykresow(double t, double w, double y, double e, double u)
{
    int limitPunktow = 1000;

    // Dodawanie punktów
    seriaZadana->append(t, w);
    seriaWyjscie->append(t, y);
    seriaUchyb->append(t, e);
    seriaSterowanie->append(t, u);
    seriaP->append(t, pid.pobierzOstatnieP());
    seriaI->append(t, pid.pobierzOstatnieI());
    seriaD->append(t, pid.pobierzOstatnieD());

    // Przesuwanie osi X
    auto przesunOs = [&](QValueAxis *ax) {
        if (t > this->oknoCzasowe) {
            ax->setRange(t - this->oknoCzasowe, t);
        } else {
            ax->setRange(0, this->oknoCzasowe);
        }
        ax->setTickCount(11);
    };

    przesunOs(osXGlowny);
    przesunOs(osXUchyb);
    przesunOs(osXSterowanie);
    przesunOs(osXPID);

    // Czyszczenie starych punktów
    if (seriaZadana->count() > limitPunktow) {
        seriaZadana->remove(0);
        seriaWyjscie->remove(0);
        seriaUchyb->remove(0);
        seriaSterowanie->remove(0);
        seriaP->remove(0);
        seriaI->remove(0);
        seriaD->remove(0);
    }

    // Auto-skalowanie Y (tylko dla danych widocznych w oknie)
    double minCzasWidoczny = (t > this->oknoCzasowe) ? t - this->oknoCzasowe : 0.0;

    auto autoScale = [&](const QList<QLineSeries *> &serie, QValueAxis *ay) {
        double minV = 1e9, maxV = -1e9;
        bool znaleziono = false;

        for (auto s : serie) {
            const auto &punkty = s->points();
            for (int i = punkty.size() - 1; i >= 0; --i) {
                const QPointF &p = punkty[i];
                if (p.x() < minCzasWidoczny)
                    break;

                if (p.y() < minV)
                    minV = p.y();
                if (p.y() > maxV)
                    maxV = p.y();
                znaleziono = true;
            }
        }

        if (!znaleziono) {
            minV = -1.0;
            maxV = 1.0;
        } else if (std::abs(maxV - minV) < 1e-9) {
            maxV = minV + 0.1;
            minV = minV - 0.1;
        }

        double zakresDanych = maxV - minV;
        double margines = zakresDanych * 0.125;
        ay->setRange(minV - margines, maxV + margines);
        ay->setTickCount(11);
    };

    autoScale({seriaZadana, seriaWyjscie}, osYGlowny);
    autoScale({seriaUchyb}, osYUchyb);
    autoScale({seriaSterowanie}, osYSterowanie);
    autoScale({seriaP, seriaI, seriaD}, osYPID);
}

void MainWindow::przelaczSymulacje()
{
    if (czyDziala) {
        timerSymulacji->stop();
        btnStartStop->setText("Start");

        czasBazy += licznikCzasuRzeczywistego.elapsed() / 1000.0;
    } else {
        licznikCzasuRzeczywistego.restart();
        timerSymulacji->start(interwalMs);
        btnStartStop->setText("Stop");
    }
    czyDziala = !czyDziala;
}

void MainWindow::zresetujSymulacje()
{
    timerSymulacji->stop();
    czyDziala = false;
    btnStartStop->setText("Start");

    aktualnyCzas = 0.0;
    czasBazy = 0.0;

    arx.zresetuj_stan();
    pid.zresetuj();

    seriaZadana = nullptr;

    seriaZadana->clear();
    seriaWyjscie->clear();
    seriaUchyb->clear();
    seriaSterowanie->clear();
    seriaP->clear();
    seriaI->clear();
    seriaD->clear();

    auto resetujOsX = [&](QValueAxis *ax) {
        ax->setRange(0, this->oknoCzasowe);
        ax->setTickCount(11);
    };

    resetujOsX(osXGlowny);
    resetujOsX(osXUchyb);
    resetujOsX(osXSterowanie);
    resetujOsX(osXPID);

    aktualizujGenerator();
}

void MainWindow::otworzKonfiguracjeARX()
{
    KonfiguracjaARX *okno = new KonfiguracjaARX(&arx, this);
    okno->setAttribute(Qt::WA_DeleteOnClose);
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
}

void MainWindow::aktualizujGenerator()
{
    Generator::TypSygnalu typ = Generator::TypSygnalu::PROSTOKATNY;
    int idx = comboGenTyp->currentIndex();

    if (idx == 1)
        typ = Generator::TypSygnalu::SINUSOIDALNY;
    else if (idx == 2)
        typ = Generator::TypSygnalu::SKOK;

    bool czyOkresowy = (idx != 2);
    spinGenOkres->setEnabled(czyOkresowy);
    spinGenWypelnienie->setEnabled(czyOkresowy);

    gen.ustawParametry(typ,
                       spinGenOffset->value(),
                       spinGenAmp->value(),
                       spinGenOkres->value(),
                       spinGenWypelnienie->value(),
                       spinGenCzasAkt->value());
}

void MainWindow::aktualizujOknoCzasowe()
{
    oknoCzasowe = spinOknoCzasowe->value();
}

void MainWindow::aktualizujInterwal()
{
    interwalMs = spinInterwal->value();

    if (czyDziala) {
        timerSymulacji->setInterval(interwalMs);

        aktualnyCzas = 0.0;
    }
}

void MainWindow::zresetujCalkePID()
{
    pid.zresetuj();
}

void MainWindow::zapiszKonfiguracje()
{
    QJsonObject root;

    // ARX
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

    // PID
    QJsonObject jPid;
    jPid["Kp"] = pid.getKp();
    jPid["Ti"] = pid.getTi();
    jPid["Td"] = pid.getTd();
    jPid["Metoda"] = (int) pid.getMetodaCalkowania();
    root["PID"] = jPid;

    // Generator
    QJsonObject jGen;
    jGen["Typ"] = (int) gen.getTyp();
    jGen["Pocz"] = gen.getPoczatkowa();
    jGen["Zm"] = gen.getZmiana();
    jGen["Czas"] = gen.getCzasZmiany();
    jGen["Wyp"] = gen.getWypelnienie();
    jGen["Akt"] = gen.getCzasAktywacji();
    root["Gen"] = jGen;

    root["Interwal"] = interwalMs;
    root["OknoCzasowe"] = oknoCzasowe;

    QString sciezka = QFileDialog::getSaveFileName(this, "Zapisz Konfigurację", "", "JSON (*.json)");
    if (sciezka.isEmpty())
        return;

    QFile plik(sciezka);
    if (plik.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(root);
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
    QJsonObject root = doc.object();
    plik.close();

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
        interwalMs = root["Interwal"].toInt();
        spinInterwal->setValue(interwalMs);
        aktualizujInterwal();
    }

    if (root.contains("OknoCzasowe")) {
        oknoCzasowe = root["OknoCzasowe"].toDouble();
        spinOknoCzasowe->setValue(oknoCzasowe);
        aktualizujOknoCzasowe();
    }

    QMessageBox::information(this, "Sukces", "Konfiguracja wczytana.");
}

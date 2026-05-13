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
#include <QDateTime>
#include <QSignalBlocker>
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
    , m_timerSieciowy(new QTimer(this))
    , m_czasSieciowy(0.0)
    , m_ostatnieYSieciowe(0.0)
    , m_ostatnieUSieciowe(0.0)
    , m_ostatnieWSieciowe(0.0)
    , m_ostatnieESieciowe(0.0)
    , m_ostatniePSieciowe(0.0)
    , m_ostatnieISieciowe(0.0)
    , m_ostatnieDSieciowe(0.0)
    , m_ostatnieDtSieciowe(0.0)
    , m_shadowYSieciowe(0.0)
    , m_shadowUSieciowe(0.0)
    , m_numerProbkiSieciowej(0)
    , m_ostatniSeqSterowania(0)
    , m_ostatniSeqOdpowiedzi(0)
    , m_brakiSterowaniaPodRzad(0)
    , m_pakietyWyslaneOkno(0)
    , m_pakietyOdebraneOkno(0)
    , m_ostatniDesync(0)
    , m_oczekujeNaOdpowiedz(false)
    , m_maNoweSterowanie(false)
    , m_rozlaczanieCelowe(false)
    , m_byloPolaczenie(false)
    , m_ostatniIndeksTaktowaniaSieci(0)
{
    konfigurujGUI();
    konfigurujWykresy();

    connect(&petla, &ProstyUAR::krokWykonany,
            this,   &MainWindow::onKrokWykonany);
    m_timerSieciowy->setTimerType(Qt::PreciseTimer);
    connect(m_timerSieciowy, &QTimer::timeout, this, &MainWindow::onSieciowyTimeout);

    aktualizujGenerator();
    aktualizujOknoCzasowe();
    petla.setInterwal(spinInterwal->value());
    m_timerSieciowy->setInterval(spinInterwal->value());

    sieci.ustawParametry();
    connect(&sieci, &Sieci::polaczono,          this, &MainWindow::onSieciPolaczono);
    connect(&sieci, &Sieci::rozlaczono,         this, &MainWindow::onSieciRozlaczono);
    connect(&sieci, &Sieci::statusZmieniony,    this, &MainWindow::onSieciStatusZmieniony);
    connect(&sieci, &Sieci::odebranoKonfiguracje, this, &MainWindow::onSieciOdebranoKonfiguracje);
    connect(&sieci, &Sieci::odebranoProbke,     this, &MainWindow::onSieciOdebranoProbke);

    aktualizujStanKontrolek();
    ustawWskaznikPolaczenia(false);
    ustawLampkeWydajnosci(true, "Brak aktywnej transmisji");
    m_zegarWydajnosci.start();
}


void MainWindow::onKrokWykonany(double czas, double w, double y, double e, double u)
{
    aktualizujDaneWykresow(czas, w, y, e, u);
}


void MainWindow::przelaczSymulacje()
{
    if (czyTrybSieciowy()) {
        if (m_timerSieciowy->isActive()) {
            if (sieci.get_tryb() == Sieci::Tryb::Regulator)
                wyslijKomendeSieciowa("stop");
            zatrzymajSymulacjeSieciowa();
        } else {
            uruchomSymulacjeSieciowa();
        }
        aktualizujStanKontrolek();
        return;
    }

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
    const bool wyslacResetDoPartnera = czyTrybSieciowy() && sieci.czyPolaczony() && !m_aplikujeZdalna;

    petla.reset();
    zatrzymajSymulacjeSieciowa();
    zresetujStanSieciowy();
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

    if (wyslacResetDoPartnera)
        wyslijKomendeSieciowa("reset");
}

void MainWindow::aktualizujInterwal()
{
    petla.setInterwal(spinInterwal->value());
    m_timerSieciowy->setInterval(spinInterwal->value());
    m_ostatnieDtSieciowe = spinInterwal->value() / 1000.0;
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
    comboTaktowanieSieci = new QComboBox();
    comboTaktowanieSieci->addItem("Taktowanie jednostronne", 0);
    comboTaktowanieSieci->addItem("Taktowanie obustronne", 1);
    connect(comboTaktowanieSieci, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_timerSieciowy->isActive()) {
            QMessageBox::warning(this,
                                 "Zmiana taktowania",
                                 "Najpierw zatrzymaj symulacje, a dopiero potem zmien taktowanie.");
            QSignalBlocker blokada(comboTaktowanieSieci);
            comboTaktowanieSieci->setCurrentIndex(m_ostatniIndeksTaktowaniaSieci);
            return;
        }

        if (czyTrybSieciowy() && sieci.czyPolaczony()) {
            const auto decyzja = QMessageBox::question(
                this,
                "Zmiana taktowania",
                "Zmiana taktowania zatrzyma i zresetuje stan sieciowy po obu stronach. Kontynuowac?");

            if (decyzja != QMessageBox::Yes) {
                QSignalBlocker blokada(comboTaktowanieSieci);
                comboTaktowanieSieci->setCurrentIndex(m_ostatniIndeksTaktowaniaSieci);
                return;
            }
        }

        if (czyTrybSieciowy()) {
            zatrzymajSymulacjeSieciowa();
            zresetujStanSieciowy();
            wyslijKomendeSieciowa("reset");
        }
        m_ostatniIndeksTaktowaniaSieci = comboTaktowanieSieci->currentIndex();
        aktualizujStanKontrolek();
        wyslijKonfiguracjeJesliPolaczony();
    });
    layoutAdres->addRow("Adres IP:", edycjaIp);
    layoutAdres->addRow("Port:", spinPort);
    layoutAdres->addRow("Taktowanie:", comboTaktowanieSieci);

    comboSerializacja = new QComboBox();
    comboSerializacja->addItem("Binarna", 0);
    comboSerializacja->addItem("Tekstowa (JSON)", 1);
    connect(comboSerializacja, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        sieci.setSerializacjaProbek(idx == 0 ? Sieci::SER_BINARNA : Sieci::SER_TEKSTOWA);
    });
    layoutAdres->addRow("Serializacja:", comboSerializacja);

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

    lblPartnerSieci = new QLabel("Partner: brak");
    lblSyncSieci = new QLabel("Sync: brak danych");
    lblSterowanieSieci = new QLabel("Sterowanie: brak danych");
    lblWydajnoscSieci = new QLabel("Pakiety/s: 0.0");
    lblShadowSieci = new QLabel("Shadow: brak danych");
    lblPartnerSieci->setWordWrap(true);
    lblSyncSieci->setWordWrap(true);
    lblSterowanieSieci->setWordWrap(true);
    lblShadowSieci->setWordWrap(true);

    QHBoxLayout *layoutWydajnosc = new QHBoxLayout();
    lblLampkaWydajnosci = new QLabel();
    lblLampkaWydajnosci->setFixedSize(18, 18);
    layoutWydajnosc->addWidget(lblLampkaWydajnosci);
    layoutWydajnosc->addWidget(lblWydajnoscSieci, 1);

    layoutSieci->addWidget(lblPartnerSieci);
    layoutSieci->addWidget(lblSyncSieci);
    layoutSieci->addWidget(lblSterowanieSieci);
    layoutSieci->addLayout(layoutWydajnosc);
    layoutSieci->addWidget(lblShadowSieci);
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
    aktualizujDaneWykresow(t,
                           w,
                           y,
                           e,
                           u,
                           pid.pobierzOstatnieP(),
                           pid.pobierzOstatnieI(),
                           pid.pobierzOstatnieD());
}

void MainWindow::aktualizujDaneWykresow(
    double t, double w, double y, double e, double u, double p, double i, double d)
{
    seriaZadana->append(t, w);
    seriaWyjscie->append(t, y);
    seriaUchyb->append(t, e);
    seriaSterowanie->append(t, u);
    seriaP->append(t, p);
    seriaI->append(t, i);
    seriaD->append(t, d);

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
            const QList<QPointF> punkty = s->points();
            for (int i = punkty.size() - 1; i >= 0; --i) {
                const QPointF &p = punkty[i];
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
    root["TaktowanieSieci"] = comboTaktowanieSieci->currentIndex();

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

    if (root.contains("TaktowanieSieci")) {
        int indeks = root["TaktowanieSieci"].toInt();
        if (indeks < 0 || indeks > 1)
            indeks = 0;
        QSignalBlocker blokada(comboTaktowanieSieci);
        comboTaktowanieSieci->setCurrentIndex(indeks);
        m_ostatniIndeksTaktowaniaSieci = indeks;
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
    auto decyzja = QMessageBox::question(this,
        "Zmiana trybu pracy",
        "Czy na pewno chcesz przejsc w tryb regulatora sieciowego?\n"
        "Lokalna symulacja zostanie zatrzymana.");
    if (decyzja != QMessageBox::Yes)
        return;

    petla.stop();
    zatrzymajSymulacjeSieciowa();
    zresetujStanSieciowy();
    btnStartStop->setText("Start");
    sieci.ustawParametry(static_cast<quint16>(spinPort->value()), edycjaIp->text());
    sieci.set_tryb(Sieci::Tryb::Regulator);
    aktualizujStanKontrolek();
}

void MainWindow::wlaczTrybObiekt()
{
    auto decyzja = QMessageBox::question(this,
        "Zmiana trybu pracy",
        "Czy na pewno chcesz przejsc w tryb obiektu sieciowego?\n"
        "Lokalna symulacja zostanie zatrzymana.");
    if (decyzja != QMessageBox::Yes)
        return;

    petla.stop();
    zatrzymajSymulacjeSieciowa();
    zresetujStanSieciowy();
    btnStartStop->setText("Start");
    sieci.ustawParametry(static_cast<quint16>(spinPort->value()), edycjaIp->text());
    sieci.set_tryb(Sieci::Tryb::Obiekt);
    aktualizujStanKontrolek();
}

void MainWindow::rozlaczSiec()
{
    auto decyzja = QMessageBox::question(this,
        "Zmiana trybu pracy",
        "Czy na pewno chcesz rozlaczyc i przejsc w tryb stacjonarny?");
    if (decyzja != QMessageBox::Yes)
        return;

    const bool symulacjaAktywna = m_timerSieciowy->isActive();
    m_rozlaczanieCelowe = true;
    zatrzymajSymulacjeSieciowa();
    sieci.rozlacz();

    if (symulacjaAktywna) {
        kontynuujLokalniePoRozlaczeniu();
    } else {
        aktualizujStanKontrolek();
    }
}

void MainWindow::onSieciPolaczono()
{
    m_byloPolaczenie = true;
    m_rozlaczanieCelowe = false;
    ustawWskaznikPolaczenia(true);
    zresetujStanSieciowy();
    aktualizujInformacjePartnera();
    aktualizujStanKontrolek();

    if (sieci.get_tryb() == Sieci::Tryb::Regulator) {
        sieci.wyslijKonfiguracje(zbudujKonfiguracjeJson());
    }
}

void MainWindow::onSieciRozlaczono()
{
    const bool bylaAktywnaSymulacja = m_timerSieciowy->isActive()
                                      || m_numerProbkiSieciowej > 0
                                      || m_ostatniSeqSterowania > 0;
    const bool zerwanoNieoczekiwanie = m_byloPolaczenie && !m_rozlaczanieCelowe;

    zatrzymajSymulacjeSieciowa();
    ustawWskaznikPolaczenia(false);
    aktualizujInformacjePartnera();
    aktualizujStanKontrolek();

    if (zerwanoNieoczekiwanie) {
        QMessageBox::warning(this,
                             "Zerwano polaczenie",
                             "Polaczenie sieciowe zostalo zerwane. Symulacja moze byc kontynuowana lokalnie na aktualnych ustawieniach.");
        if (bylaAktywnaSymulacja)
            kontynuujLokalniePoRozlaczeniu();
    }

    m_byloPolaczenie = false;
    m_rozlaczanieCelowe = false;
}

void MainWindow::onSieciStatusZmieniony(const QString &opis)
{
    lblStatusSieci->setText(opis);
}

void MainWindow::onSieciOdebranoKonfiguracje(const QJsonObject &konfig)
{
    zastosujKonfiguracjeJson(konfig);
}

void MainWindow::onSieciOdebranoProbke(const QJsonObject &probka)
{
    zarejestrujPakietOdebrany();

    const QString rodzaj = probka["rodzaj"].toString();

    if (rodzaj == "komenda") {
        obsluzKomendeSieciowa(probka);
        return;
    }

    if (rodzaj == "sterowanie") {
        obsluzSterowanieSieciowe(probka);
    } else if (rodzaj == "wyjscie") {
        obsluzWyjscieSieciowe(probka);
    }
}

void MainWindow::obsluzKomendeSieciowa(const QJsonObject &probka)
{
    const QString komenda = probka["komenda"].toString();

    if (komenda == "start") {
        if (sieci.get_tryb() == Sieci::Tryb::Obiekt && !czySiecJednostronna()) {
            m_timerSieciowy->setInterval(spinInterwal->value());
            m_timerSieciowy->start();
            btnStartStop->setText("Stop");
            lblStatusSieci->setText("Obiekt: start z regulatora.");
            aktualizujStanKontrolek();
        }
    } else if (komenda == "stop") {
        zatrzymajSymulacjeSieciowa();
        lblStatusSieci->setText("Stop odebrany z regulatora.");
        aktualizujStanKontrolek();
    } else if (komenda == "reset") {
        m_aplikujeZdalna = true;
        zresetujSymulacje();
        m_aplikujeZdalna = false;
        lblStatusSieci->setText("Reset odebrany z partnera.");
        aktualizujStanKontrolek();
    }
}

void MainWindow::onSieciowyTimeout()
{
    if (sieci.get_tryb() == Sieci::Tryb::Regulator) {
        wykonajKrokRegulatoraSieciowego();
    } else if (sieci.get_tryb() == Sieci::Tryb::Obiekt && !czySiecJednostronna()) {
        wykonajKrokObiektuSieciowego();
    }
}

bool MainWindow::czyTrybSieciowy() const
{
    return sieci.get_tryb() != Sieci::Tryb::Nieokreslony;
}

MainWindow::TaktowanieSieci MainWindow::aktualneTaktowanieSieci() const
{
    if (comboTaktowanieSieci && comboTaktowanieSieci->currentIndex() == 1)
        return TaktowanieSieci::Obustronne;
    return TaktowanieSieci::Jednostronne;
}

bool MainWindow::czySiecJednostronna() const
{
    return aktualneTaktowanieSieci() == TaktowanieSieci::Jednostronne;
}

void MainWindow::zresetujStanSieciowy()
{
    m_czasSieciowy = 0.0;
    m_ostatnieYSieciowe = 0.0;
    m_ostatnieUSieciowe = 0.0;
    m_ostatnieWSieciowe = 0.0;
    m_ostatnieESieciowe = 0.0;
    m_ostatniePSieciowe = 0.0;
    m_ostatnieISieciowe = 0.0;
    m_ostatnieDSieciowe = 0.0;
    m_ostatnieDtSieciowe = spinInterwal->value() / 1000.0;
    m_shadowYSieciowe = 0.0;
    m_shadowUSieciowe = 0.0;
    m_numerProbkiSieciowej = 0;
    m_ostatniSeqSterowania = 0;
    m_ostatniSeqOdpowiedzi = 0;
    m_brakiSterowaniaPodRzad = 0;
    m_ostatniDesync = 0;
    m_oczekujeNaOdpowiedz = false;
    m_maNoweSterowanie = false;

    if (lblSyncSieci)
        lblSyncSieci->setText("Sync: brak danych");
    if (lblSterowanieSieci)
        lblSterowanieSieci->setText("Sterowanie: brak danych");
    if (lblShadowSieci)
        lblShadowSieci->setText("Shadow: brak danych");
}

void MainWindow::uruchomSymulacjeSieciowa()
{
    if (!sieci.czyPolaczony()) {
        lblStatusSieci->setText("Najpierw połącz aplikacje w sieci.");
        return;
    }

    if (sieci.get_tryb() == Sieci::Tryb::Obiekt && czySiecJednostronna()) {
        lblStatusSieci->setText("Obiekt: w taktowaniu jednostronnym czeka na kroki regulatora.");
        return;
    }

    m_timerSieciowy->setInterval(spinInterwal->value());
    m_timerSieciowy->start();
    btnStartStop->setText("Stop");

    if (sieci.get_tryb() == Sieci::Tryb::Regulator) {
        if (!czySiecJednostronna())
            wyslijKomendeSieciowa("start");
        lblStatusSieci->setText("Regulator: wysyłanie próbek sterowania.");
    } else {
        lblStatusSieci->setText("Obiekt: własny timer aktywny, odsyłanie próbek wyjścia.");
    }
}

void MainWindow::zatrzymajSymulacjeSieciowa()
{
    if (m_timerSieciowy->isActive())
        m_timerSieciowy->stop();

    m_oczekujeNaOdpowiedz = false;
    m_maNoweSterowanie = false;
    btnStartStop->setText("Start");
}

void MainWindow::wykonajKrokRegulatoraSieciowego()
{
    if (!sieci.czyPolaczony())
        return;

    if (czySiecJednostronna() && m_oczekujeNaOdpowiedz)
        return;

    const double dt = spinInterwal->value() / 1000.0;
    m_ostatnieDtSieciowe = dt;
    m_czasSieciowy += dt;
    ++m_numerProbkiSieciowej;

    const double w = gen.generuj(m_czasSieciowy);
    const double e = w - m_ostatnieYSieciowe;
    const double u = pid.symuluj(e, dt);

    m_ostatnieWSieciowe = w;
    m_ostatnieESieciowe = e;
    m_ostatnieUSieciowe = u;
    m_ostatniePSieciowe = pid.pobierzOstatnieP();
    m_ostatnieISieciowe = pid.pobierzOstatnieI();
    m_ostatnieDSieciowe = pid.pobierzOstatnieD();
    m_shadowYSieciowe = arx.symuluj(u);

    QJsonObject probka;
    probka["rodzaj"] = "sterowanie";
    probka["seq"] = m_numerProbkiSieciowej;
    probka["t"] = m_czasSieciowy;
    probka["dt"] = dt;
    probka["w"] = w;
    probka["y_poprzednie"] = m_ostatnieYSieciowe;
    probka["e"] = e;
    probka["u"] = u;
    probka["P"] = m_ostatniePSieciowe;
    probka["I"] = m_ostatnieISieciowe;
    probka["D"] = m_ostatnieDSieciowe;
    probka["shadow_y"] = m_shadowYSieciowe;
    probka["taktowanie"] = comboTaktowanieSieci->currentIndex();

    wyslijProbkeSieciowa(probka);
    m_oczekujeNaOdpowiedz = czySiecJednostronna();

    lblSterowanieSieci->setText(QString("Sterowanie: wyslano seq %1, u=%2")
                                    .arg(m_numerProbkiSieciowej)
                                    .arg(u, 0, 'f', 3));
}

void MainWindow::obsluzSterowanieSieciowe(const QJsonObject &probka)
{
    if (sieci.get_tryb() != Sieci::Tryb::Obiekt)
        return;

    m_ostatniSeqSterowania = probka["seq"].toInt();
    m_czasSieciowy = probka["t"].toDouble(m_czasSieciowy);
    m_ostatnieDtSieciowe = probka["dt"].toDouble(spinInterwal->value() / 1000.0);
    m_ostatnieWSieciowe = probka["w"].toDouble();
    m_ostatnieESieciowe = probka["e"].toDouble();
    m_ostatnieUSieciowe = probka["u"].toDouble();
    m_ostatniePSieciowe = probka["P"].toDouble();
    m_ostatnieISieciowe = probka["I"].toDouble();
    m_ostatnieDSieciowe = probka["D"].toDouble();
    m_maNoweSterowanie = true;
    m_brakiSterowaniaPodRzad = 0;
    lblSterowanieSieci->setText(QString("Sterowanie: odebrano seq %1, u=%2")
                                    .arg(m_ostatniSeqSterowania)
                                    .arg(m_ostatnieUSieciowe, 0, 'f', 3));

    if (czySiecJednostronna())
        wykonajKrokObiektuSieciowego();
}

void MainWindow::wykonajKrokObiektuSieciowego()
{
    if (!sieci.czyPolaczony() || sieci.get_tryb() != Sieci::Tryb::Obiekt)
        return;

    if (!czySiecJednostronna() && !m_maNoweSterowanie && m_ostatniSeqSterowania == 0) {
        lblSterowanieSieci->setText("Sterowanie: czekam na pierwsze u z regulatora.");
        return;
    }

    const bool brakNowegoSterowania = !czySiecJednostronna() && !m_maNoweSterowanie;
    if (brakNowegoSterowania) {
        ++m_brakiSterowaniaPodRzad;
        lblSterowanieSieci->setText(QString("Sterowanie: brak nowego u, trzymam poprzednie (%1 krokow).")
                                        .arg(m_brakiSterowaniaPodRzad));
    }

    const double eShadow = m_ostatnieWSieciowe - m_ostatnieYSieciowe;
    m_shadowUSieciowe = pid.symuluj(eShadow, m_ostatnieDtSieciowe);
    const double y = arx.symuluj(m_ostatnieUSieciowe);
    m_ostatnieYSieciowe = y;

    QJsonObject odpowiedz;
    odpowiedz["rodzaj"] = "wyjscie";
    odpowiedz["seq"] = m_ostatniSeqSterowania;
    odpowiedz["t"] = m_czasSieciowy;
    odpowiedz["w"] = m_ostatnieWSieciowe;
    odpowiedz["e"] = m_ostatnieESieciowe;
    odpowiedz["u"] = m_ostatnieUSieciowe;
    odpowiedz["y"] = y;
    odpowiedz["P"] = m_ostatniePSieciowe;
    odpowiedz["I"] = m_ostatnieISieciowe;
    odpowiedz["D"] = m_ostatnieDSieciowe;
    odpowiedz["dt"] = m_ostatnieDtSieciowe;
    odpowiedz["shadow_u"] = m_shadowUSieciowe;
    odpowiedz["brak_nowego_u"] = brakNowegoSterowania;

    wyslijProbkeSieciowa(odpowiedz);
    aktualizujDaneWykresow(m_czasSieciowy,
                           m_ostatnieWSieciowe,
                           y,
                           m_ostatnieESieciowe,
                           m_ostatnieUSieciowe,
                           m_ostatniePSieciowe,
                           m_ostatnieISieciowe,
                           m_ostatnieDSieciowe);

    lblShadowSieci->setText(QString("Shadow: lokalny PID u=%1, roznica=%2")
                                .arg(m_shadowUSieciowe, 0, 'f', 3)
                                .arg(m_shadowUSieciowe - m_ostatnieUSieciowe, 0, 'f', 3));

    m_maNoweSterowanie = false;
}

void MainWindow::obsluzWyjscieSieciowe(const QJsonObject &probka)
{
    if (sieci.get_tryb() != Sieci::Tryb::Regulator)
        return;

    const int seq = probka["seq"].toInt();
    if (seq < m_ostatniSeqOdpowiedzi) {
        lblSyncSieci->setText(QString("Sync: pominieto stara odpowiedz seq %1").arg(seq));
        return;
    }

    m_ostatniSeqOdpowiedzi = seq;
    m_ostatnieYSieciowe = probka["y"].toDouble();
    m_oczekujeNaOdpowiedz = false;

    const qint64 nadanoMs = probka["nadano_ms"].toVariant().toLongLong();
    const qint64 opoznienieMs = nadanoMs > 0 ? QDateTime::currentMSecsSinceEpoch() - nadanoMs : 0;
    const int desync = m_numerProbkiSieciowej - seq;
    ustawStatusSynchronizacji(desync, opoznienieMs);

    aktualizujDaneWykresow(probka["t"].toDouble(),
                           probka["w"].toDouble(),
                           m_ostatnieYSieciowe,
                           probka["e"].toDouble(),
                           probka["u"].toDouble(),
                           probka["P"].toDouble(),
                           probka["I"].toDouble(),
                           probka["D"].toDouble());

    const QString brakU = probka["brak_nowego_u"].toBool() ? ", obiekt trzymal poprzednie u" : "";
    lblSterowanieSieci->setText(QString("Sterowanie: odpowiedz seq %1%2").arg(seq).arg(brakU));
    lblShadowSieci->setText(QString("Shadow: lokalny ARX y=%1, roznica=%2")
                                .arg(m_shadowYSieciowe, 0, 'f', 3)
                                .arg(m_shadowYSieciowe - m_ostatnieYSieciowe, 0, 'f', 3));
}

void MainWindow::wyslijProbkeSieciowa(QJsonObject probka)
{
    if (!sieci.czyPolaczony())
        return;

    probka["nadano_ms"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    sieci.wyslijProbke(probka);
    ++m_pakietyWyslaneOkno;
    odswiezWydajnoscSieci();
}

void MainWindow::wyslijKomendeSieciowa(const QString &komenda)
{
    QJsonObject probka;
    probka["rodzaj"] = "komenda";
    probka["komenda"] = komenda;
    probka["seq"] = m_numerProbkiSieciowej;
    probka["t"] = m_czasSieciowy;
    probka["dt"] = spinInterwal->value() / 1000.0;
    probka["taktowanie"] = comboTaktowanieSieci->currentIndex();
    wyslijProbkeSieciowa(probka);
}

void MainWindow::zarejestrujPakietOdebrany()
{
    ++m_pakietyOdebraneOkno;
    odswiezWydajnoscSieci();
}

void MainWindow::odswiezWydajnoscSieci()
{
    if (!m_zegarWydajnosci.isValid())
        m_zegarWydajnosci.start();

    const qint64 ms = m_zegarWydajnosci.elapsed();
    if (ms < 1000)
        return;

    const int pakiety = m_pakietyWyslaneOkno + m_pakietyOdebraneOkno;
    const double pakietyNaSekunde = pakiety * 1000.0 / ms;
    lblWydajnoscSieci->setText(QString("Pakiety/s: %1 (TX %2, RX %3)")
                                   .arg(pakietyNaSekunde, 0, 'f', 1)
                                   .arg(m_pakietyWyslaneOkno)
                                   .arg(m_pakietyOdebraneOkno));

    const bool saProbki = (m_numerProbkiSieciowej > 0 || m_ostatniSeqSterowania > 0);
    if (saProbki) {
        const double oczekiwane = 2000.0 / qMax(1, spinInterwal->value());
        const bool wydajnoscOk = pakietyNaSekunde >= oczekiwane * 0.6;
        const bool syncOk = std::abs(m_ostatniDesync) <= 2;
        const bool sterowanieOk = m_brakiSterowaniaPodRzad < 3;
        ustawLampkeWydajnosci(wydajnoscOk && syncOk && sterowanieOk,
                              wydajnoscOk && syncOk && sterowanieOk
                                  ? "Wyrabia sie"
                                  : "Nie wyrabia sie: sprawdz pakiety/s, desync albo brak sterowania");
    } else {
        ustawLampkeWydajnosci(true, "Polaczenie gotowe, brak aktywnej transmisji probek");
    }

    m_pakietyWyslaneOkno = 0;
    m_pakietyOdebraneOkno = 0;
    m_zegarWydajnosci.restart();
}

void MainWindow::ustawLampkeWydajnosci(bool ok, const QString &opis)
{
    const QString kolor = ok ? "#2ecc40" : "#ff4136";
    const QString ramka = ok ? "#186a1f" : "#8a1f17";
    lblLampkaWydajnosci->setStyleSheet(
        QString("background-color: %1; border-radius: 9px; border: 1px solid %2;")
            .arg(kolor, ramka));
    lblLampkaWydajnosci->setToolTip(opis);
    lblWydajnoscSieci->setToolTip(opis);
}

void MainWindow::ustawStatusSynchronizacji(int desync, qint64 opoznienieMs)
{
    m_ostatniDesync = desync;
    lblSyncSieci->setText(QString("Sync: desync=%1 probek, opoznienie=%2 ms")
                              .arg(desync)
                              .arg(opoznienieMs));

    const bool ok = std::abs(desync) <= 1 && opoznienieMs <= spinInterwal->value() * 2;
    if (!ok)
        ustawLampkeWydajnosci(false, "Wykryto opoznienie albo desynchronizacje probek");
}

void MainWindow::aktualizujInformacjePartnera()
{
    lblPartnerSieci->setText(QString("Partner: %1").arg(sieci.opisPartnera()));
}

void MainWindow::kontynuujLokalniePoRozlaczeniu()
{
    petla.ustawStan(m_czasSieciowy, m_ostatnieYSieciowe);
    petla.start();
    btnStartStop->setText("Stop");
    lblStatusSieci->setText("Tryb lokalny: kontynuacja po zerwaniu polaczenia.");
    aktualizujStanKontrolek();
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
    const bool lokalnie = !trybWybrany;
    const bool obiektZTimerem = obiekt && !czySiecJednostronna();

    bool mozePID = lokalnie || regulator;
    grpPid->setEnabled(mozePID);
    grpGen->setEnabled(mozePID);

    bool mozeARX = lokalnie || obiekt;
    btnArx->setEnabled(mozeARX);

    bool mozeStart = lokalnie || (polaczony && (regulator || obiektZTimerem));
    btnStartStop->setEnabled(mozeStart);
    btnReset->setEnabled(lokalnie || trybWybrany);
    spinInterwal->setEnabled(lokalnie || regulator || obiektZTimerem);
    comboTaktowanieSieci->setEnabled(lokalnie || (regulator && !m_timerSieciowy->isActive()));

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QtCharts>

#include "Generator.h"
#include "ModelARX.h"
#include "ProstyUAR.h"
#include "RegulatorPID.h"
#include "Sieci.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    Sieci sieci;

private slots:
    void onKrokWykonany(double czas, double w, double y, double e, double u);

    void przelaczSymulacje();
    void zresetujSymulacje();
    void otworzKonfiguracjeARX();

    void aktualizujPID();
    void aktualizujGenerator();
    void aktualizujInterwal();
    void aktualizujOknoCzasowe();

    void zresetujCalkePID();
    void zapiszKonfiguracje();
    void wczytajKonfiguracje();

    // Sloty sieciowe
    void wlaczTrybRegulator();
    void wlaczTrybObiekt();
    void rozlaczSiec();
    void onSieciPolaczono();
    void onSieciRozlaczono();
    void onSieciStatusZmieniony(const QString &opis);
    void onSieciOdebranoKonfiguracje(const QJsonObject &konfig);
    void onSieciOdebranoProbke(const QJsonObject &probka);
    void onSieciowyTimeout();

private:
    enum class TaktowanieSieci { Jednostronne, Obustronne };

    ModelARX arx;
    RegulatorPID pid;
    Generator gen;
    ProstyUAR petla;

    double oknoCzasowe;
    bool   m_aplikujeZdalna;
    QTimer *m_timerSieciowy;
    double m_czasSieciowy;
    double m_ostatnieYSieciowe;
    double m_ostatnieUSieciowe;
    double m_ostatnieWSieciowe;
    double m_ostatnieESieciowe;
    double m_ostatniePSieciowe;
    double m_ostatnieISieciowe;
    double m_ostatnieDSieciowe;
    double m_ostatnieDtSieciowe;
    double m_shadowYSieciowe;
    double m_shadowUSieciowe;
    int    m_numerProbkiSieciowej;
    int    m_ostatniSeqSterowania;
    int    m_ostatniSeqOdpowiedzi;
    int    m_brakiSterowaniaPodRzad;
    int    m_pakietyWyslaneOkno;
    int    m_pakietyOdebraneOkno;
    int    m_ostatniDesync;
    bool   m_oczekujeNaOdpowiedz;
    bool   m_maNoweSterowanie;
    bool   m_rozlaczanieCelowe;
    bool   m_byloPolaczenie;
    int    m_ostatniIndeksTaktowaniaSieci;
    QElapsedTimer m_zegarWydajnosci;

    // GUI - Kontrolki
    QLineEdit *edycjaKp, *edycjaTi, *edycjaTd;
    QComboBox *comboMetodaCalk;

    QComboBox *comboGenTyp;
    QDoubleSpinBox *spinGenOffset, *spinGenAmp, *spinGenOkres;
    QDoubleSpinBox *spinGenWypelnienie;
    QDoubleSpinBox *spinGenCzasAkt;

    QSpinBox *spinInterwal;
    QDoubleSpinBox *spinOknoCzasowe;

    QPushButton *btnStartStop;
    QPushButton *btnReset;
    QPushButton *btnArx;
    QPushButton *btnResetI;
    QPushButton *btnZapiszJson;
    QPushButton *btnWczytajJson;

    // GUI - Sieci
    QPushButton *btnTrybRegulator;
    QPushButton *btnTrybObiekt;
    QPushButton *btnRozlacz;
    QLineEdit *edycjaIp;
    QSpinBox *spinPort;
    QLabel *lblStatusSieci;
    QLabel *lblWskaznikPolaczenia;
    QComboBox *comboTaktowanieSieci;
    QComboBox *comboSerializacja;
    QLabel *lblPartnerSieci;
    QLabel *lblSyncSieci;
    QLabel *lblSterowanieSieci;
    QLabel *lblWydajnoscSieci;
    QLabel *lblLampkaWydajnosci;
    QLabel *lblShadowSieci;

    QGroupBox *grpPid;
    QGroupBox *grpGen;
    QGroupBox *grpSym;

    // Wykresy
    QLineSeries *seriaZadana, *seriaWyjscie, *seriaUchyb, *seriaSterowanie;
    QLineSeries *seriaP, *seriaI, *seriaD;
    QChart *wykresGlowny, *wykresUchyb, *wykresSterowanie, *wykresPID;
    QValueAxis *osXGlowny, *osYGlowny, *osXUchyb, *osYUchyb;
    QValueAxis *osXSterowanie, *osYSterowanie, *osXPID, *osYPID;

    void konfigurujGUI();
    void konfigurujWykresy();
    void aktualizujDaneWykresow(double t, double w, double y, double e, double u);
    void aktualizujDaneWykresow(
        double t, double w, double y, double e, double u, double p, double i, double d);

    QJsonObject zbudujKonfiguracjeJson() const;
    void zastosujKonfiguracjeJson(const QJsonObject &konfig);
    void wyslijKonfiguracjeJesliPolaczony();
    void aktualizujStanKontrolek();
    void ustawWskaznikPolaczenia(bool polaczony);
    void zresetujStanSieciowy();
    void uruchomSymulacjeSieciowa();
    void zatrzymajSymulacjeSieciowa();
    bool czyTrybSieciowy() const;
    bool czySiecJednostronna() const;
    TaktowanieSieci aktualneTaktowanieSieci() const;
    void wykonajKrokRegulatoraSieciowego();
    void wykonajKrokObiektuSieciowego();
    void obsluzSterowanieSieciowe(const QJsonObject &probka);
    void obsluzWyjscieSieciowe(const QJsonObject &probka);
    void obsluzKomendeSieciowa(const QJsonObject &probka);
    void wyslijProbkeSieciowa(QJsonObject probka);
    void wyslijKomendeSieciowa(const QString &komenda);
    void zarejestrujPakietOdebrany();
    void odswiezWydajnoscSieci();
    void ustawLampkeWydajnosci(bool ok, const QString &opis);
    void ustawStatusSynchronizacji(int desync, qint64 opoznienieMs);
    void aktualizujInformacjePartnera();
    void kontynuujLokalniePoRozlaczeniu();
};

#endif // MAINWINDOW_H

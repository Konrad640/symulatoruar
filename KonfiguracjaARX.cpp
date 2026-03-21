#include "KonfiguracjaARX.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

KonfiguracjaARX::KonfiguracjaARX(ModelARX *model, QWidget *parent)
    : QDialog(parent)
    , modelArx(model)
{
    setWindowTitle("Konfiguracja Modelu ARX");
    setModal(false);
    resize(350, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    std::vector<double> wekA = modelArx->getA();
    std::vector<double> wekB = modelArx->getB();
    if (wekA.size() < 3)
        wekA.resize(3, 0.0);
    if (wekB.size() < 3)
        wekB.resize(3, 0.0);

    QGroupBox *groupA = new QGroupBox("Współczynniki A");
    QVBoxLayout *layoutA = new QVBoxLayout(groupA);

    layoutA->addWidget(new QLabel("a1"));
    spinA1 = new QDoubleSpinBox();
    spinA1->setRange(-10.00, 10.0);
    spinA1->setSingleStep(0.01);
    spinA1->setDecimals(2);
    spinA1->setValue(wekA[0]);
    layoutA->addWidget(spinA1);

    layoutA->addWidget(new QLabel("a2"));
    spinA2 = new QDoubleSpinBox();
    spinA2->setRange(-10.00, 10.0);
    spinA2->setSingleStep(0.01);
    spinA2->setDecimals(2);
    spinA2->setValue(wekA[1]);
    layoutA->addWidget(spinA2);

    layoutA->addWidget(new QLabel("a3"));
    spinA3 = new QDoubleSpinBox();
    spinA3->setRange(-10.00, 10.0);
    spinA3->setSingleStep(0.01);
    spinA3->setDecimals(2);
    spinA3->setValue(wekA[2]);
    layoutA->addWidget(spinA3);

    mainLayout->addWidget(groupA);

    QGroupBox *groupB = new QGroupBox("Współczynniki B");
    QVBoxLayout *layoutB = new QVBoxLayout(groupB);

    layoutB->addWidget(new QLabel("b1"));
    spinB1 = new QDoubleSpinBox();
    spinB1->setRange(-10.0, 10.00);
    spinB1->setSingleStep(0.01);
    spinB1->setDecimals(2);
    spinB1->setValue(wekB[0]);
    layoutB->addWidget(spinB1);

    layoutB->addWidget(new QLabel("b2"));
    spinB2 = new QDoubleSpinBox();
    spinB2->setRange(-10.0, 10.00);
    spinB2->setSingleStep(0.01);
    spinB2->setDecimals(2);
    spinB2->setValue(wekB[1]);
    layoutB->addWidget(spinB2);

    layoutB->addWidget(new QLabel("b3"));
    spinB3 = new QDoubleSpinBox();
    spinB3->setRange(-10.0, 10.00);
    spinB3->setSingleStep(0.01);
    spinB3->setDecimals(2);
    spinB3->setValue(wekB[2]);
    layoutB->addWidget(spinB3);

    mainLayout->addWidget(groupB);

    QGroupBox *groupOther = new QGroupBox("Pozostałe parametry");
    QVBoxLayout *layoutOther = new QVBoxLayout(groupOther);

    layoutOther->addWidget(new QLabel("Opóźnienie transportowe (k):"));
    spinOpoznienie = new QSpinBox();
    spinOpoznienie->setRange(1, 1000);
    spinOpoznienie->setValue(modelArx->getOpoznienie());
    layoutOther->addWidget(spinOpoznienie);

    layoutOther->addWidget(new QLabel("Odchylenie std. szumu:"));
    spinSzum = new QDoubleSpinBox();
    spinSzum->setRange(0.0, 100.0);
    spinSzum->setSingleStep(0.01);

    spinSzum->setValue(modelArx->getSzum());
    layoutOther->addWidget(spinSzum);

    mainLayout->addWidget(groupOther);

    QPushButton *btnZamknij = new QPushButton("Zastosuj i Zamknij");
    connect(btnZamknij, &QPushButton::clicked, this, &KonfiguracjaARX::close);
    mainLayout->addWidget(btnZamknij);
}

void KonfiguracjaARX::closeEvent(QCloseEvent *event)
{
    zapiszZmiany();
    QDialog::closeEvent(event);
}

void KonfiguracjaARX::zapiszZmiany()
{
    std::vector<double> noweA;
    noweA.push_back(spinA1->value());
    noweA.push_back(spinA2->value());
    noweA.push_back(spinA3->value());

    std::vector<double> noweB;
    noweB.push_back(spinB1->value());
    noweB.push_back(spinB2->value());
    noweB.push_back(spinB3->value());

    modelArx->setA(noweA);
    modelArx->setB(noweB);
    modelArx->setOpoznienie(spinOpoznienie->value());
    modelArx->setSzum(spinSzum->value());
}

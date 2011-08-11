#ifndef TOUHOUEQUIPS_H
#define TOUHOUEQUIPS_H

#include<touhoucards.h>

class Hakkero : public Weapon
{
    Q_OBJECT
public:
    Q_INVOKABLE Hakkero(Card::Suit suit, int number = 1);
};

class TenguFan : public Weapon
{
    Q_OBJECT
public:
    Q_INVOKABLE TenguFan(Card::Suit suit, int number = 1);
};

class Hisonoken : public Weapon
{
    Q_OBJECT
public:
    Q_INVOKABLE Hisonoken(Card::Suit suit, int number = 2);
};

class Gungnir : public Weapon
{
    Q_OBJECT
public:
    Q_INVOKABLE Gungnir(Card::Suit suit, int number = 3);
};

#endif // TOUHOUEQUIPS_H

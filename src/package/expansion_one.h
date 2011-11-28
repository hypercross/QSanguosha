#ifndef EXPANSION_ONE_H
#define EXPANSION_ONE_H

#include<standard.h>
#include<engine.h>
#include<touhoucards.h>

class Moutama : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Moutama(Card::Suit suit, int number);
    virtual void resolveDefense(CombatStruct &combat) const;
    virtual void resolveAttack(CombatStruct &combat) const;
};

class Weather: public DelayedTrick{
    Q_OBJECT

public:
    Weather(Card::Suit suit, int number);

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class Kaisei : public Weather
{
    Q_OBJECT
public:
    Q_INVOKABLE Kaisei(Card::Suit suit, int number);

    virtual void takeEffect(ServerPlayer *target) const;
};

class ExpansionOnePackage : public Package
{
    Q_OBJECT
public:
    ExpansionOnePackage();
};

#endif // EXPANSION_ONE_H

#ifndef TOUHOUCARDS_H
#define TOUHOUCARDS_H

#include<standard.h>

class CombatCard : public BasicCard
{
    Q_OBJECT
public:
    Q_INVOKABLE CombatCard(Card::Suit suit, int number);

    virtual QString getSubtype() const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool isAvailable(const Player *player) const;
    //virtual void onHit(const CardEffectStruct &effect) const;
    virtual QString getLogName() const;
    virtual bool canbeBlocked(const Card* card) const;
};

class Barrage : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Barrage(Card::Suit suit,int number);
};

class Strike : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Strike(Card::Suit suit,int number);
};

class Rune : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Rune(Card::Suit suit,int number);
};

class TouhouPackage: public Package{
    Q_OBJECT

public:
    TouhouPackage();
};

#endif // TOUHOUCARDS_H

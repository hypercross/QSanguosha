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

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;

    virtual bool canbeBlocked(const Card* card) const;
    virtual void resolveDefense(CombatStruct &combat) const;
    virtual void resolveAttack(CombatStruct &combat) const;
};

class Barrage : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Barrage(Card::Suit suit,int number);
    virtual void resolveDefense(CombatStruct &combat) const;
    virtual void resolveAttack(CombatStruct &combat) const;
};

class Strike : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Strike(Card::Suit suit,int number);
    virtual void resolveDefense(CombatStruct &combat) const;
    virtual void resolveAttack(CombatStruct &combat) const;
};

class Rune : public CombatCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Rune(Card::Suit suit,int number);
    virtual void resolveDefense(CombatStruct &combat) const;
    virtual void resolveAttack(CombatStruct &combat) const;
};

class Moutama : public BasicCard
{
    Q_OBJECT
public:
    Q_INVOKABLE Moutama(Card::Suit suit, int number);
    virtual QString getSubtype() const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class ExSpell : public AOE
{
    Q_OBJECT
public:
    Q_INVOKABLE ExSpell(Card::Suit suit,int number);
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FullscreanBarrage : public AOE
{
    Q_OBJECT
public:
    Q_INVOKABLE FullscreanBarrage(Card::Suit suit,int number);
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Dannatu : public SingleTargetTrick
{
    Q_OBJECT
public:
    Q_INVOKABLE Dannatu(Card::Suit suit,int number);
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Surprise : public SingleTargetTrick
{
    Q_OBJECT
public:
    Q_INVOKABLE Surprise(Card::Suit suit, int number);
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TouhouPackage: public Package{
    Q_OBJECT

public:
    TouhouPackage();
};

#endif // TOUHOUCARDS_H

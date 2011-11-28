#ifndef TOUHOUCARDS_H
#define TOUHOUCARDS_H

#include<standard.h>
#include<engine.h>
#include<touhou-generals.h>

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
    //virtual void onHit(const CardEffectStruct &effect) const;

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;

    virtual int battle(const Card* card) const;
    virtual void resolveDefense(CombatStruct &combat) const;
    virtual void resolveAttack(CombatStruct &combat) const;
    virtual bool isAvailable(const Player *player) const;

    static bool IsAvailable(const Player *player);

    static int BattleJudge(const Card* atker,const Card* dfser);
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

class NiceGuyCard : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE NiceGuyCard(Card::Suit suit, int number);

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool isAvailable(const Player *player) const;
    virtual void onMove(const CardMoveStruct &move) const;
};

class Yukkuri : public OffensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Yukkuri(Card::Suit suit, int number);

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class YukkuriCard: public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE YukkuriCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class Pants : public OffensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Pants(Card::Suit suit, int number);

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class Broomstick : public DefensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Broomstick(Card::Suit suit, int number);

    virtual void onUninstall(ServerPlayer *player) const;
};

class ZunHat : public DefensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE ZunHat(Card::Suit suit, int number);
};

class Mushroom : public OffensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Mushroom(Card::Suit suit, int number);
};

class TeaCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE TeaCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class Tea : public DefensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Tea(Card::Suit suit,int number);

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class SaisenCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE SaisenCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class Saisen : public OffensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Saisen(Card::Suit suit,int number);

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class Sinbag : public OffensiveHorse
{
    Q_OBJECT
public:
    Q_INVOKABLE Sinbag(Card::Suit suit,int number);
};

class TouhouPackage: public Package{
    Q_OBJECT

public:
    TouhouPackage();

    void addGenerals();
    void addEquips();
};

#endif // TOUHOUCARDS_H

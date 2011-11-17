#ifndef TOUHOUGENERALS_H
#define TOUHOUGENERALS_H

#include "standard.h"
#include "touhoucards.h"
#include "standard-skillcards.h"


class SwitchModeCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE SwitchModeCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};


class GuifuCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE GuifuCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;

    static void ApplyChain(const QString & objectname,ServerPlayer * sp,ServerPlayer *source);
};

class PerfectFreezeCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE PerfectFreezeCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class PhoenixSoarCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE PhoenixSoarCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class FuujinSaishiCard: public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE FuujinSaishiCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class MosesMiracleCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE MosesMiracleCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class DeathlureCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE DeathlureCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DaremoinaiCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE DaremoinaiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class PhilosopherStoneCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE PhilosopherStoneCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class WorldJarCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE WorldJarCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class RealSumiSakuraCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE RealSumiSakuraCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FiveProblemCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE FiveProblemCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class MisfortuneCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE MisfortuneCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class OnbashiraCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE OnbashiraCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IronWheelCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE IronWheelCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class NinetailCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NinetailCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class MindreaderCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE MindreaderCard();


    virtual void onEffect(const CardEffectStruct &effect) const;
};

class UltimateBuddhistCard : public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE UltimateBuddhistCard();


    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // TOUHOUGENERALS_H

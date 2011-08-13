#ifndef TOUHOUGENERALS_H
#define TOUHOUGENERALS_H

#include "standard.h"
#include "touhoucards.h"
#include "standard-skillcards.h"

class GuifuCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE GuifuCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class FreezeCard : public SkillCard
{
    Q_OBJECT
public:
    Q_INVOKABLE FreezeCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
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

#endif // TOUHOUGENERALS_H

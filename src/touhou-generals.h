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

#endif // TOUHOUGENERALS_H

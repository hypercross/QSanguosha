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
};


#endif // TOUHOUGENERALS_H

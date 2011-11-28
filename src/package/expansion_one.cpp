#include "expansion_one.h"

Moutama::Moutama(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("moutama");
}

void Moutama::resolveAttack(CombatStruct &combat) const
{
    Room * room = combat.from->getRoom();
    room->changeMp(combat.from,1);
}

void Moutama::resolveDefense(CombatStruct &combat) const
{
    Room * room = combat.from->getRoom();
    room->changeMp(combat.to,1);
}

Weather::Weather(Suit suit, int number):DelayedTrick(suit, number, true){
}

void Weather::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    source->drawCards(1);
    foreach(ServerPlayer *sp,room->getAllPlayers())
    {
        foreach(const Card * card, sp->getJudgingArea())
        {
            if(card->inherits("Weather"))
            {
                room->throwCard(card);
                DelayedTrick::use(room,source,targets);
                return;
            }
        }
    }

    DelayedTrick::use(room,source,targets);
}

bool Weather::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty();
}

Kaisei::Kaisei(Card::Suit suit, int number):Weather(suit,number)
{
    setObjectName("kaisei");

    judge.pattern = QRegExp("(.*):(diamond|heart):(.*)");
    judge.good = false;
    judge.reason = objectName();
}

void Kaisei::takeEffect(ServerPlayer *target) const
{
    target->getRoom()->changeMp(target,1);
    onNullified(target);
}


ExpansionOnePackage::ExpansionOnePackage()
    :Package("exp_one")
{
    QList<Card *> cards;

    cards << new Moutama(Card::Spade, 4);
    cards << new Moutama(Card::Club, 5);
    cards << new Moutama(Card::Heart, 6);
    cards << new Moutama(Card::Diamond, 7);

    cards << new Kaisei(Card::Diamond,1);

    foreach(Card *card, cards)
        card->setParent(this);
}

ADD_PACKAGE(ExpansionOne)

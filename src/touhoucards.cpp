#include "touhoucards.h"
#include "engine.h"


QString CombatCard::getLogName() const
{
    return "combat";
}


CombatCard::CombatCard(Card::Suit suit, int number):BasicCard(suit,number)
{
    will_throw = false;
}

QString CombatCard::getSubtype() const
{
    return "combat";
}

void CombatCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    foreach(ServerPlayer* player,targets)player->tag["combatEffective"]=false;

    BasicCard::use(room,source,targets);

    CardStar cs=this;
    QVariant data = QVariant::fromValue(cs);
    source->tag["chosenAttack"] = data;
    bool broken=room->getThread()->trigger(CombatReveal,source,data);
    const Card* attackCard;

    if(!broken){
        QVariant data = source->tag["chosenAttack"];
        const Card* card=data.value<CardStar>();
        attackCard=card;
        room->throwCard(card);
        room->getThread()->trigger(CombatRevealed,source,data);
    }else return;



    foreach(ServerPlayer* player,targets)
    {
        if(!player->tag["combatEffective"].toBool())continue;
        QVariant data = player->tag["chosenBlock"];
        broken=room->getThread()->trigger(CombatReveal,player,data);
        if(!broken){

            data = player->tag["chosenBlock"];
            CardStar card = data.value<CardStar>();
            room->throwCard(card);
            room->getThread()->trigger(CombatRevealed,player,data);

            CombatStruct combat;
            combat.from   = source;
            combat.to     = player;
            combat.combat = qobject_cast<const CombatCard*>(attackCard);
            combat.block  = card;

            data=QVariant::fromValue(combat);

            broken=room->getThread()->trigger(CombatFinish,player,data);
            if(!broken)
            {
                DamageStruct dmg;
                dmg.damage=1;
                dmg.from=source;
                dmg.to = player;

                if(!combat.combat->canbeBlocked(card))room->damage(dmg);

                room->getThread()->trigger(CombatFinished,source,data);
            }
        }
    }

}

void CombatCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    CombatStruct combat;
    combat.from   = effect.from;
    combat.to     = effect.to;
    combat.combat = this;

    room->combatEffect(combat);
}

bool CombatCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return !targets.isEmpty();
}

bool CombatCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int slash_targets = 1;
    if(Self->hasWeapon("halberd") && Self->isLastHandCard(this)){
        slash_targets = 3;
    }

    bool distance_limit = true;

    if(Self->hasFlag("tianyi_success")){
        distance_limit = false;
        slash_targets ++;
    }

    if(Self->hasSkill("shenji") && Self->getWeapon() == NULL)
        slash_targets = 3;

    if(targets.length() >= slash_targets)
        return false;

    if(inherits("WushenSlash")){
        distance_limit = false;
    }

    return Self->canSlash(to_select, distance_limit);
}

bool CombatCard::canbeBlocked(const Card *card) const
{
    bool a = false;
    if(!card)return false;
    a=a || (this->inherits("Barrage") && card->inherits("Rune"));
    a=a || (this->inherits("Rune") && card->inherits("Strike"));
    a=a || (this->inherits("Strike") && card->inherits("Barrage"));
    return a;
}

bool CombatCard::isAvailable(const Player *player) const{
    return Slash::IsAvailable(player);
}

Barrage::Barrage(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("barrage");
}

Strike::Strike(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("strike");
}

Rune::Rune(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("rune");
}

TouhouPackage::TouhouPackage()
    :Package("touhou")
{
    QList<Card *> cards;
    cards<< new Barrage(Card::Spade,1)
         << new Barrage(Card::Spade,2)
         << new Barrage(Card::Spade,3)
         << new Barrage(Card::Spade,4)
         << new Barrage(Card::Spade,5)
         << new Barrage(Card::Spade,6)
         << new Barrage(Card::Spade,7)
         << new Barrage(Card::Spade,8)
         << new Barrage(Card::Spade,9)
         << new Barrage(Card::Spade,10)
         << new Barrage(Card::Spade,11);

    cards<< new Strike(Card::Club,1)
         << new Strike(Card::Club,2)
         << new Strike(Card::Club,3)
         << new Strike(Card::Club,4)
         << new Strike(Card::Club,5)
         << new Strike(Card::Club,6);

    cards<< new Rune(Card::Heart,1)
         << new Rune(Card::Heart,2)
         << new Rune(Card::Heart,3)
         << new Rune(Card::Heart,4)
         << new Rune(Card::Heart,5)
         << new Rune(Card::Heart,6)
         << new Rune(Card::Heart,7)
         << new Rune(Card::Heart,8);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(Touhou)

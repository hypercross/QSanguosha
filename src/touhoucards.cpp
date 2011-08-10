#include "touhoucards.h"
#include "engine.h"

void CombatCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    LogMessage log;
    log.from = player;
    log.to = card_use.to;
    log.type = "#AnnounceAttack";
    room->sendLog(log);

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();
    thread->trigger(CardUsed, player, data);

    thread->trigger(CardFinished, player, data);
}

CombatCard::CombatCard(Card::Suit suit, int number):BasicCard(suit,number)
{
    will_throw = false;
}

void CombatCard::resolveAttack(CombatStruct &combat) const
{
}

void CombatCard::resolveDefense(CombatStruct &combat) const
{
}

QString CombatCard::getSubtype() const
{
    return "combat";
}

void CombatCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    //CardStar cs=this;
    //QVariant data = QVariant::fromValue(cs);
    //source->tag["chosenAttack"] = data;

    int farthest = 1;
    foreach(ServerPlayer *player,targets)
        if(source->distanceTo(player)>farthest)
            farthest = source->distanceTo(player);
    if(farthest > source->getAttackRange())
        room->changeMp(source,source->getAttackRange() - farthest);

    source->addToPile("Attack",this->getEffectiveId(),false);

    BasicCard::use(room,source,targets);
    room->getThread()->delay();

    bool broken=room->getThread()->trigger(CombatReveal,source);
    const Card* attackCard;

    if(!broken){
//        QVariant data = source->tag["chosenAttack"];
//        const Card* card=data.value<CardStar>();
        attackCard=Sanguosha->getCard(source->getPile("Attack").first());

        LogMessage log;
        log.type = "$revealResult";
        log.from = source;
        log.card_str = attackCard->getEffectIdString();
        room->sendLog(log);

        room->throwCard(attackCard);

        room->getThread()->trigger(CombatRevealed,source);
        room->getThread()->delay();
    }
    if(!attackCard)return;

        foreach(ServerPlayer* player,targets)
        {
            if(!player->tag["combatEffective"].toBool())continue;
//            QVariant data = player->tag["chosenBlock"];
            broken=room->getThread()->trigger(CombatReveal,player);
            if(!broken){

                //data = player->tag["chosenBlock"];
                //CardStar card = data.value<CardStar>();
                QList<int> pile=player->getPile("Defense");

                CardStar card ;

                if(pile.length()<1)
                {
                    CombatStruct combat;
                    combat.from   = source;
                    combat.to     = player;
                    combat.combat = qobject_cast<const CombatCard*>(attackCard);
                    combat.block  = card;

                    combat.combat->resolveAttack(combat);
                }else{
                    card = Sanguosha->getCard(pile.first());
                    LogMessage log;
                    log.type = "$revealResult";
                    log.from = player;
                    log.card_str = card->getEffectIdString();
                    room->sendLog(log);

                    room->throwCard(card);
                    room->getThread()->trigger(CombatRevealed,player);
                    room->getThread()->delay();

                    CombatStruct combat;
                    combat.from   = source;
                    combat.to     = player;
                    combat.combat = qobject_cast<const CombatCard*>(attackCard);
                    combat.block  = card;

                    QVariant data=QVariant::fromValue(combat);

                    broken=room->getThread()->trigger(CombatFinish,player,data);

                    if(!broken)
                    {
                        const CombatCard * ccard=qobject_cast<const CombatCard*>(card);
                        if((!card->inherits("CombatCard")) || ccard->canbeBlocked(combat.combat))
                        {
                            combat.combat->resolveAttack(combat);
                        }
                        if(combat.combat->canbeBlocked(card))
                        {
                            ccard->resolveDefense(combat);
                        }
                        room->getThread()->trigger(CombatFinished,source,data);
                    }
                }
            }


        }

    foreach(ServerPlayer* player,targets)
    {
        player->tag["combatEffective"]=QVariant();
//        player->tag["chosenBlock"] =QVariant();
    }
//        source->tag["chosenAttack"]=QVariant();
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
//    if(Self->hasWeapon("halberd") && Self->isLastHandCard(this)){
//        slash_targets = 3;
//    }

    bool distance_limit = true;

//    if(Self->hasFlag("tianyi_success")){
//        distance_limit = false;
//        slash_targets ++;
//    }

//    if(Self->hasSkill("shenji") && Self->getWeapon() == NULL)
//        slash_targets = 3;

    if(targets.length() >= slash_targets)
        return false;

//    if(inherits("WushenSlash")){
//        distance_limit = false;
//    }

    return Self->canCombat(to_select, distance_limit);
}

bool CombatCard::canbeBlocked(const Card *card) const
{
    bool a = false;
    if(!card)return false;
    if(card->objectName()==this->objectName())return this->getNumber()<=card->getNumber();
    a=a || (this->inherits("Barrage") && card->inherits("Strike"));
    a=a || (this->inherits("Rune") && card->inherits("Barrage"));
    a=a || (this->inherits("Strike") && card->inherits("Rune"));
    return a;
}

bool CombatCard::isAvailable(const Player *player) const{
    return Slash::IsAvailable(player);
}

Barrage::Barrage(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("barrage");
}

void Barrage::resolveDefense(CombatStruct &combat) const
{
}

void Barrage::resolveAttack(CombatStruct &combat) const
{
    DamageStruct dmg;
    dmg.damage = 1;
    dmg.from   = combat.from;
    dmg.to     = combat.to;
    dmg.card   = this;

    dmg.to->getRoom()->damage(dmg);
}

Strike::Strike(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("strike");
}

void Strike::resolveAttack(CombatStruct &combat) const
{
    combat.to->getRoom()->changeMp(combat.to,-1);
    if(combat.from->getRoom()->obtainable(this,combat.from))
    {
        combat.from->getRoom()->obtainCard(combat.from,this->getEffectiveId());
        combat.from->jilei(QString(this->getEffectiveId()));
        combat.from->invoke("jilei",this->getEffectIdString());
        combat.to->setFlags("jilei");
    }
}

void Strike::resolveDefense(CombatStruct &combat) const
{
    if(combat.to->getRoom()->obtainable(this,combat.to))
    {
        combat.to->getRoom()->obtainCard(combat.to,this->getEffectiveId());
        combat.to->jilei(QString(this->getEffectiveId()));
        combat.to->invoke("jilei",this->getEffectIdString());
        combat.to->setFlags("jilei");
    }
}

Rune::Rune(Card::Suit suit, int number):CombatCard(suit,number)
{
    setObjectName("rune");
}

void Rune::resolveAttack(CombatStruct &combat) const
{
    DamageStruct dmg;
    dmg.damage = 1;
    dmg.from   = combat.from;
    dmg.to     = combat.to;
    dmg.card   = this;

    dmg.to->getRoom()->damage(dmg);
    combat.to->getRoom()->changeMp(combat.to,-1);
}

void Rune::resolveDefense(CombatStruct &combat) const
{
    DamageStruct dmg;
    dmg.damage = 1;
    dmg.from   = combat.to;
    dmg.to     = combat.from;
    dmg.card   = this;

    dmg.to->getRoom()->damage(dmg);
    combat.to->getRoom()->changeMp(combat.from,-1);
}

Moutama::Moutama(Card::Suit suit, int number):BasicCard(suit,number)
{
    setObjectName("moutama");
    target_fixed = true;
}

QString Moutama::getSubtype() const
{
    return "recover_card";
}

void Moutama::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    room->throwCard(this->getEffectiveId());
    room->changeMp(source,1);
    room->drawCards(source,1);
}

ExSpell::ExSpell(Card::Suit suit, int number)
    :AOE(suit,number)
{
    setObjectName("ex_spell");
}

void ExSpell::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    if(source->hasFlag("dying"))
    {
        RecoverStruct recover;
        recover.card = this;
        recover.who = source;
        recover.recover=1-source->getHp();
        room->recover(source, recover);
    }
    if(source->getHp() > 0)AOE::use(room,source,targets);
}

void ExSpell::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *slash = room->askForCard(effect.to, "barrage", "ex-spell-barrage:" + effect.from->objectName());
    if(slash == NULL){
        DamageStruct damage;
        damage.card = this;
        damage.damage = 1;
        damage.to = effect.to;

        damage.from = effect.from;
        room->damage(damage);
    }
}

FullscreanBarrage::FullscreanBarrage(Card::Suit suit, int number)
    :AOE(suit,number)
{
    setObjectName("fullscrean_barrage");
}

void FullscreanBarrage::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    if(source->hasFlag("dying"))
    {
        RecoverStruct recover;
        recover.card = this;
        recover.who = source;
        recover.recover=1-source->getHp();
        room->recover(source, recover);
    }
    if(source->getHp() > 0)AOE::use(room,source,targets);
}

void FullscreanBarrage::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *slash = room->askForCard(effect.to, "strike", "full-scre-strike:" + effect.from->objectName());
    if(slash == NULL){
        DamageStruct damage;
        damage.card = this;
        damage.damage = 1;
        damage.to = effect.to;

        damage.from = effect.from;
        room->damage(damage);
    }
}

Dannatu::Dannatu(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, true)
{
    setObjectName("dannatu");
}

bool Dannatu::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
//    if(to_select->hasSkill("kongcheng") && to_select->isKongcheng())
//        return false;

    if(to_select == Self)
        return false;

    return targets.isEmpty();
}


void Dannatu::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *first = effect.to;
    ServerPlayer *second = effect.from;
    Room *room = first->getRoom();

    room->setEmotion(first, "duel-a");
    room->setEmotion(second, "duel-b");

    forever{
        const Card *slash = room->askForCard(first, "barrage", "dannatu-barrage:" + second->objectName());
        if(slash == NULL)break;
        qSwap(first, second);
    }

    DamageStruct damage;
    damage.card = this;
    damage.from = second;
    damage.to = first;

    room->damage(damage);
}

Surprise::Surprise(Card::Suit suit, int number)
    :SingleTargetTrick(suit,number,true)
{
    setObjectName("surprise");
}

bool Surprise::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (to_select != Self) && targets.isEmpty();
}

void Surprise::onEffect(const CardEffectStruct &effect) const
{
    Room* room=effect.to->getRoom();
    QList<int> hand=effect.to->handCards();
    room->fillAG(hand,effect.from);
    int cid=room->askForAG(effect.from,hand,true,"surprise");
    if(cid>=0)
    {
        QString js=QString("%1").arg(cid);
        effect.to->jilei(js);
        effect.to->invoke("jilei",js);
        effect.to->setFlags("jilei");

        room->showCard(effect.to,cid);
    }
    effect.from->invoke("clearAG");
    room->getThread()->delay();
}

NiceGuyCard::NiceGuyCard(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("nice_guy_card");
}

void NiceGuyCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    room->throwCard(this);
}

bool NiceGuyCard::isAvailable(const Player *player) const
{
    return false;
}

void NiceGuyCard::onMove(const CardMoveStruct &move) const
{
    ServerPlayer *from = move.from;
    if(from && move.to_place == Player::DiscardedPile)
    {
        from->getRoom()->drawCards(from,1);
    }
}

TouhouPackage::TouhouPackage()
    :Package("touhou")
{
    QList<Card *> cards;
    int i=0;
    for(;i<104;i++)
        if(i<30)
            cards<<new Barrage((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<42)
            cards<<new Strike((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<50)
            cards<<new Rune((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<58)
            cards<<new Peach((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<61)
            cards<<new Indulgence((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<67)
            cards<<new Dismantlement((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<72)
            cards<<new Snatch((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<75)
            cards<<new ExNihilo((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<78)
            cards<<new Dannatu((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<81)
            cards<<new ExSpell((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<82)
            cards<<new FullscreanBarrage((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<85)
            cards<<new Nullification((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<87)
            cards<<new AmazingGrace((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<88)
            cards<<new GodSalvation((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<89)
            cards<<new Surprise((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<93)
            cards<<new NiceGuyCard((Card::Suit)(i/26),(i%26)/2+1);


//    cards<< new Barrage(Card::Spade,1)
//         << new Barrage(Card::Spade,2)
//         << new Barrage(Card::Spade,3)
//         << new Barrage(Card::Spade,4)
//         << new Barrage(Card::Spade,5)
//         << new Barrage(Card::Spade,6)
//         << new Barrage(Card::Spade,7)
//         << new Barrage(Card::Spade,8)
//         << new Barrage(Card::Spade,9)
//         << new Barrage(Card::Spade,10)
//         << new Barrage(Card::Spade,11);

//    cards<< new Strike(Card::Club,1)
//         << new Strike(Card::Club,2)
//         << new Strike(Card::Club,3)
//         << new Strike(Card::Club,4)
//         << new Strike(Card::Club,5)
//         << new Strike(Card::Club,6);

//    cards<< new Rune(Card::Heart,1)
//         << new Rune(Card::Heart,2)
//         << new Rune(Card::Heart,3)
//         << new Rune(Card::Heart,4)
//         << new Rune(Card::Heart,5)
//         << new Rune(Card::Heart,6)
//         << new Rune(Card::Heart,7)
//         << new Rune(Card::Heart,8);

//    cards<< new Moutama(Card::Diamond,1)
//         << new Moutama(Card::Diamond,2)
//         << new Moutama(Card::Diamond,3)
//         << new Moutama(Card::Diamond,4)
//         << new Moutama(Card::Diamond,5)
//         << new Moutama(Card::Diamond,6)
//         << new Moutama(Card::Diamond,7)
//         << new Moutama(Card::Diamond,8);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(Touhou)

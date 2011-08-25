#include "standard.h"
#include "serverplayer.h"
#include "room.h"
#include "skill.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "engine.h"
#include "client.h"
#include "touhoucards.h"

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
    //mp consumption
    int farthest = 1;
    foreach(ServerPlayer *player,targets)
        if(source->distanceTo(player)>farthest)
            farthest = source->distanceTo(player);
    if(farthest > source->getAttackRange()
            && !source->hasFlag("tianyi_success")
            && !source->hasSkill("sharpmind"))
    {
        if(source->getMp()< (farthest - source->getAttackRange()) )return;
        room->changeMp(source,source->getAttackRange() - farthest);
    }


    //set attacker & ask for blocker
    CardStar declare = this;
    QVariant data = QVariant::fromValue(declare);
    bool broken = room->getThread()->trigger(AttackDeclare,source,data);
    declare = data.value<CardStar>();
    if(broken || !declare->inherits("CombatCard"))return;

    source->addToPile("Attack",declare->getEffectiveId(),false);
    if(declare->getSkillName().length()>0)
    {
        source->tag["Combat_Convert_From"] = declare->getEffectiveId()+1;
        source->tag["Combat_Convert_To"] = declare->toString();
    }

    room->getThread()->trigger(AttackDeclared,source,data);

    BasicCard::use(room,source,targets);
    room->getThread()->delay(500);


    // reveal attacker
    CombatRevealStruct reveal;

    QList<int> atkpile = source->getPile("Attack");

    if(atkpile.length()<2)
        reveal.revealed = Sanguosha->getCard(atkpile.first());
    else{
        room->fillAG(atkpile,source);
        int cid = room->askForAG(source,atkpile,false,"combat-reveal");
        source->invoke("clearAG");
        reveal.revealed = Sanguosha->getCard(cid);
    }

    if(reveal.revealed->getEffectiveId() == source->tag["Combat_Convert_From"].toInt()-1)
        reveal.revealed = Card::Parse(source->tag["Combat_Convert_To"].toString());

    source->tag["Combat_Convert_From"] = 0;
    source->tag["Combat_Convert_To"]   = QVariant();


    reveal.who      = source ;
    reveal.attacker = true ;
    foreach(ServerPlayer* player,room->getAlivePlayers())
        reveal.opponets << player;

    data = QVariant::fromValue(reveal);

    broken=room->getThread()->trigger(CombatReveal,source,data);
    reveal = data.value<CombatRevealStruct>();
    if(broken)return;


    const Card * attackCard = reveal.revealed;

    LogMessage log;
    log.type = QString("%1%2").arg(attackCard->isVirtualCard() ? "#" : "$").arg("revealResult");
    log.from = source;
    log.card_str = attackCard->toString();
    room->sendLog(log);

    room->throwCard(attackCard);

    broken = room->getThread()->trigger(CombatRevealed,source,data);
    if(broken)return;
    room->getThread()->delay();

    //if(!reveal.revealed->inherits("CombatCard"))return;

    // reveal each blocker and finish combat
        foreach(ServerPlayer* player,room->getAlivePlayers())
        {


            //skip invalid targets
            if(!player->tag["combatEffective"].toBool())continue;
            player->tag["combatEffective"]=QVariant();


            //reveal blocker
            QList<int> pile=player->getPile("Defense");
            CardStar blocker = new DummyCard;

            if(pile.length()>0)
            {
                CombatRevealStruct block_reveal;

                if(pile.length()<2)
                    block_reveal.revealed = Sanguosha->getCard(pile.first());
                else{
                    room->fillAG(pile,player);
                    int cid = room->askForAG(player,pile,false,"combat-reveal");
                    player->invoke("clearAG");
                    block_reveal.revealed = Sanguosha->getCard(cid);
                }

                if(block_reveal.revealed->getEffectiveId() == player->tag["Combat_Convert_From"].toInt()-1)
                    block_reveal.revealed = Card::Parse(player->tag["Combat_Convert_To"].toString());


                player->tag["Combat_Convert_From"] = 0;
                player->tag["Combat_Convert_To"]   = QVariant();


                block_reveal.who      = player;
                block_reveal.opponets << source;

                data = QVariant::fromValue(block_reveal);
                broken = room->getThread()->trigger(CombatReveal,player,data);
                block_reveal = data.value<CombatRevealStruct>();
                if(broken)continue;

                blocker = block_reveal.revealed;

                LogMessage log;
                log.type = QString("%1%2").arg(blocker->isVirtualCard() ? "#" : "$").arg("revealResult");
                log.from = player;
                log.card_str = blocker->toString();
                room->sendLog(log);

                room->throwCard(blocker);

                broken = room->getThread()->trigger(CombatRevealed,player,data);
                if(broken)continue;

                room->getThread()->delay();
            }


            //finish combat

            CombatStruct combat;
            combat.from   = source;
            combat.to     = player;
            combat.combat = attackCard;
            combat.block  = blocker;

            data=QVariant::fromValue(combat);

            broken=room->getThread()->trigger(CombatFinish,source,data);
            broken = broken || room->getThread()->trigger(TargetFinish,player,data);
            if(broken)
            {
                if(combat.block->inherits("DummyCard"))delete combat.block;
                continue;
            }
            combat = data.value<CombatStruct>();

            //const CombatCard * ccard=qobject_cast<const CombatCard*>(combat.block);
            attackCard = combat.combat;
            blocker    = combat.block;

            if(blocker->canbeBlocked(combat.combat) && combat.combat->inherits("CombatCard"))
            {
                const CombatCard * ccard=qobject_cast<const CombatCard*>(combat.combat);
                ccard->resolveAttack(combat);
            }

            if(attackCard->canbeBlocked(blocker) && blocker->inherits("CombatCard"))
            {
                const CombatCard * ccard=qobject_cast<const CombatCard*>(blocker);
                ccard->resolveDefense(combat);
            }

            room->getThread()->trigger(CombatFinished,source,data);
            room->getThread()->trigger(TargetFinished,player,data);

            if(combat.block->inherits("DummyCard"))delete combat.block;
            if(combat.combat->inherits("DummyCard"))delete combat.combat;

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
    if(Self->hasWeapon("tengu_fan"))slash_targets ++;
//    if(Self->hasWeapon("halberd") && Self->isLastHandCard(this)){
//        slash_targets = 3;
//    }

    bool distance_limit = true;

    if(Self->hasFlag("tianyi_success")){
        distance_limit = false;
        slash_targets ++;
    }

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
    return Slash::IsAvailable(player) || player->hasWeapon("hakkero");
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
        combat.from->setFlags("jilei_temp");
    }
}

void Strike::resolveDefense(CombatStruct &combat) const
{
    if(combat.to->getRoom()->obtainable(this,combat.to))
    {
        combat.to->getRoom()->obtainCard(combat.to,this->getEffectiveId());
        combat.to->jilei(QString(this->getEffectiveId()));
        combat.to->invoke("jilei",this->getEffectIdString());
        combat.to->setFlags("jilei_temp");
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
    const Card *slash = room->askForCard(effect.to, "strike+rune", "full-scre-strike:" + effect.from->objectName());
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
    return (to_select != Self) && targets.isEmpty() && (to_select->getHandcardNum()>0);
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
        if(from->isDead())return;
        from->getRoom()->drawCards(from,1);
    }
}

YukkuriCard::YukkuriCard()
{
}

void YukkuriCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    room->throwCard(source->getOffensiveHorse());

    Dannatu *dannatu = new Dannatu(Card::NoSuit, 0);
    dannatu->setCancelable(false);

    CardEffectStruct effect;
    effect.card = dannatu;
    effect.from = source;
    effect.to   = targets.first();

    room->cardEffect(effect);
}

class YukkuriSkill : public ZeroCardViewAsSkill
{
public:
    YukkuriSkill():ZeroCardViewAsSkill("yukkuri")
    {
    }

    virtual const Card* viewAs() const{
        return new YukkuriCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("Yukkuri");
    }
};

Yukkuri::Yukkuri(Card::Suit suit, int number)
    :OffensiveHorse(suit,number)
{
    setObjectName("yukkuri");
}

void Yukkuri::onInstall(ServerPlayer *player) const
{
    player->getRoom()->attachSkillToPlayer(player,objectName());
}

void Yukkuri::onUninstall(ServerPlayer *player) const
{
    player->getRoom()->detachSkillFromPlayer(player,objectName());
}

Pants::Pants(Card::Suit suit, int number)
    :OffensiveHorse(suit,number)
{
    setObjectName("pants");
}

void Pants::onInstall(ServerPlayer *player) const
{
    Room * room = player->getRoom();
    room->setPlayerProperty(player,"maxmp",player->getMaxMP()+1);
    room->changeMp(player,1);
}

void Pants::onUninstall(ServerPlayer *player) const
{
    Room * room = player->getRoom();
    room->setPlayerProperty(player,"maxmp",player->getMaxMP()-1);
}

Broomstick::Broomstick(Card::Suit suit, int number)
    :DefensiveHorse(suit,number, +2)
{
    setObjectName("broomstick");
}

class ZunHatSkill : public TriggerSkill
{
public:
    ZunHatSkill():TriggerSkill("zunhat")
    {
        events << MpChanged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target->getDefensiveHorse() && target->getDefensiveHorse()->objectName() == objectName();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        MpChangeStruct change = data.value<MpChangeStruct>();
        if(change.delta<=0)return false;

        Room *room = player->getRoom();
        //if(!room->askForSkillInvoke(player,objectName()))return false;

        change.delta++;
        data = QVariant::fromValue(change);

        LogMessage log;
        log.type = "#Zunhat" ;
        log.from = player ;
        log.arg  = QString("%1").arg(change.delta-1);
        log.arg2 = QString("%1").arg(change.delta);
        room->sendLog(log);

        return false;
    }
};

ZunHat::ZunHat(Card::Suit suit, int number)
    :DefensiveHorse(suit,number)
{
    setObjectName("zunhat");

    skill = new ZunHatSkill;
}

class MushroomSkill : public MasochismSkill
{
public:
    MushroomSkill():MasochismSkill("mushroom")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target->getOffensiveHorse() &&
                target->getOffensiveHorse()->objectName() == objectName();
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room * room = target->getRoom();
        if(!room->askForSkillInvoke(target,objectName()))return;
        if(!damage.from)return;
        if(damage.from->isDead())return;

        room->throwCard(target->getOffensiveHorse());

        if(room->askForDiscard(damage.from,objectName(),2,true,true))return;
        DamageStruct returnDamage;
        returnDamage.from = target ;
        returnDamage.to   = damage.from ;

        room->damage(returnDamage);
    }
};

Mushroom::Mushroom(Card::Suit suit, int number)
    :OffensiveHorse(suit,number)
{
    setObjectName("mushroom");

    skill = new MushroomSkill;
}

TeaCard::TeaCard()
{
    target_fixed = true;
}

void TeaCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    room->throwCard(source->getDefensiveHorse());
    room->changeMp(source,1);
    room->drawCards(source,1);
}

class TeaSkill : public ZeroCardViewAsSkill
{
public:
    TeaSkill():ZeroCardViewAsSkill("tea")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("Tea");
    }

    virtual const Card* viewAs() const
    {
        return new TeaCard;
    }

};

Tea::Tea(Card::Suit suit, int number)
    :DefensiveHorse(suit,number)
{
    setObjectName("tea");
}

void Tea::onInstall(ServerPlayer *player) const
{
    player->getRoom()->attachSkillToPlayer(player,objectName());
}

void Tea::onUninstall(ServerPlayer *player) const
{
    player->getRoom()->detachSkillFromPlayer(player,objectName());
}

SaisenCard::SaisenCard()
{

}

bool SaisenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool valid = true;
    valid = valid && targets.isEmpty();
    valid = valid && to_select != Self;
    valid = valid && to_select->getHandcardNum() > Self->getHandcardNum();
    return valid;
}

void SaisenCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    room->throwCard(source->getOffensiveHorse());
    int cid = targets.first()->getRandomHandCardId();
    if(cid<0)return;
    room->moveCardTo(Sanguosha->getCard(cid),source,Player::Hand,false);
}

class SaisenSkill : public ZeroCardViewAsSkill
{
public:
    SaisenSkill():ZeroCardViewAsSkill("saisen")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("Saisen");
    }

    virtual const Card* viewAs() const
    {
        return new SaisenCard;
    }

};

Saisen::Saisen(Card::Suit suit, int number)
    :OffensiveHorse(suit,number)
{
    setObjectName("saisen");
}

void Saisen::onInstall(ServerPlayer *player) const
{
    player->getRoom()->attachSkillToPlayer(player,objectName());
}

void Saisen::onUninstall(ServerPlayer *player) const
{
    player->getRoom()->detachSkillFromPlayer(player,objectName());
}

class SinbagSkill : public TriggerSkill
{
public:
    SinbagSkill():TriggerSkill("sinbag")
    {
        events << CombatFinish << TargetFinish;
    }

    virtual int getPriority() const
    {
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target->getOffensiveHorse() &&
                target->getOffensiveHorse()->objectName() == objectName();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();
        if(event == TargetFinish && !combat.block->inherits("Strike"))return false;
        if(event == CombatFinish && !combat.combat->inherits("Strike"))return false;

        Room * room = player->getRoom();
        if(!room->askForSkillInvoke(player,objectName()))return false;

        room->throwCard(player->getOffensiveHorse());

        LogMessage log;
        log.type = "#Sinbag";
        log.from = player ;
        room->sendLog(log);

        ServerPlayer* target = (event == TargetFinish) ? combat.from : combat.to;
        DamageStruct dmg;
        dmg.from = player;
        dmg.to   = target;

        room->damage(dmg);
        return true;

    }
};

Sinbag::Sinbag(Card::Suit suit, int number)
    :OffensiveHorse(suit,number)
{
    setObjectName("sinbag");

    skill = new SinbagSkill;
}

TouhouPackage::TouhouPackage()
    :Package("touhou")
{
    QList<Card *> cards;

    cards << new Dannatu(Card::Spade,1);

    cards << new Surprise(Card::Spade,3);
    cards << new Snatch(Card::Spade,4);

    cards << new ExSpell(Card::Spade,6);
    cards << new Barrage(Card::Spade,7);
    cards << new Barrage(Card::Spade,8);
    cards << new Barrage(Card::Spade,9);
    cards << new Barrage(Card::Spade,10);
    cards << new Dismantlement(Card::Spade,11);
    cards << new Nullification(Card::Spade,12);
    cards << new ExSpell(Card::Spade,13);


    cards << new Surprise(Card::Spade,3);
    cards << new FullscreanBarrage(Card::Spade,4);
    cards << new Barrage(Card::Spade,5);
    cards << new Indulgence(Card::Spade,6);
    cards << new Strike(Card::Spade,7);
    cards << new Rune(Card::Spade,8);
    cards << new Rune(Card::Spade,9);
    cards << new Rune(Card::Spade,10);
    cards << new SupplyShortage(Card::Spade,11);

    cards << new GodSalvation(Card::Heart,1);
    cards << new Strike(Card::Heart,2);
    cards << new Dismantlement(Card::Heart,3);
    cards << new Peach(Card::Heart,4);
    cards << new Peach(Card::Heart,5);
    cards << new Peach(Card::Heart,6);
    cards << new Peach(Card::Heart,7);
    cards << new Peach(Card::Heart,8);
    cards << new Peach(Card::Heart,9);
    cards << new Barrage(Card::Heart,10);
    cards << new Barrage(Card::Heart,11);
    cards << new Peach(Card::Heart,12);
    cards << new Strike(Card::Heart,13);

    cards << new FullscreanBarrage(Card::Heart,1);

    cards << new AmazingGrace(Card::Heart,4);
    cards << new AmazingGrace(Card::Heart,5);
    cards << new Dismantlement(Card::Heart,6);
    cards << new Dismantlement(Card::Heart,7);
    cards << new ExNihilo(Card::Heart,8);
    cards << new Indulgence(Card::Heart,9);
    cards << new Barrage(Card::Heart,10);
    cards << new ExNihilo(Card::Heart,11);

    cards << new NiceGuyCard(Card::Heart,13);

    cards << new Dannatu(Card::Club,1);
    cards << new Barrage(Card::Club,2);
    cards << new Barrage(Card::Club,3);
    cards << new Barrage(Card::Club,4);
    cards << new Barrage(Card::Club,5);
    cards << new Barrage(Card::Club,6);
    cards << new Barrage(Card::Club,7);
    cards << new Barrage(Card::Club,8);
    cards << new Barrage(Card::Club,9);
    cards << new Barrage(Card::Club,10);
    cards << new Barrage(Card::Club,11);
    cards << new Collateral(Card::Club,12);
    cards << new Nullification(Card::Club,13);


    cards << new Rune(Card::Club,2);
    cards << new Rune(Card::Club,3);
    cards << new Rune(Card::Club,4);
    cards << new SupplyShortage(Card::Club,5);

    cards << new Strike(Card::Club,7);
    cards << new Strike(Card::Club,8);
    cards << new Rune(Card::Club,9);
    cards << new Rune(Card::Club,10);

    cards << new Rune(Card::Club,13);


    cards << new Dismantlement(Card::Diamond,2);
    cards << new Barrage(Card::Diamond,3);
    cards << new Barrage(Card::Diamond,4);
    cards << new Barrage(Card::Diamond,5);
    cards << new Barrage(Card::Diamond,6);
    cards << new Barrage(Card::Diamond,7);
    cards << new Snatch(Card::Diamond,8);
    cards << new Snatch(Card::Diamond,9);
    cards << new ExSpell(Card::Diamond,10);
    cards << new NiceGuyCard(Card::Diamond,11);
    cards << new Nullification(Card::Diamond,12);


    cards << new Surprise(Card::Diamond,1);
    cards << new Strike(Card::Diamond,2);
    cards << new Strike(Card::Diamond,3);
    cards << new Strike(Card::Diamond,4);

    cards << new Indulgence(Card::Diamond,6);
    cards << new Strike(Card::Diamond,7);
    cards << new Strike(Card::Diamond,8);
    cards << new Strike(Card::Diamond,9);
    cards << new Strike(Card::Diamond,10);
    cards << new Surprise(Card::Diamond,11);
    cards << new Peach(Card::Diamond,12);
    cards << new Barrage(Card::Diamond,13);

    //ex
    cards<< new Tea(Card::Club,2);
    cards<< new Nullification(Card::Diamond,12);



    foreach(Card *card, cards)
        card->setParent(this);

    addMetaObject<YukkuriCard>();
    addMetaObject<TeaCard>();
    addMetaObject<SaisenCard>();

    skills << new YukkuriSkill << new TeaSkill << new SaisenSkill;

    addGenerals();
    addEquips();

    //type = CardPack;
}

ADD_PACKAGE(Touhou)

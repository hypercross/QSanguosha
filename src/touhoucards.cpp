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
    if(farthest > source->getAttackRange())
    {
        if(source->getMp()< (farthest - source->getAttackRange()) )return;
        room->changeMp(source,source->getAttackRange() - farthest);
    }


    //set attacker & ask for blocker

    source->addToPile("Attack",this->getEffectiveId(),false);

    BasicCard::use(room,source,targets);
    room->getThread()->delay();


    // reveal attacker
    CombatRevealStruct reveal;
    reveal.revealed = Sanguosha->getCard(source->getPile("Attack").first());
    reveal.who      = source ;
    reveal.attacker = true ;
    foreach(ServerPlayer* player,room->getAlivePlayers())
        reveal.opponets << player;

    QVariant data = QVariant::fromValue(reveal);

    bool broken=room->getThread()->trigger(CombatReveal,source,data);
    if(broken || !reveal.revealed->inherits("CombatCard"))return;
    reveal = data.value<CombatRevealStruct>();

    const Card * attackCard = reveal.revealed;

    LogMessage log;
    log.type = "$revealResult";
    log.from = source;
    log.card_str = attackCard->getEffectIdString();
    room->sendLog(log);

    room->throwCard(attackCard);

    broken = room->getThread()->trigger(CombatRevealed,source,data);
    if(broken)return;
    room->getThread()->delay();


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
                block_reveal.revealed = Sanguosha->getCard(pile.first());
                block_reveal.who      = player;
                block_reveal.opponets << source;

                data = QVariant::fromValue(block_reveal);
                broken = room->getThread()->trigger(CombatReveal,player,data);
                if(broken)continue;
                block_reveal = data.value<CombatRevealStruct>();

                blocker = block_reveal.revealed;

                LogMessage log;
                log.type = "$revealResult";
                log.from = player;
                log.card_str = blocker->getEffectIdString();
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
            combat.combat = qobject_cast<const CombatCard*>(attackCard);
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

            const CombatCard * ccard=qobject_cast<const CombatCard*>(combat.block);
            if((!combat.block->inherits("CombatCard")) || ccard->canbeBlocked(combat.combat))
                combat.combat->resolveAttack(combat);

            if(combat.combat->canbeBlocked(combat.block))
                ccard->resolveDefense(combat);

            if(combat.block->inherits("DummyCard"))delete combat.block;

            room->getThread()->trigger(CombatFinished,source,data);
            room->getThread()->trigger(TargetFinished,player,data);

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
        combat.from->setFlags("jilei");
    }
}

void Strike::resolveDefense(CombatStruct &combat) const
{
    if(combat.to->getRoom()->obtainable(this,combat.to))
    {
        combat.to->getRoom()->obtainCard(combat.to,this->getEffectiveId());
//        combat.to->jilei(QString(this->getEffectiveId()));
//        combat.to->invoke("jilei",this->getEffectIdString());
//        combat.to->setFlags("jilei");
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
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target->getDefensiveHorse() && target->getDefensiveHorse()->objectName() == objectName();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        MpChangeStruct change = data.value<MpChangeStruct>();
        if(change.delta<0)return false;

        Room *room = player->getRoom();
        if(!room->askForSkillInvoke(player,objectName()))return false;

        change.delta++;
        data = QVariant::fromValue(change);

        LogMessage log;
        log.type = "#Zunhat" ;
        log.from = player ;
        log.arg  = change.delta-1;
        log.arg2 = change.delta;
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
    int i=0;
    for(;i<104;i++)
        if(i<28)
            cards<<new Barrage((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<38)
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
        else if(i<94)
            cards<<new Yukkuri((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<95)
            cards<<new Pants((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<96)
            cards<<new Broomstick((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<97)
            cards<<new ZunHat((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<98)
            cards<<new Mushroom((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<99)
            cards<<new Tea((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<100)
            cards<<new Sinbag((Card::Suit)(i/26),(i%26)/2+1);
        else if(i<101)
            cards<<new Saisen((Card::Suit)(i/26),(i%26)/2+1);


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

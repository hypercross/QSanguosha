#include "touhou-generals.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"
#include "touhoucards.h"

GuifuCard::GuifuCard()
{
    setObjectName("guifu");
}

bool GuifuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return SkillCard::targetFilter(targets,to_select,Self) &&
            !to_select->hasSkill(objectName() + "_constraint");
}

void GuifuCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    if(source->getMp()<1)return;
    room->changeMp(source,-1);
    foreach(ServerPlayer * sp,targets)
    {
        if(sp->hasSkill(objectName() + "_constraint"))continue;

        LogMessage log;
        log.type = "#SetConstraint";
        log.from = source;
        log.arg   = sp->getGeneralName();

        room ->sendLog(log);
        //room ->attachSkillToPlayer(sp ,objectName() + "_detacher");
        //room ->attachSkillToPlayer(sp ,objectName() + "_constraint");
        room ->acquireSkill(sp ,"#" + objectName() + "_detacher");
        room->acquireSkill(sp ,objectName() + "_constraint");
        sp->addMark("Chain");
        sp->setChained(true);
        room->broadcastProperty(sp,"chained");
    }
}

void GuifuCard::ApplyChain(const QString &objectname,ServerPlayer* sp, ServerPlayer *source)
{
    Room* room = sp->getRoom();

    if(sp->hasSkill(objectname + "_constraint"))return;

    LogMessage log;
    log.type = "#SetConstraint";
    log.from = source;
    log.arg   = sp->getGeneralName();

    room ->sendLog(log);
    //room ->attachSkillToPlayer(sp ,objectName() + "_detacher");
    //room ->attachSkillToPlayer(sp ,objectName() + "_constraint");
    room ->acquireSkill(sp ,"#" + objectname + "_detacher");
    room->acquireSkill(sp ,objectname + "_constraint");
    sp->addMark("Chain");
    sp->setChained(true);
    room->broadcastProperty(sp,"chained");
}

class GuifuDetacher : public DetacherSkill
{
public:
    GuifuDetacher():DetacherSkill("guifu")
    {
    }
};

class GuifuConstraint : public ConstraintSkill
{
public:
    GuifuConstraint():ConstraintSkill("guifu")
    {
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        Room* room = player->getRoom();
        room->askForDiscard(player,"guifu",1);
        return false;
    }
};

class GuifuViewAs : public ZeroCardViewAsSkill
{
public:
    GuifuViewAs():ZeroCardViewAsSkill("guifu")
    {
    }

    virtual const Card* viewAs() const
    {
        return new GuifuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp()>=1;
    }
};

class MusouViewAs : public OneCardViewAsSkill
{
public:
    MusouViewAs():OneCardViewAsSkill("musoutensei")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) || player->hasWeapon("hakkero");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "rune" || pattern == ".combat" || pattern == ".";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        return card->getSuit() == Card::Spade && !card->isEquipped();

    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *rune = new Rune(card->getSuit(), card->getNumber());
        rune->addSubcard(card->getId());
        rune->setSkillName(objectName());
        return rune;
    }

};

class Baka : public TriggerSkill
{
public:
    Baka():TriggerSkill("baka")
    {
        events << AttackDeclared << BlockDeclared << PhaseChange;

        frequency = Compulsory;
    }

    virtual int getPriority()
    {
        return -1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(event == AttackDeclared)
        {
            player->getRoom()->showCard(player,data.value<CardStar>()->getEffectiveId());
        }
        else if(event == BlockDeclared){
            int cid = data.value<CombatStruct>().block->getEffectiveId();
            player->getRoom()->showCard(player,cid);
        }
        else if(player->getPhase() == Player::Discard)
        {
            int value = qBound(-10,3 - player->getHp(),0);
            player->setXueyi(value,false);
        }
        return false;
    }
};

FreezeCard::FreezeCard()
{
    setObjectName("perfect_freeze");
}

bool FreezeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return !to_select->isAllNude() && SkillCard::targetFilter(targets,to_select,Self);
}

void FreezeCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer * to =effect.to ;
    Room * room = to->getRoom();
    ServerPlayer * from =effect.from;

    if(from->getMp()<1)return;
    room->changeMp(from,-1);

    int cid = room->askForCardChosen(from,to,"hej",objectName());

    room->obtainCard(to,cid);
    room->showCard(to,cid);
    to->jilei(QString::number(cid));
    to->invoke("jilei",QString::number(cid));
    to->setFlags("jilei");
}

class PerfectFreeze : public ZeroCardViewAsSkill
{
public:
    PerfectFreeze():ZeroCardViewAsSkill("perfect_freeze")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp()>0;
    }

    virtual const Card* viewAs() const
    {
        return new FreezeCard;
    }

};

class WindGirl : public FilterSkill
{
public:
    WindGirl():FilterSkill("wind_girl")
    {

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getCard();

        return card->inherits("CombatCard") && !card->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        int number = card->getNumber() + 3 ;
        if (number > 13) number = 13;

        Card *newCombat;
        if(card->inherits("Rune")) newCombat = new Rune(card->getSuit(),number);
        else if(card->inherits("Strike")) newCombat = new Strike(card->getSuit(),number);
        else if(card->inherits("Barrage")) newCombat = new Barrage(card->getSuit(),number);

        newCombat->addSubcard(card->getId());
        newCombat->setSkillName(objectName());
        return newCombat;
    }

};

class FastshotViewas : public OneCardViewAsSkill
{
public:
    FastshotViewas():OneCardViewAsSkill("fast_shot")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp()>0;
    }

    virtual bool viewFilter(const CardItem *to_select) const
    {
        return to_select->getCard()->isBlack();
    }

    virtual const Card* viewAs(CardItem *card_item) const
    {
        const Card* card = card_item->getFilteredCard();
        Surprise * fscard=new Surprise(card->getSuit(),card->getNumber());
        fscard->addSubcard(card);
        fscard->setSkillName(objectName());
        return fscard;
    }
};

class Fastshot : public TriggerSkill
{
public:
    Fastshot():TriggerSkill("fast_shot")
    {
        view_as_skill = new FastshotViewas;

        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if(!use.card->isVirtualCard())return false;
        if(!use.card->inherits("Surprise"))return false;

        if(player->getMp()<1)return true;
        player->getRoom()->changeMp(player,-1);

        return false;
    }
};

class PhoenixTailViewas : public OneCardViewAsSkill
{
public:
    PhoenixTailViewas():OneCardViewAsSkill("phoenix_tail")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp()>1;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "peach+analeptic" && player->getMp()>1;
    }

    virtual bool viewFilter(const CardItem *to_select) const
    {
        return to_select->getCard()->getSuit() == Card::Diamond;
    }

    virtual const Card* viewAs(CardItem *card_item) const
    {
        const Card* card = card_item->getFilteredCard();
        ExSpell * fscard=new ExSpell(card->getSuit(),card->getNumber());
        fscard->addSubcard(card);
        fscard->setSkillName(objectName());
        return fscard;
    }
};

class PhoenixTail : public TriggerSkill
{
public:
    PhoenixTail():TriggerSkill("phoenix_tail")
    {
        view_as_skill = new PhoenixTailViewas;

        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if(!use.card->isVirtualCard())return false;
        if(!use.card->inherits("ExSpell"))return false;

        if(player->getMp()<2)return true;
        player->getRoom()->changeMp(player,-2);

        return false;
    }
};


class PhoenixSoar : public ZeroCardViewAsSkill
{
public:
    PhoenixSoar():ZeroCardViewAsSkill("phoenix_soar")
    {

    }

    virtual const Card* viewAs() const
    {
        return new PhoenixSoarCard();
    }
};

PhoenixSoarCard::PhoenixSoarCard()
{
    setObjectName("phoenix_soar");

    target_fixed = true;
}

void PhoenixSoarCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    room->changeMp(source,2);
    room->loseHp(source);


    source->jilei(NULL);
    source->invoke("jilei",NULL);

    LogMessage log;
    log.type = "#JileiClear";
    log.from = source;
    room->sendLog(log);

    room->setPlayerFlag(source,"-jilei");
    room->setPlayerFlag(source,"-jilei_temp");
}

DeathlureCard::DeathlureCard()
{
    setObjectName("death_lure");
}

bool DeathlureCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return Self->getMp() > to_select->getHp() && SkillCard::targetFilter(targets,to_select,Self);
}

void DeathlureCard::onEffect(const CardEffectStruct &effect) const
{
    int thresh = effect.to->getHp();
    if(effect.from->getMp() <= thresh)return;

    Room *room = effect.to->getRoom();

    room->changeMp(effect.from, - effect.from->getMp());

    if(!room->askForCard(effect.to,"peach+analeptic","deathlure-pa"))room->loseHp(effect.to,effect.to->getHp());
}

class Deathlure : public ZeroCardViewAsSkill
{
public:
    Deathlure():ZeroCardViewAsSkill("death_lure")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp()>0 ;
    }

    virtual const Card* viewAs() const
    {
        return new DeathlureCard;
    }
};

class SumiSakura : public MasochismSkill
{
public:
    SumiSakura():MasochismSkill("sumi_sakura")
    {

    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room * room =target->getRoom();

        bool invoke = true;

        if(target->getMaxHP()<2)invoke = false;
        else if(!room->askForSkillInvoke(target,objectName()))invoke = false;

        if(invoke)
        {
            room->loseMaxHp(target);
            room->setPlayerProperty(target,"maxmp",target->getMaxMP()+1);

            room->broadcastProperty(target,"maxmp");
        }

        room->changeMp(target,target->getMaxMP());
    }
};

class Munenmusou : public TriggerSkill
{
public:
    Munenmusou():TriggerSkill("munenmusou")
    {
        events << MpChanged ;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        MpChangeStruct change = data.value<MpChangeStruct>();

        if(change.delta >= 0) return false;

        if(player->getMp()<1) return false;

        if(change.delta + player -> getMp() < 1) change.delta = 1 - player->getMp();
        else return false;

        data = QVariant::fromValue(change) ;

        return false;
    }
};

class GuardianKanameishi : public MasochismSkill
{
public:
    GuardianKanameishi():MasochismSkill("guardian_kanameishi")
    {
        frequency = Frequent ;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room * room = target->getRoom();
        int num = 3 - target->getHandcardNum() ;
        if(num > 0 && room->askForSkillInvoke(target,objectName()))room->drawCards(target,num);
    }

};

FuujinSaishiCard::FuujinSaishiCard()
{
    setObjectName("fuujin_saishi");

    target_fixed = true;
}

void FuujinSaishiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    foreach(ServerPlayer *player, room->getAlivePlayers()){
        room->changeMp(player,1);
    }
}

class FuujinSaishi : public ZeroCardViewAsSkill
{
public:
    FuujinSaishi():ZeroCardViewAsSkill("fuujin_saishi")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("FuujinSaishiCard");
    }

    virtual const Card* viewAs() const
    {
        return new FuujinSaishiCard;
    }
};

MosesMiracleCard::MosesMiracleCard()
{
    will_throw = false;
    setObjectName("moses_miracle");

}

void MosesMiracleCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    if(targets.isEmpty())
        return;

    ServerPlayer *target = targets.first();
    room->moveCardTo(this, target, Player::Hand, false);

    QString result = room->askForChoice(source,objectName(),"draw+recover");
    if(result == "draw")
        room->drawCards(source,2);
    else {
        RecoverStruct recover ;
        recover.who = source;
        room->recover(source,recover);
    }
}

class MosesMiracle : public ViewAsSkill
{
public:
    MosesMiracle():ViewAsSkill("moses_miracle")
    {

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        return selected.length()<2 && !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MosesMiracleCard");
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<2)
            return NULL;

        MosesMiracleCard *mmc = new MosesMiracleCard;
        mmc->addSubcards(cards);
        return mmc;
    }
};

class UmbrellaIllusion : public TriggerSkill{
public:
    UmbrellaIllusion():TriggerSkill("umbrella_illusion")
    {
        events << Predamaged ;
    }

    virtual int getPriority() const
    {
        return -2;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getRoom()->getCardPlace(102) == Player::Equip;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.damage >= player->getHp())
        {


            LogMessage log;
            log.type = "#UmbrellaIllusion";
            log.arg  = QString::number(damage.damage);
            log.arg  = QString::number(player->getHp() - 1);
            player->getRoom()->sendLog(log);

            damage.damage = player->getHp() - 1;

            if(damage.damage <= 0)return true;
            data=QVariant::fromValue(damage);

        }
        return false;
    }
};

class UmbrellaRecollect: public TriggerSkill{
public:
    UmbrellaRecollect():TriggerSkill("#umbrella_recollect"){
        events << CardLost;
        frequency = Frequent;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return ! target->hasSkill(objectName());
    }



    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardMoveStar cms = data.value<CardMoveStar>();

        if(cms->card_id != 102)return false;

        ServerPlayer* kogasa = player->getRoom()->findPlayerBySkillName(objectName());

        const Card* umb = Sanguosha->getCard(102) ;
        if(room->getCardPlace(102)==Player::DiscardedPile)kogasa->obtainCard(umb);

        return false;
    }
};

class NightlessCity : public TriggerSkill
{
public:
    NightlessCity():TriggerSkill("nightless_city")
    {
        events << CardLost;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) &&
                target->getRoom()->getCurrent() != target;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        Room *room = player->getRoom();
        CardMoveStar cms = data.value<CardMoveStar>();

        if(cms->to_place != Player::DiscardedPile)return false;

        if(player->getMp()<1)return false;

        if(!room->askForSkillInvoke(player,objectName()))return false;

        room->changeMp(player,-1);

        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(spade|club):(.*)");
        judge.good = false;
        judge.reason = objectName();
        judge.who = room->getCurrent();

        room->judge(judge);
        if(judge.isGood())
        {
            DamageStruct damage;
            damage.from = player;
            damage.to = judge.who;

            room->setEmotion(player, "good");
            room->damage(damage);
        }else room->setEmotion(player, "bad");

        return false;
    }

};

class ImperishableHeart : public TriggerSkill
{
public:
    ImperishableHeart():TriggerSkill("imperishable_heart"){
        frequency = Frequent;

        events << FinishJudge;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        CardStar card = judge->card;
        if(card->getSuit() != Card::Heart ||
                player->getRoom()->getCardPlace(card->getId()) != Player::DiscardedPile)
            return false;

        ServerPlayer* ojoosama = player->getRoom()->findPlayerBySkillName(objectName());
        if(!ojoosama->getRoom()->askForSkillInvoke(ojoosama,objectName()))return false;
        ojoosama->obtainCard(card);

        return false;
    }

};

class RemiliaStalker : public TriggerSkill
{
public:
    RemiliaStalker():TriggerSkill("remilia_stalker")
    {
        events << BlockDeclare;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();
        if(!combat.from->hasSkill(objectName()) ) return false;
        if(combat.from->getAttackRange()<=player->getMp()) return false;

        player->tag["combatEffective"]=true;
        return true;
    }
};

class KillerDoll : public OneCardViewAsSkill
{
public:
    KillerDoll():OneCardViewAsSkill("killer_doll"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) || player->hasWeapon("hakkero");
    }



    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        return card->inherits("Barrage");

    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *strike = new Strike(card->getSuit(), 13);
        strike->addSubcard(card->getId());
        strike->setSkillName(objectName());
        return strike;
    }
};

class DeflatedWorld : public PhaseChangeSkill
{
public:
    DeflatedWorld():PhaseChangeSkill("deflated_world")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();

        if(target->getPhase() != Player::Judge) return false;
        if(target->getMp()<2) return false;

        if(!room->askForSkillInvoke(target,objectName()))return false;

        room->changeMp(target,-2);
        room->setPlayerFlag(target,"tianyi_success");
        return true;
    }
};

class FourNineFive: public TriggerSkill{
public:
    FourNineFive():TriggerSkill("four_nine_five"){
        frequency = Compulsory;
        events << Damage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if(!damage.card)return false;
        if(damage.card->isVirtualCard())return false;

        int cid = damage.card->getEffectiveId();
        int num = Sanguosha->getCard(cid)->getNumber();
        if(num != 4 && num != 9 && num != 5)return false;

        Room *room = player->getRoom();

        room->playSkillEffect(objectName());

        LogMessage log;
        log.type = "#FourNineFiveRecover";
        log.from = player;
        log.arg = QString::number(damage.damage);
        room->sendLog(log);

        RecoverStruct recover;
        recover.who = player;
        recover.recover = damage.damage;
        room->recover(player, recover);

        return false;
    }
};

DaremoinaiCard::DaremoinaiCard()
{
    setObjectName("dare_mo_inai");
}

bool DaremoinaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->distanceTo(to_select) <= Self->getAttackRange();
}

void DaremoinaiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *from = effect.from;
    ServerPlayer *to = effect.to;
    Room * room = to->getRoom();

    int count = from->getMp();
    if(count <= 1 ) return;

    room->changeMp(from,-count);

    bool suits[4] = {false,false,false,false};

    for(int i=0;i<count;i++)
    {
        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(.*):(.*)");
        judge.good = true;
        judge.reason = objectName();
        judge.who = to;

        room->judge(judge);
        suits[judge.card->getSuit()]=true;
    }

    int dmg = count+1;

    for(int i=0;i<4;i++) if(suits[i])dmg--;

    DamageStruct damage;
    damage.from = from;
    damage.to   = to;
    damage.damage=dmg;

    room->damage(damage);

}

class Daremoinai : public ViewAsSkill
{
public:
    Daremoinai():ViewAsSkill("dare_mo_inai")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp()>0;
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        return selected.length()<Self->getMp() && !to_select->isEquipped();
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<Self->getMp())return NULL;
        Card *card = new DaremoinaiCard;
        card->addSubcards(cards);
        return card;
    }
};

class PhilosopherStone: public ViewAsSkill
{
public:
    PhilosopherStone():ViewAsSkill("philosopher_stone")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("PhilosopherStoneCard");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        foreach(CardItem * aselected,selected)
        {
            if(to_select->getCard()->getSuit() == aselected->getCard()->getSuit())return false;
        }

        return selected.length()<4 && !to_select->isEquipped();
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<1)return NULL;

        Card *psc = new PhilosopherStoneCard;

        psc->addSubcards(cards);
        return psc;

    }
};

PhilosopherStoneCard::PhilosopherStoneCard()
{
    setObjectName("philosopher_stone");

    will_throw = true ;
}

bool PhilosopherStoneCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return SkillCard::targetFilter(targets,to_select,Self)
            && getSubcards().length()>3 && Self->getMp()>3;
}

bool PhilosopherStoneCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return getSubcards().length()<4 || targets.length()>0;
}

void PhilosopherStoneCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{

    room->throwCard(this);
    int cases = getSubcards().length();

    RecoverStruct recover;
    recover.who = source;

    switch(cases)
    {
    case 1:
        room->recover(source,recover);
        break;
    case 2:
        room->drawCards(source,2);
        break;
    case 3:
        room->setPlayerMark(source,"extra_turn",1);
        break;
    case 4:
        if(source->getMp()<4)return;
        room->changeMp(source,-4);

        ServerPlayer* sp  = targets.first();
        if(sp->hasSkill(objectName() + "_constraint"))return;

        LogMessage log;
        log.type = "#SetConstraint";
        log.from = source;
        log.arg   = sp->getGeneralName();

        room ->sendLog(log);
        //room ->attachSkillToPlayer(sp ,objectName() + "_detacher");
        //room ->attachSkillToPlayer(sp ,objectName() + "_constraint");
        room ->acquireSkill(sp ,"#" + objectName() + "_detacher");
        room->acquireSkill(sp ,objectName() + "_constraint");
        sp->addMark("Chain");
        sp->setChained(true);
        room->broadcastProperty(sp,"chained");
        break;
    }
}


class PhilosopherStoneDetacher : public DetacherSkill
{
public:
    PhilosopherStoneDetacher():DetacherSkill("philosopher_stone")
    {
    }

    virtual bool validPhaseChange(ServerPlayer *player, QVariant &data) const
    {
        return false;
    }
};

class PhilosopherStoneConstraint : public ConstraintSkill
{
public:
    PhilosopherStoneConstraint():ConstraintSkill("philosopher_stone")
    {
        events << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(player->getPhase() != Player::Start)return false;

        Room * room =player->getRoom();

        ServerPlayer *pachuli = room->findPlayerBySkillName("philosopher_stone");
        if(!pachuli->askForSkillInvoke(objectName()))return false;

        room->doGuanxing(pachuli, room->getNCards(4, false),false);
        return false;
    }
};

class PhilosopherStonePhaseChange : public PhaseChangeSkill
{
public:
    PhilosopherStonePhaseChange():PhaseChangeSkill("#philosopher_stone")
    {

    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::NotActive
                && target->getMark("extra_turn") > 0;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        target->setMark("extra_turn",0);

        Room *room = target->getRoom();

        LogMessage log;
        log.type = "#PSExtraTurn";
        log.from = target;
        room->sendLog(log);

        room->getThread()->trigger(TurnStart, target);

        return false;
    }
};

class DoubleSwordMaster: public TriggerSkill
{
public:
    DoubleSwordMaster():TriggerSkill("double_sword_master")
    {
        events << CombatFinished;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct cs = data.value<CombatStruct>();

        if(!cs.combat->canbeBlocked(cs.block))
        {
            if(player->getMp()==player->getMaxMP())return false;
            if(!player->getRoom()->askForSkillInvoke(player,objectName()))return false;
            player->getRoom()->changeMp(player,1);
        }else
        {
            if(cs.to->containsTrick("supply_shortage"))return false;
            const Card* basic = player->getRoom()->askForCard(player,".basic","snow-basic",false);
            if(!basic)return false;

            SupplyShortage *shortage = new SupplyShortage(basic->getSuit(), basic->getNumber());
            shortage->setSkillName(objectName());
            shortage->addSubcard(basic);

            CardUseStruct use;
            use.card = shortage;
            use.from = player;
            use.to << cs.to ;

            player->getRoom()->useCard(use,false);
        }
        return false;
    }
};

class Arcanum : public ViewAsSkill
{
public:
    Arcanum():ViewAsSkill("arcanum")
    {

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        foreach(CardItem *aselected,selected)
            if(to_select->getCard()->getSuit() != aselected->getCard()->getSuit())
                return false;
        return selected.length()<2 && !to_select->isEquipped();
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<2)return NULL;

        GodSalvation *salve = new GodSalvation(cards.first()->getCard()->getSuit(),0);
        salve->addSubcards(cards);
        salve->setSkillName(objectName());
        return salve;
    }
};

class WorldJar : public ZeroCardViewAsSkill
{
public:
    WorldJar():ZeroCardViewAsSkill("worldjar")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp() >= Self->aliveCount()/2 && !player->hasUsed("WorldJarCard");
    }

    virtual const Card* viewAs() const
    {
        return new WorldJarCard;
    }
};

WorldJarCard::WorldJarCard()
{
    setObjectName("worldjar");
}

bool WorldJarCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return targets.length() >= Self->aliveCount()/2;
}

bool WorldJarCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->aliveCount()/2;
}

void WorldJarCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    if(source->getMp()<targets.length())return;
    room->changeMp(source, - targets.length());
    foreach(ServerPlayer * sp,targets)GuifuCard::ApplyChain(objectName(),sp, source);
}

class WorldJarDetacher: public DetacherSkill
{
public:
    WorldJarDetacher():DetacherSkill("worldjar")
    {

    }
};

class WorldJarProhibit:public ProhibitSkill
{
public:
    WorldJarProhibit():ProhibitSkill("#worldjar")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const
    {
        return (from->hasSkill("worldjar_constraint")) != (to->hasSkill("worldjar_constraint"))
                && !card->isVirtualCard();
    }

    virtual bool isGlobal() const
    {
        return true;
    }
};

void TouhouPackage::addGenerals()
{
    General *lingmeng = new General(this,"reimu","_hrp", 3, false ,false, 4);
    lingmeng->addSkill(new GuifuViewAs);
    lingmeng->addSkill(new MusouViewAs);

    General *chiruno = new General(this,"chiruno","_esd", 9 , false , false , 1);
    chiruno->addSkill(new Baka);
    chiruno->addSkill(new PerfectFreeze);

    General *aya =new General(this,"aya","_stb",3,false,false,3);
    aya->addSkill(new WindGirl);
    aya->addSkill(new Fastshot);

    General *mokou = new General(this,"mokou","_in",4,false,false,3);
    mokou->addSkill(new PhoenixTail);
    mokou->addSkill(new PhoenixSoar);

    General *yuyuko = new General(this,"yuyuko","_pcb",4,false,false,2);
    yuyuko->addSkill(new Deathlure);
    yuyuko->addSkill(new SumiSakura);

    General *tenshi = new General(this,"tenshi","_swr",4,false,false,2);
    tenshi->addSkill(new Munenmusou);
    tenshi->addSkill(new GuardianKanameishi);

    General *sanai = new General(this,"sanai","_mof",4,false,false,4);
    sanai->addSkill(new FuujinSaishi);
    sanai->addSkill(new MosesMiracle);


    //    General *kogasa = new General(this,"kogasa","_ufo",3,false,false,4);
    //    kogasa->addSkill(new UmbrellaIllusion);
    //    kogasa->addSkill(new UmbrellaRecollect);
    //fix me

    General *remilia = new General(this,"remilia","_esd",3,false,false,4);
    remilia->addSkill(new NightlessCity);
    remilia->addSkill(new ImperishableHeart);
    remilia->addSkill(new RemiliaStalker);

    General *sakuya = new General(this,"sakuya","_esd",4,false,false,2);
    sakuya->addSkill(new KillerDoll);
    sakuya->addSkill(new DeflatedWorld);

    General *flandre = new General(this,"flandre","_esd",3,false,false,4);
    flandre->addSkill(new FourNineFive);
    flandre->addSkill(new Daremoinai);

    General *pachuli = new General(this,"pachuli","_esd",3,false,false,5);
    pachuli->addSkill(new PhilosopherStone);
    pachuli->addSkill(new PhilosopherStonePhaseChange);

    General *youmu = new General(this,"youmu","_pcb",4,false,false,3);
    youmu->addSkill(new DoubleSwordMaster);

    General *eirin = new General(this,"eirin","_in",3,false,false,4);
    eirin->addSkill(new Skill("sharpmind"));
    eirin->addSkill(new WorldJar);
    eirin->addSkill(new WorldJarProhibit);
    eirin->addSkill(new Arcanum);

    skills << new GuifuDetacher << new GuifuConstraint << new PhilosopherStoneDetacher << new PhilosopherStoneConstraint;
    skills << new WorldJarDetacher << new Skill("worldjar_constraint");

    addMetaObject<GuifuCard>();
    addMetaObject<FreezeCard>();
    addMetaObject<PhoenixSoarCard>();
    addMetaObject<DeathlureCard>();
    addMetaObject<FuujinSaishiCard>();
    addMetaObject<MosesMiracleCard>();
    addMetaObject<DaremoinaiCard>();
    addMetaObject<PhilosopherStoneCard>();
    addMetaObject<WorldJarCard>();
}

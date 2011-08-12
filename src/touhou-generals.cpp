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
        return to_select->getCard()->isRed();
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

void TouhouPackage::addGenerals()
{
    General *lingmeng = new General(this,"reimu","wei", 3, false ,false, 4);
    lingmeng->addSkill(new GuifuViewAs);
    lingmeng->addSkill(new MusouViewAs);

    General *chiruno = new General(this,"chiruno","wei", 9 , false , false , 1);
    chiruno->addSkill(new Baka);
    chiruno->addSkill(new PerfectFreeze);

    General *aya =new General(this,"aya","wei",3,false,false,3);
    aya->addSkill(new WindGirl);
    aya->addSkill(new Fastshot);

    General *mokou = new General(this,"mokou","wei",4,false,false,3);
    mokou->addSkill(new PhoenixTail);
    mokou->addSkill(new PhoenixSoar);

    skills << new GuifuDetacher << new GuifuConstraint;
    addMetaObject<GuifuCard>();
    addMetaObject<FreezeCard>();
    addMetaObject<PhoenixSoarCard>();
}

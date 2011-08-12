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
        events << AttackDeclared << BlockDeclared;

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
        else{
            int cid = data.value<CombatStruct>().block->getEffectiveId();
            player->getRoom()->showCard(player,cid);
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

void TouhouPackage::addGenerals()
{
    General *lingmeng = new General(this,"reimu","wei", 3, false ,false, 4);
    lingmeng->addSkill(new GuifuViewAs);
    lingmeng->addSkill(new MusouViewAs);

    General *chiruno = new General(this,"chiruno","wei", 9 , false , false , 1);
    chiruno->addSkill(new Baka);
    chiruno->addSkill(new PerfectFreeze);

    skills << new GuifuDetacher << new GuifuConstraint;
    addMetaObject<GuifuCard>();
    addMetaObject<FreezeCard>();
}

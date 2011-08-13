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

        if(target->getMaxHP() < 3) room->changeMp(target,target->getMaxMP());
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

        if(change.delta + player -> getMp() < 1) change.delta = 1 - player->getMp();
        else return false;

        if(change.delta > 0) return false;

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
        int num = target->getMaxHP() - target->getHandcardNum() ;
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

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<2)
            return NULL;

        MosesMiracleCard *mmc = new MosesMiracleCard;
        mmc->addSubcards(cards);
        return mmc;
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

    General *sanai = new General(this,"sanai","_mof",3,false,false,4);
    sanai->addSkill(new FuujinSaishi);
    sanai->addSkill(new MosesMiracle);

    skills << new GuifuDetacher << new GuifuConstraint;
    addMetaObject<GuifuCard>();
    addMetaObject<FreezeCard>();
    addMetaObject<PhoenixSoarCard>();
    addMetaObject<DeathlureCard>();
    addMetaObject<FuujinSaishiCard>();
    addMetaObject<MosesMiracleCard>();
}

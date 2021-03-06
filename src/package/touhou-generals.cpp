#include "touhou-generals.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"
#include "touhoucards.h"

class SwitchMode : public ZeroCardViewAsSkill
{
public:
    SwitchMode():ZeroCardViewAsSkill("switchmode")
    {

    }

    virtual const Card* viewAs() const
    {
        return new SwitchModeCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("SwitchModeCard");
    }
};


SwitchModeCard::SwitchModeCard()
{
    setObjectName("switchmode");

    target_fixed = true;
}

void SwitchModeCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    source->setSlowMode(!source->slowMode());
    room->broadcastProperty(source,"slow_mode");
    room->setPlayerFlag(source,"IdlingWave");
}


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
    if(!room->changeMp(source,-1))return;
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
        events << CardFinished;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->isVirtualCard())
            return false;
        int jilei = 0;
        if(player->hasFlag("jilei") || player->hasFlag("jilei_temp")){
            QSet<const Card *> jilei_cards;
            QList<const Card *> handcards = player->getHandcards();
            foreach(const Card *card, handcards){
                if(player->isJilei(card))
                    jilei_cards << card;
            }

            if(jilei_cards.size() == player->getHandcardNum()){
                // show all his cards
                player->getRoom()->showAllCards(player);
                return false;
            }

            jilei = jilei_cards.size();
        }
        if(player->getHandcardNum() - jilei)
            player->getRoom()->askForDiscard(player,"@guifu-constraint",1);
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

class Musoutensei: public OneCardViewAsSkill
{
public:
    Musoutensei():OneCardViewAsSkill("musoutensei")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return CombatCard::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "rune" ||
               pattern == ".combat" ||
               pattern == "strike" ||
               pattern == ".";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        return card->inherits("Strike") ||
               card->inherits("Rune");

    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *convert;
        if(card->inherits("Strike"))convert = new Rune(card->getSuit(), card->getNumber());
        else convert = new Strike(card->getSuit(), card->getNumber());
        convert->addSubcard(card->getId());
        convert->setSkillName(objectName());
        return convert;
    }

};

class Musoufuuin: public TriggerSkill
{
public:
    Musoufuuin():TriggerSkill("musoufuuin"){
        frequency = Compulsory;
        events << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if(damage.card && damage.card->inherits("Rune"))
        {
            LogMessage log;
            log.type = "#MusouProtect";
            log.from = player;
            player->getRoom()->sendLog(log);

            return true;
        }

        return false;
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

        return false;
    }
};

PerfectFreezeCard::PerfectFreezeCard()
{
    setObjectName("perfectfreeze");

    target_fixed = true ;
}

void PerfectFreezeCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    ServerPlayer * to = players.at(qrand() % players.size());
    ServerPlayer * from =source;

    if(!room->changeMp(from,-1))return;

    int cid = room->askForCardChosen(from,to,"hej","perfect_freeze");

    room->obtainCard(to,cid);
    room->showCard(to,cid);
    to->jilei(QString::number(cid));
    to->invoke("jilei",QString::number(cid));
    to->setFlags("jilei");

    LogMessage log;
    log.type = "$JileiA";
    log.from = to;
    log.card_str = Sanguosha->getCard(cid)->toString();
    room->sendLog(log);
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
        return new PerfectFreezeCard;
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
        return TriggerSkill::triggerable(target) && target->getRoom()->getCardPlace(104) == Player::Equip;
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
        if(!kogasa)return false;
        const Card* umb = Sanguosha->getCard(102) ;
        if(room->getCardPlace(102)==Player::DiscardedPile)kogasa->obtainCard(umb);

        return false;
    }
};

class ScareViewas : public ZeroCardViewAsSkill
{
public:
    ScareViewas():ZeroCardViewAsSkill("scare_viewas")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMp();
    }

    virtual const Card* viewAs() const
    {
        return new Surprise(Card::NoSuit,0);
    }
};

class Scare : public TriggerSkill
{
public:
    Scare():TriggerSkill("scare")
    {
        view_as_skill = new ScareViewas;

        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if(!use.card->isVirtualCard())return false;
        if(!use.card->inherits("Surprise"))return false;

        player->getRoom()->changeMp(player,-1);

        return false;
    }
};

class Forgotten : public PhaseChangeSkill
{
public:
    Forgotten():PhaseChangeSkill("forgotten")
    {

    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::Finish
                && target->getMp();
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if(!target->getMp())return false;
        if(!target->askForSkillInvoke(objectName()))return false;
        target->getRoom()->changeMp(target,-1);
        ServerPlayer* tgt = target->getRoom()->askForPlayerChosen(target,target->getRoom()->getAlivePlayers(),objectName());

        GuifuCard::ApplyChain(objectName(),tgt,target);
        return false;
    }
};

class ForgottenDetacher : public DetacherSkill
{
public:
    ForgottenDetacher():DetacherSkill("forgotten")
    {
        events << CombatTargetDeclare;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(event == PhaseChange)return false;
        if(data.value<CombatStruct>().to->hasSkill("forgotten"))
            DetacherSkill::Detach(player,this);

        return false;
    }
};

class ForgottenProhibit : public ProhibitSkill
{
public:
    ForgottenProhibit():ProhibitSkill("#forgotten_prohibit")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const
    {
        return from->hasSkill("forgotten_constraint") && card->inherits("TrickCard");
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
            room->changeMp(player,1);
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
        if(!ojoosama)return false;
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
        return CombatCard::IsAvailable(player);
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

class PerfectMaid: public TriggerSkill
{
public:
    PerfectMaid():TriggerSkill("perfectmaid"){
        frequency = Compulsory;
        events << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if(damage.card && damage.card->inherits("Rune"))
        {
            LogMessage log;
            log.type = "#MusouProtect";
            log.from = player;
            player->getRoom()->sendLog(log);

            return true;
        }

        return false;
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
        target->skip(Player::Draw);
        return true;
    }
};

class FourNineFive: public TriggerSkill{
public:
    FourNineFive():TriggerSkill("four_nine_five"){
        frequency = Compulsory;
        events << CardUsed << CardResponsed << CombatRevealed;
    }

    virtual int getPriority()
    {
        return -2;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        const Card* card;
        if( event == CardUsed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            if(card->inherits("CombatCard"))return false;
        }else if(event == CardResponsed)
        {
            card = data.value<CardStar>();

        }else
        {
            CombatRevealStruct reveal = data.value<CombatRevealStruct>();
            card =reveal.revealed;
        }
        int cid = card->getEffectiveId();
        if(cid<0)return false;
        int num = Sanguosha->getCard(cid)->getNumber();
        if(num != 4 && num != 9 && num != 5)return false;

        Room *room = player->getRoom();

        room->playSkillEffect(objectName());


        RecoverStruct recover;
        recover.who = player;
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

    int dmg = count;
    suits[Card::Heart] = false;

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
        return player->getMp()>1;
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        return selected.length()<1 && !to_select->isEquipped() && to_select->getCard()->getSuit() == Card::Heart;
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<1)return NULL;
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

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "nullification";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        foreach(CardItem * aselected,selected)
        {
            if(to_select->getCard()->getSuit() == aselected->getCard()->getSuit())return false;
        }

        if(ClientInstance->getPattern() == "nullification") return selected.length()<1;

        return selected.length()<4;
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<1)return NULL;

        if(ClientInstance->getPattern() == "nullification"){
            const Card *first = cards.first()->getFilteredCard();
            Card *ncard = new Nullification(first->getSuit(), first->getNumber());
            ncard->addSubcard(first);
            ncard->setSkillName(objectName());
            return ncard;
        }

        if(cards.length()<2)return NULL;

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
    case 2:
        room->recover(source,recover);
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
        if(!pachuli)return false;
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

        if(CombatCard::BattleJudge(cs.combat,cs.block))
        {
            if(player->getMp()==player->getMaxMP())return false;
            if(!player->getRoom()->askForSkillInvoke(player,objectName()))return false;
            player->getRoom()->changeMp(player,1);
        }else
        {
            if(cs.to->containsTrick("supply_shortage"))return false;
            if(player->getMp()<1)return false;
            const Card* basic = player->getRoom()->askForCard(player,".basic","snow-basic",true);
            if(!basic)return false;

            player->getRoom()->changeMp(player,-1);

            SupplyShortage *shortage = new SupplyShortage(basic->getSuit(), basic->getNumber());
            shortage->setSkillName(objectName());
            shortage->addSubcard(basic);

            player->getRoom()->moveCardTo(shortage,cs.to,Player::Judging,true);
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
        return player->getMp() > 1 && !player->hasUsed("WorldJarCard");
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
    if(source->getMp()< 2 )return;
    room->changeMp(source, - 2);
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
                && !card->inherits("SkillCard");
    }

    virtual bool isGlobal() const
    {
        return true;
    }
};

class AllLost :public GameStartSkill
{
public:
    AllLost():GameStartSkill("alllost")
    {

    }

    virtual void onGameStart(ServerPlayer *player) const
    {
        player->getRoom()->changeMp(player,player->getMaxMP());
    }
};

class RealSumiSakuraViewas: public ZeroCardViewAsSkill{
public:
    RealSumiSakuraViewas():ZeroCardViewAsSkill("sumisakura"){
    }

    virtual const Card *viewAs() const{
        return new RealSumiSakuraCard;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@sumisakura";
    }
};

class RealSumiSakura: public PhaseChangeSkill{
public:
    RealSumiSakura():PhaseChangeSkill("sumisakura"){
        default_choice = "gainmp";

        view_as_skill = new RealSumiSakuraViewas;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::Start
                && target->getMp() < target->getMaxMP();
    }

    virtual bool onPhaseChange(ServerPlayer *yuyuko) const{
        Room *room = yuyuko->getRoom();
        room->askForUseCard(yuyuko, "@@sumisakura", "@sumisakura");

        return false;
    }
};

RealSumiSakuraCard::RealSumiSakuraCard()
{
    mute = true;
}

void RealSumiSakuraCard::onEffect(const CardEffectStruct &effect) const
{
    int num = effect.from->getMaxMP() -effect.from->getMp();
    Room *room = effect.to->getRoom();

    QString choice = room->askForChoice(effect.from,"sumisakura","gainmp+gaincard");

    if(choice == "gainmp")
    {

        room->changeMp(effect.to,num);

        num = qMin(num,effect.to->getCardCount(false)) ;
        room->askForDiscard(effect.to,"sumisakura",num);
    }

    else

    {
        room->changeMp(effect.to,-num);

        room->drawCards(effect.to,num);
    }
}

class NightReturn : public PhaseChangeSkill
{
public:
    NightReturn():PhaseChangeSkill("nightreturn")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target->getMp()>1 && target->getPhase() == Player::Judge
                &&PhaseChangeSkill::triggerable(target);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room * room = target->getRoom();

        if(!room->askForSkillInvoke(target,objectName()))return false;

        room->changeMp(target,-2);

        target->skip(Player::Play);
        target->skip(Player::Discard);

        return true;
    }
};

FiveProblemCard::FiveProblemCard()
{
}

void FiveProblemCard::onEffect(const CardEffectStruct &effect) const
{
    if(effect.from->getMp()<1)return;
    effect.from->getRoom()->changeMp(effect.from,-1);

    CardStar card = effect.to->getRoom()->getTag("FiveProblemCard").value<CardStar>();
    const Card* c = card;
    effect.to->obtainCard(card);

    effect.to->setFlags("jilei");
    effect.to->jilei(c->getEffectIdString());
    effect.to->invoke("jilei",c->getEffectIdString());

    LogMessage log;
    log.type = "$JileiA";
    log.from = effect.to;
    log.card_str = card->toString();
    effect.from->getRoom()->sendLog(log);
}

class FiveProblemViewAs: public ZeroCardViewAsSkill
{
public:
    FiveProblemViewAs():ZeroCardViewAsSkill("fiveproblem"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@fiveproblem";
    }

    virtual const Card *viewAs() const{
        return new FiveProblemCard;
    }
};

class FiveProblem : public MasochismSkill
{
public:
    FiveProblem():MasochismSkill("fiveproblem")
    {
        view_as_skill = new FiveProblemViewAs;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room* room = target->getRoom();
        if(!room->obtainable(damage.card,target))return;
        if(target->getMp()<1)return;

        room->setTag("FiveProblemCard",QVariant::fromValue(damage.card));

        room->askForUseCard(target,"@@fiveproblem","@fiveproblem");
    }

};

class RealmController : public TriggerSkill
{
public:
    RealmController():TriggerSkill("realmcontroller")
    {
        events << ConstraintLose;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {

        Room *room =player->getRoom();
        ServerPlayer * yukari = room->findPlayerBySkillName(objectName());
        if(!yukari)return false;

        //if(yukari->getMp()<1)return false;
        if(!room->askForCard(yukari,".C","@realmcontroller"))return false;
        return true;
    }

};

class BlackButterflyConstraint : public ConstraintSkill
{
public:
    BlackButterflyConstraint():ConstraintSkill("blackbutterfly")
    {
        events << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(player->getPhase() != Player::Play)return false;
        Room * room = player->getRoom();

        const Card * card = room->askForCard(player,".C","@black-butterfly");
        if(!card)
        {
            LogMessage log;
            log.type = "#blackbutterfly-losthp";
            log.from = player;
            room->sendLog(log);
            room->loseHp(player);
        }

        return false;
    }
};

class BlackButterflyDetacher : public DetacherSkill
{
public:
    BlackButterflyDetacher():DetacherSkill("blackbutterfly")
    {

    }

    virtual bool validPhaseChange(ServerPlayer *player, QVariant &data) const
    {
        return player->getPhase()==Player::Draw;
    }
};

class BlackButterfly : public MasochismSkill
{
public:
    BlackButterfly():MasochismSkill("blackbutterfly")
    {

    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room * room = target->getRoom();

        QList<ServerPlayer *> other_players = room->getOtherPlayers(target);

        if(target->getMp()<1)return;
        if(!room->askForSkillInvoke(target,"blackbutterfly"))return;
        room->changeMp(target,-1);

        foreach(ServerPlayer* aplayer, other_players)
        {
            GuifuCard::ApplyChain(objectName(),aplayer,target);
        }
    }
};

class Kamikakushi: public DistanceSkill{
public:
    Kamikakushi():DistanceSkill("kamikakushi"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(!to->hasSkill(objectName()))return 0;
        static bool bypass = true;

        if(bypass=(!bypass))return to->isChained() ? 1 : 0;

        int orig = from->rawDistanceTo(to);

        return qMax(to->isChained() ? 1 : 0,2 - orig);

    }
};

class SubterraneanSun : public TriggerSkill
{
public:
    SubterraneanSun():TriggerSkill("subterraneansun")
    {
        events << AttackDeclared << Predamage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        Room * room = player->getRoom();

        if(event == AttackDeclared)
        {
            const Card * card = data.value<CardStar>();

            if(player->getMp()<2)return false;
            if(!room->askForSkillInvoke(player,objectName()))
                return false;
            room->changeMp(player,-2);

            room->showCard(player,card->getEffectiveId());

            if(!card->inherits("Barrage"))return false;
            room->setTag("SunDamagePlus",card->toString());

            return false;
        }

        DamageStruct damage = data.value<DamageStruct>();

        if(!damage.card ||
                damage.card->toString() != room->getTag("SunDamagePlus").toString())
            return false;

        room->setTag("SunDamagePlus",QString("-1"));

        LogMessage log;
        log.type = "#SunDamagePlus";
        log.from = player;
        log.to << damage.to;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(damage.damage + 1);
        player->getRoom()->sendLog(log);

        damage.damage ++;
        data = QVariant::fromValue(damage);

        return false;
    }

};

class MegaFlare : public OneCardViewAsSkill{
public:
    MegaFlare():OneCardViewAsSkill("megaflare"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->getSuit()==Card::Spade;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Dismantlement *dismantlement = new Dismantlement(first->getSuit(), first->getNumber());
        dismantlement->addSubcard(first->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }
};

class Catwalk : public TriggerSkill
{
public:
    Catwalk():TriggerSkill("catwalk")
    {
        events << BlockDeclare;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        Room *room = player->getRoom();

        if(!player->getMp())return false;

        if(player->getPile("Defense").size())return false;

        if(!room->askForSkillInvoke(player,objectName()))return false;

        if(player->getPile("Defense").size())room->throwCard(player->getPile("Defense").first());
        else player->gainMark("@koishi");
        const Card* card = room->peek();
        room->drawCards(player,1);
        player->addToPile("Defense",card->getId(),false);

        LogMessage log;
        log.type = "#chosenCatwalk";
        log.from = player;
        room->sendLog(log);

        return false;
    }
};

class FireWheel : public TriggerSkill
{
public:
    FireWheel():TriggerSkill("firewheel")
    {
        events << AttackDeclared << CombatTargetDeclare;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        Room * room = player->getRoom();

        if(event == AttackDeclared){

            const Card * card = data.value<CardStar>();

            if(player->getMp()<1)return false;
            if(!room->askForSkillInvoke(player,objectName()))return false;

            room->changeMp(player,-1);
            room->showCard(player,card->getEffectiveId());

            if(!card->inherits("Rune"))return false;

            room->setPlayerFlag(player,"firewheel");
            return false;
        }else{

            CombatStruct combat = data.value<CombatStruct>();
            if(!combat.from->hasFlag("firewheel"))return false;

            if(!room->askForCard(combat.to,"strike","@firewheel"))
            {
                DamageStruct damage ;
                damage.from = combat.from;
                damage.to   = combat.to ;
                room->damage(damage);
            }

            return true;
        }
    }

};

class OstinateStone : public TriggerSkill
{
public:
    OstinateStone():TriggerSkill("ostinatestone")
    {
        events << PhaseChange << CombatRevealed << BlockDeclared;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(player->getPhase() == Player::Start)
        {



            Room * room =player->getRoom();
            const Card* block = room->askForCard(player,".","ostinateCard",true);
            if(block)
            {
                if(player->getPile("Defense").size())room->throwCard(player->getPile("Defense").first());
                else player->gainMark("@koishi");
                player->addToPile("Defense",block->getEffectiveId(),false);


                LogMessage log;
                log.type = "#chosenOstinate";
                log.from = player;
                room->sendLog(log);
            }
        }

        if(event == CombatRevealed)
        {
            CombatRevealStruct reveal  = data.value <CombatRevealStruct>();
            if(reveal.attacker)return false;
            player->loseMark("@koishi");
        }
        if(event == BlockDeclared)player->gainMark("@koishi");
        return false;
    }
};

class UnconsViewas : public ZeroCardViewAsSkill
{
public:
    UnconsViewas():ZeroCardViewAsSkill("unconsciousness")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.startsWith("@@unc");
    }

    virtual const Card* viewAs() const
    {
        QString pattern = ClientInstance->getPattern();

        QRegExp rx("@@unc_(\\d+)");
        if(rx.exactMatch(pattern))return Sanguosha->getCard(rx.capturedTexts().at(1).toInt());
        return NULL;
    }
};

class Unconsciousness : public TriggerSkill
{
public:
    Unconsciousness():TriggerSkill("unconsciousness")
    {
        events << CardLost << DrawNCards;

        view_as_skill = new UnconsViewas;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target)
                &&( target->getPhase() == Player::Discard
                || target->getPhase() == Player::Draw);
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(event == DrawNCards )
        {
            if(player->getMp()<1)return false;
            if(player->getPhases().indexOf(Player::Play) == -1 || !player->getRoom()->askForSkillInvoke(player,objectName()))return false;

            player->getRoom()->changeMp(player,-1);
            data = QVariant::fromValue(data.toInt() + 1);
            player->skip(Player::Play);
            return false;
        }

        const CardMoveStruct* move = data.value<CardMoveStar>();
        if(move->from_place != Player::Hand)
            return false;

        const Card* card =Sanguosha->getCard(move->card_id);

        if(card->inherits("CombatCard"))return false;
        if(!card->isAvailable(player))return false;
        if(move->from_place != Player::Hand)return false;
        QString pattern = "@@unc_" + QString::number(move->card_id);

        player->getRoom()->askForUseCard(player,pattern,"@unc:::" + card->objectName());

        return false;
    }
};

class OpticalCamo : public TriggerSkill
{
public:
    OpticalCamo():TriggerSkill("opticalcamo")
    {
        events << BlockDeclare;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return !target->hasSkill(objectName());
    }

    virtual int  getPriority() const
    {
        return -1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        Room * room = player->getRoom();
        ServerPlayer * nitori = room->findPlayerBySkillName(objectName());
        if(!nitori)return false;

        if(nitori->getHandcardNum()<1 || nitori->getMp()<1)return false;

        const Card * block = room->askForCard(nitori,".","@camo-card:" + player->objectName(),true);
        if(!block)return false;

        room->changeMp(nitori,-1);
        room->drawCards(nitori,1);
        player->tag["combatEffective"]=true;
        player->addToPile("Defense",block->getEffectiveId(),false);

        LogMessage log;
        log.type = "#chosenCamo";
        log.from = player;
        room->sendLog(log);

        CombatStruct effect = data.value<CombatStruct>();
        effect.block = block;

        QVariant declareData = QVariant::fromValue(effect);

        room->getThread()->trigger(BlockDeclared, effect.to, declareData);
        return true;

    }
};

class ExtendedArm : public OneCardViewAsSkill
{
public:
    ExtendedArm():OneCardViewAsSkill("extendedarm")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return CombatCard::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "strike" || pattern == ".combat" || pattern == ".";
    }

    virtual bool viewFilter(const CardItem *to_select) const
    {
        const Card* card = to_select->getFilteredCard();

        return card->inherits("Weapon") && !card->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *strike = new Strike(card->getSuit(), card->getNumber());
        strike->addSubcard(card->getId());
        strike->setSkillName(objectName());
        return strike;
    }
};

class WheelMisfortune : public ZeroCardViewAsSkill
{
public:
    WheelMisfortune():ZeroCardViewAsSkill("wheelmisfortune")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MisfortuneCard");
    }

    virtual const Card* viewAs() const
    {
        return new MisfortuneCard;
    }
};

MisfortuneCard::MisfortuneCard(){
    once = true;
}

bool MisfortuneCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.isEmpty() && to_select->getMp() == to_select->getMaxMP())
        return false;

    if(!targets.isEmpty() && to_select->getMp() == 0) return false;

    return true;
}

bool MisfortuneCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void MisfortuneCard::use(Room *room, ServerPlayer *, const QList<ServerPlayer *> &targets) const{

    ServerPlayer *to = targets.at(0);
    ServerPlayer *from = targets.at(1);

    room->changeMp(to,1);
    to->jilei(NULL);
    to->invoke("jilei",NULL);

    LogMessage log;
    log.type = "#JileiClear";
    log.from = to;
    room->sendLog(log);

    room->changeMp(from,-1);
}

class BrokenAmulet  : public TriggerSkill
{
public:
    BrokenAmulet():TriggerSkill("brokenamulet")
    {
        events << CombatTargetDeclared;

        frequency = Compulsory;
    }

    virtual int getPriority() const
    {
        return 2;
    }


    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();
        Room * room = player->getRoom();

        if(!combat.to->hasSkill(objectName()))return false;
        if(combat.to->getMp()>combat.from->getMp())return false;
        if(combat.from->getMp()<1)
        {
            LogMessage log;
            log.type = "#InsufficientMp";
            log.from = combat.from;
            log.arg  = objectName();
            room->sendLog(log);

            return true;
        }
        room->changeMp(combat.from,-1);
        return false;
    }
};

class Belief: public PhaseChangeSkill
{
public:
    Belief():PhaseChangeSkill("belief")
    {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if(target->getMp() > target->getHp()
                && target->getRoom()->askForSkillInvoke(target,objectName()))

        {

            RecoverStruct recover;
            recover.who = target;

            target->getRoom()->recover(target,recover);
        }

        return false;
    }

};

OnbashiraCard::OnbashiraCard(){
    once = true;
    mute = true;
}

void OnbashiraCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    QString choice = room->askForChoice(effect.from,"onbashira","recovermp+losemp");

    int num = (choice == "recovermp") ? 1 : -1 ;
    //room->changeMp(effect.from,num);
    room->changeMp(effect.to,num);
}

class Onbashira : public ViewAsSkill
{
public:
    Onbashira():ViewAsSkill("onbashira")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return ! player->hasUsed("OnbashiraCard");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const
    {
        return selected.length()<2 && ! to_select->getCard()->isEquipped();
    }

    virtual const Card* viewAs(const QList<CardItem *> &cards) const
    {
        if(cards.length()<2)return NULL;
        OnbashiraCard * ocard = new OnbashiraCard();
        ocard->addSubcards(cards);
        ocard->setSkillName(objectName());
        return ocard;
    }
};

class IronWheel : public ZeroCardViewAsSkill
{
public:
    IronWheel():ZeroCardViewAsSkill("ironwheel")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IronWheelCard") && player->getMp()>0;
    }

    virtual const Card * viewAs() const
    {
        return new IronWheelCard;
    }
};

IronWheelCard::IronWheelCard()
{
    setObjectName("ironwheel");
    once = true;
    mute =true;
    target_fixed = true;
}

void IronWheelCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    if(source->getMp()<1)return;

    room->changeMp(source,-1);

    QList<ServerPlayer*> players;

    int min = 100;
    foreach(ServerPlayer *sp,room->getAlivePlayers())if(sp->getHandcardNum() < min)
        min = sp->getHandcardNum();

    foreach(ServerPlayer *sp,room->getAlivePlayers())if(sp->getHandcardNum()==min)
        players << sp;

    ServerPlayer* target = room->askForPlayerChosen(source,players,"ironwheel");
    room->drawCards(target,2);
}

class HatIllusion : public TriggerSkill{
public:
    HatIllusion():TriggerSkill("hatillusion")
    {
        events << Predamaged << HpLost;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getRoom()->getCardPlace(97) == Player::Equip;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(event == HpLost)
        {
            int num = data.toInt();
            if(num<player->getHp())return false;

            LogMessage log;
            log.type = "#HatIllusion";
            log.arg  = QString::number(num);
            log.arg2 = QString::number(player->getHp() - 1);
            log.from = player;
            player->getRoom()->sendLog(log);

            if(player->getHp()<2)return true;

            data=QVariant::fromValue(player->getHp() - 1);
        }

        DamageStruct damage = data.value<DamageStruct>();
        if(damage.damage >= player->getHp())
        {


            LogMessage log;
            log.type = "#HatIllusion";
            log.arg  = QString::number(damage.damage);
            log.arg2 = QString::number(player->getHp() - 1);
            log.from = player;
            player->getRoom()->sendLog(log);

            damage.damage = player->getHp() - 1;

            if(damage.damage <= 0)return true;
            data=QVariant::fromValue(damage);

        }
        return false;
    }
};

class DollMaster : public TriggerSkill
{
public:
    DollMaster():TriggerSkill("dollmaster")
    {
        events << CombatTargetDeclared;

        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) &&
                !target->getArmor();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();
        Room * room = player->getRoom();

        if(!room->askForSkillInvoke(player,objectName()))return false;

        const Card* card = room->peek();
        room->drawCards(combat.from,1);

        room->throwCard(combat.combat);
        combat.from->addToPile("Attack",card->getId(),false);
        combat.combat = card;

        player->tag["combatEffective"] = true;

        LogMessage log;
        log.from = player;
        log.type = "#DollExchange";
        player->getRoom()->sendLog(log);
        player->getRoom()->getThread()->delay();

        data = QVariant::fromValue(combat);

        return false;
    }
};

class FolkDance: public TriggerSkill{
public:
    FolkDance():TriggerSkill("folkdance"){
        events << BlockDeclared;
    }

    virtual bool triggerable(const ServerPlayer *) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CombatStruct combat = data.value<CombatStruct>();
        if(!combat.from->hasSkill(objectName()))
            return false;
        if(combat.from->getMp()<1)return false;
        if(combat.from->askForSkillInvoke(objectName(),data)){
            Room *room = combat.from->getRoom();
            room->changeMp(combat.from,-1);
            room->moveCardTo(combat.block, player, Player::Hand, false);
            player->jilei(QString(combat.block->getEffectiveId()));
            player->invoke("jilei", combat.block->getEffectIdString());
            player->setFlags("jilei_temp");

            LogMessage log;
            log.type = "#FolkDance";
            log.from = player;

            room->sendLog(log);

            const Card* block = room->askForCard(player, ".", "blockCard", true);
            combat.block = block;

            if(block){
                combat.to->addToPile("Defense", block->getEffectiveId(), false);
                player->tag["combatEffective"] = true;
                if(block->getSkillName().length()>0){
                    player->tag["Combat_Convert_From"] = block->getEffectiveId()+1;
                    player->tag["Combat_Convert_To"] = block->toString();
                }

                LogMessage log;
                log.type = "#chosenBlock";
                log.from = player;
                room->sendLog(log);
            }

            data = QVariant::fromValue(combat);
        }

        return false;
    }
};

class MasterSpark : public OneCardViewAsSkill
{
public:
    MasterSpark():OneCardViewAsSkill("masterspark")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return CombatCard::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "barrage" || pattern == ".combat" || pattern == ".";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        return card->getSuit() == Card::Spade;

    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *barrage = new Barrage(card->getSuit(), card->getNumber());
        barrage->addSubcard(card->getId());
        barrage->setSkillName(objectName());
        return barrage;
    }

};

class ThiefMarisa: public TriggerSkill{
public:
    ThiefMarisa():TriggerSkill("thiefmarisa")
    {
        events << CombatFinished << TargetFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target)
                && target->getMp();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();

        Room * room = combat.from->getRoom();
        const Card * card = event == CombatFinished ? combat.block : combat.combat;
        if(!card)return false;
        if(room->getCardPlace(card->getId())!=Player::DiscardedPile)return false;
        if(!room->obtainable(card,player))return false;
        if(!player->getMp())return false;
        if(!room->askForSkillInvoke(player,objectName()))return false;

        room->changeMp(player,-1);
        player->obtainCard(card);
        return false;
    }


};

class LunaticRedEyes : public TriggerSkill
{
public:
    LunaticRedEyes():TriggerSkill("lunaticredeyes")
    {
        events << CombatFinish << TargetFinish;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMp()>1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();

        Room * room = player->getRoom();

        if(player->getMp()<2)return false;
        if(!room->askForSkillInvoke(player,objectName()))return false;

        room->changeMp(player,-2);
        const Card* card = combat.block;
        combat.block = combat.combat;
        combat.combat = card ;

        LogMessage log;
        log.type = "#LunaticExchange";
        log.from = player;
        room->sendLog(log);

        data = QVariant::fromValue(combat);

        return false;
    }

};

class IdlingWave : public MasochismSkill
{
public:
    IdlingWave():MasochismSkill("idlingwave")
    {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room * room = target->getRoom();
        if(!room->askForSkillInvoke(target,objectName()))return;

        room->changeMp(target,1);
        if(damage.from)room->setPlayerFlag(room->getCurrent(),"IdlingWave");

        LogMessage log;
        log.type = "#idlingwave";
        log.from = target;
        log.to   << room->getCurrent();
        room->sendLog(log);
    }
};

class GodsBanquet : public PhaseChangeSkill
{
public:
    GodsBanquet():PhaseChangeSkill("godsbanquet")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if(target->getPhase() != Player::Draw)return false;
        Room * room = target->getRoom();

        if(!room->askForSkillInvoke(target,objectName()))return false;

        foreach(ServerPlayer* sp,room->getOtherPlayers(target))
        {
            if(sp->getHandcardNum()>0)
            {
                const Card* card = sp->getRandomHandCard();
                room->moveCardTo(card,target,Player::Hand,false);
            }
        }

        target->skip(Player::Play);
        return true;
    }
};

UltimateBuddhistCard::UltimateBuddhistCard()
{

}


void UltimateBuddhistCard::onEffect(const CardEffectStruct &effect) const
{
    if(effect.from->getMp()<1)return;
    effect.from->getRoom()->changeMp(effect.from,-1);

    effect.to->obtainCard(this);
    RecoverStruct recover;
    recover.card= this;
    recover.who = effect.to;
    effect.to->getRoom()->recover(effect.to,recover);
}

class UltimateBuddhist: public OneCardViewAsSkill{
public:
    UltimateBuddhist():OneCardViewAsSkill("ultimatebuddhist"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("peach")
                && player->getPhase() == Player::NotActive
                && player->getMp()>0;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->getSuit() == Card::Club
                && !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        UltimateBuddhistCard *peach = new UltimateBuddhistCard;
        peach->addSubcard(first->getId());
        return peach;
    }
};

class Phantasm: public DistanceSkill{
public:
    Phantasm():DistanceSkill("phantasm"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(from->hasSkill(objectName()) && to->isChained())
            return -10;
        else
            return 0;
    }
};


class NinetailViewas : public OneCardViewAsSkill
{
public:
    NinetailViewas():OneCardViewAsSkill("ninetail")
    {

    }

    virtual bool viewFilter(const CardItem *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const
    {
        Card* card = new NinetailCard;
        card->addSubcard(card_item->getCard());
        return card;
    }
};

NinetailCard::NinetailCard()
{
    setObjectName("ninetail");

    will_throw = true;
    target_fixed=true;
}

void NinetailCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const
{
    source->getRoom()->changeMp(source,1);
    room->throwCard(this);
}

class Ninetail : public TriggerSkill
{
public:
    Ninetail():TriggerSkill("ninetail")
    {
        view_as_skill = new NinetailViewas;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        return false;
    }
};

MindreaderCard::MindreaderCard()
{
}

void MindreaderCard::onEffect(const CardEffectStruct &effect) const
{
    int cid=-1;
    Room* room=effect.to->getRoom();


    if(!room->changeMp(effect.from,-1))
    {
        room->logNoMp(effect.from,objectName());
        return;
    }


    room->drawCards(effect.to,2);
    QList<int> hand=effect.to->handCards();

    if(!hand.isEmpty())
    {
        room->fillAG(hand,effect.from);
        cid=room->askForAG(effect.from,hand,true,"mindreader");
    }

    if(cid>=0)
    {
        hand.removeOne(cid);
        room->moveCardTo(Sanguosha->getCard(cid),effect.from,Player::Hand,false);
        effect.from->invoke("clearAG");
        if(!hand.isEmpty())
        {
            room->fillAG(hand,effect.from);

            cid=room->askForAG(effect.from,hand,true,"mindreader");

            if(cid>=0)
            {
                room->moveCardTo(Sanguosha->getCard(cid),effect.from,Player::Hand,false);
                hand.removeOne(cid);
            }



        }
    }

    effect.from->invoke("clearAG");


}

class MindreaderViewas: public ZeroCardViewAsSkill
{
public:
    MindreaderViewas():ZeroCardViewAsSkill("mindreader")
    {

    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "@@mindreader";
    }


    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }

    virtual const Card* viewAs() const
    {
        return new MindreaderCard;
    }
};

class Mindreader : public PhaseChangeSkill
{
public:
    Mindreader():PhaseChangeSkill("mindreader")
    {
        view_as_skill = new MindreaderViewas;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if(target->getPhase() != Player::Draw)return false;
        if(!target->getMp())return false;

        Room * room = target->getRoom();
        if(!room->askForUseCard(target,"@@mindreader","@mindreader"))return false;
        return true;
    }
};

class FSViewas : public OneCardViewAsSkill
{
public:
    FSViewas():OneCardViewAsSkill("fs_viewas")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("springflower_limited");
    }

    virtual bool viewFilter(const CardItem *to_select) const
    {
        return to_select->getFilteredCard()->isBlack();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Dannatu *duel = new Dannatu(card->getSuit(), card->getNumber());
        duel->addSubcard(card);
        duel->setSkillName(objectName());
        return duel;
    }
};

class FantasySpringflower : public PhaseChangeSkill
{
public:
    FantasySpringflower():PhaseChangeSkill("fantasyspringflower")
    {
        view_as_skill = new FSViewas;

        frequency = Limited;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if(target->getPhase()==Player::NotActive
                && target->getMark("springflower_limited"))
        {
            target->setMark("springflower_limited",0);
            return false;
        }

        if(target->getPhase()!=Player::Start)return false;
        if(!target->getMark("@springflower"))return false;
        if(target->getMp()<2)return false;
        if(!target->askForSkillInvoke(objectName()))return false;

        target->getRoom()->changeMp(target,-2);
        target->loseMark("@springflower");

        target->drawCards(qMax(0,target->getHp() - target->getHandcardNum()));
        target->getRoom()->setPlayerMark(target,"springflower_limited",1);

        return false;
    }
};

class FSViewasProhibit : public ProhibitSkill
{
public:
    FSViewasProhibit():ProhibitSkill("#fs_viewas_prohibit")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const
    {
        return card->getSkillName()=="fs_viewas"
                && from->distanceTo(to)>1;
    }

    virtual bool isGlobal() const
    {
        return true;
    }
};

class Kachoufuugetu : public TriggerSkill
{
public:
    Kachoufuugetu():TriggerSkill("kachoufuugetu")
    {
        events << CombatFinished;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(!player->getMp())return false;
        if(!player->askForSkillInvoke(objectName()))return false;

        player->getRoom()->changeMp(player,-1);

        CombatStruct combat = data.value<CombatStruct>();
        GuifuCard::ApplyChain(objectName(),combat.to,player);
        return false;
    }
};

class KachoufuugetuConstraint : public ConstraintSkill
{
public:
    KachoufuugetuConstraint():ConstraintSkill("kachoufuugetu")
    {
        events << BlockDeclare << CardLost;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(event == CardLost)
        {
            CardMoveStar cms = data.value<CardMoveStar>();
            if(cms->to == cms->from)return false;

            player->getRoom()->detachSkillFromPlayer(player,"kachoufuugetu_viewas");
            return false;
        }

        player->getRoom()->acquireSkill(player,"kachoufuugetu_viewas",false);
        return false;
    }
};

class KachoufuugetuDetacher : public DetacherSkill
{
public:
    KachoufuugetuDetacher():DetacherSkill("kachoufuugetu")
    {
        events.clear();
        events<< Damage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        DetacherSkill::Detach(player,this);
        return false;
    }
};

class KachoufuugetuViewas : public FilterSkill
{
public:
    KachoufuugetuViewas():FilterSkill("kachoufuugetu_viewas")
    {

    }

    virtual bool viewFilter(const CardItem *to_select) const
    {
        return to_select->getCard()->inherits("Barrage");
    }

    virtual const Card *viewAs(CardItem *card_item) const
    {
        Peach *card= new Peach(card_item->getCard()->getSuit(),card_item->getCard()->getNumber());
        card->addSubcard(card_item->getCard());
        card->setSkillName(objectName());
        return card;
    }

};

class FantasyFlier : public DistanceSkill
{
public:
    FantasyFlier():DistanceSkill("fantasyflier")
    {

    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        if(!from->getMark("fantasyflier_enabled"))return 0;
        if(!to->getMark("fantasyflier_enabled"))return 0;

        int temp = qAbs(from->getSeat() - from->getMark("fantasyflier"));
        int distF =qMin( temp, from->aliveCount() - temp);

        temp = qAbs(to->getSeat() - to->getMark("fantasyflier"));
        int distT =qMin(temp, to->aliveCount() - temp);

        if(distT + distF == from->rawDistanceTo(to,false)) return -1;

        return 0;
    }
};

class FantasyFlierMarkassigner : public TriggerSkill
{
public:
    FantasyFlierMarkassigner():TriggerSkill("#ff_markassigner")
    {
        events << GameStart << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        ServerPlayer * nue = player->getRoom()->findPlayerBySkillName(objectName());
        if(!nue)return false;

        foreach(ServerPlayer * sp, player->getRoom()->getOtherPlayers(nue))
        {
            nue->getRoom()->setPlayerMark(sp,"fantasyflier_enabled",1);
            nue->getRoom()->setPlayerMark(sp,"fantasyflier",nue->getSeat());
        }


        return false;
    }
};

class UFO : public TriggerSkill
{
public:
    UFO():TriggerSkill("ufo")
    {
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        const Card * card = effect.card;
        if(!(card->inherits("Snatch")||card->inherits("Dismantlement")))return false;
        if(!player->getMp())return false;
        if(!player->askForSkillInvoke(objectName()))return false;

        player->getRoom()->changeMp(player,-1);
        int self_chosen = player->getRoom()->askForCardChosen(player,player,"hej","ufo");
        player->getRoom()->provide(Sanguosha->getCard(self_chosen));

        return false;
    }
};



void TouhouPackage::addGenerals()
{
    //General *tester = new General(this,"tester","_hrp");
    //tester->addSkill(new KachoufuugetuConstraint);

    General *reimu = new General(this,"reimu","_hrp", 3, false ,false, 4);
    reimu->addSkill(new GuifuViewAs);
    reimu->addSkill(new Musoutensei);
    //reimu->addSkill(new Musoufuuin);

    General *aya =new General(this,"aya","_hrp",4,false,false,3);
    aya->addSkill(new WindGirl);
    aya->addSkill(new Fastshot);

    General * alice = new General(this,"alice","_hrp",3,false,false,3);
    alice->addSkill(new DollMaster);
    alice->addSkill(new FolkDance);

    General * marisa = new General(this,"marisa","_hrp",4,false,false,3);
    marisa->addSkill(new MasterSpark);
    marisa->addSkill(new ThiefMarisa);

    General *chiruno = new General(this,"chiruno","_esd", 9 , false , false , 1);
    chiruno->addSkill(new Baka);
    chiruno->addSkill(new PerfectFreeze);

    General *remilia = new General(this,"remilia","_esd",3,false,false,4);
    remilia->addSkill(new NightlessCity);
    remilia->addSkill(new ImperishableHeart);
    remilia->addSkill(new RemiliaStalker);

    General *sakuya = new General(this,"sakuya","_esd",4,false,false,2);
    sakuya->addSkill(new PerfectMaid);
    sakuya->addSkill(new DeflatedWorld);

    General *flandre = new General(this,"flandre","_esd",3,false,false,4);
    flandre->addSkill(new FourNineFive);
    flandre->addSkill(new Daremoinai);

    General *pachuli = new General(this,"pachuli","_esd",3,false,false,5);
    pachuli->addSkill(new PhilosopherStone);
    pachuli->addSkill(new PhilosopherStonePhaseChange);

    General *yuyuko = new General(this,"yuyuko","_pcb",4,false,false,3);
    //yuyuko->addSkill(new Deathlure);
    yuyuko->addSkill(new AllLost);
    yuyuko->addSkill(new RealSumiSakura);

    General *youmu = new General(this,"youmu","_pcb",4,false,false,3);
    youmu->addSkill(new DoubleSwordMaster);

    General * yukari =new General(this,"yukari","_pcb",3,false,false,4);
    yukari->addSkill(new RealmController);
    yukari->addSkill(new BlackButterfly);
    yukari->addSkill(new Kamikakushi);

    General * ran = new General(this,"ran","_pcb",3,false,false,4);
    ran->addSkill(new Phantasm);
    ran->addSkill(new Ninetail);
    ran->addSkill(new UltimateBuddhist);

    General *mokou = new General(this,"mokou","_in",4,false,false,3);
    mokou->addSkill(new PhoenixTail);
    mokou->addSkill(new PhoenixSoar);

    General *eirin = new General(this,"eirin","_in",4,false,false,4);
    eirin->addSkill(new Skill("sharpmind"));
    eirin->addSkill(new WorldJar);
    eirin->addSkill(new WorldJarProhibit);
    eirin->addSkill(new Arcanum);

    General *kaguya = new General(this,"kaguya","_in",4,false,false);
    kaguya->addSkill(new NightReturn);
    kaguya->addSkill(new FiveProblem);

    General * reisen = new General(this,"reisen","_in",3,false,false,4);
    reisen->addSkill(new IdlingWave);
    reisen->addSkill(new LunaticRedEyes);

    General *sanai = new General(this,"sanai","_mof",4,false,false,4);
    sanai->addSkill(new FuujinSaishi);
    sanai->addSkill(new MosesMiracle);

    General * nitori = new General(this,"nitori","_mof",3,false,false,4);
    nitori->addSkill(new OpticalCamo);
    nitori->addSkill(new ExtendedArm);

    General * hina =  new General(this,"hina","_mof",4,false,false,2);
    hina->addSkill(new WheelMisfortune);
    hina->addSkill(new BrokenAmulet);

    General * kanako = new General(this,"kanako","_mof",4,false,false,3);
    kanako->addSkill(new Belief);
    kanako->addSkill(new Onbashira);

    General * suwako = new General(this,"suwako","_mof",3,false,false,3);
    suwako->addSkill(new IronWheel);
    suwako->addSkill("belief");
    suwako->addSkill(new HatIllusion);

    General *tenshi = new General(this,"tenshi","_swr",4,false,false,2);
    tenshi->addSkill(new Munenmusou);
    tenshi->addSkill(new GuardianKanameishi);

    General *utuho = new General(this,"utuho","_sa",4,false,false,3);
    utuho->addSkill(new SubterraneanSun);
    utuho->addSkill(new MegaFlare);

    General * rin = new General(this,"rin","_sa",3,false,false,4);
    rin->addSkill(new FireWheel);
    rin->addSkill(new Catwalk);

    General * koishi = new General(this,"koishi","_sa",3,false,false,4);
    koishi->addSkill(new OstinateStone);
    koishi->addSkill(new Unconsciousness);

    General * satori = new General(this,"satori","_sa",4,false,false,3);
    satori->addSkill(new Mindreader);

    General *kogasa = new General(this,"kogasa","_ufo",3,false,false,4);//--now for test
    kogasa->addSkill(new UmbrellaIllusion);
    kogasa->addSkill(new Forgotten);
    kogasa->addSkill(new ForgottenProhibit);
    kogasa->addSkill(new Scare);

    General *yuuka = new General(this,"yuuka","_pfv",4,false,false,3);
    yuuka->addSkill(new FantasySpringflower);
    yuuka->addSkill(new FSViewasProhibit);
    yuuka->addSkill(new MarkAssignSkill("@springflower", 1));

    yuuka->addSkill(new Kachoufuugetu);


    General *nue = new General(this,"nue","_ufo",4,false,false,3);
    nue->addSkill(new FantasyFlier);
    nue->addSkill(new FantasyFlierMarkassigner);
    nue->addSkill(new UFO);


    skills << new GuifuDetacher << new GuifuConstraint << new PhilosopherStoneDetacher << new PhilosopherStoneConstraint;
    skills << new WorldJarDetacher << new Skill("worldjar_constraint");
    skills << new BlackButterflyConstraint << new BlackButterflyDetacher;
    skills << new ForgottenDetacher << new Skill("forgotten_constraint");
    skills << new KachoufuugetuConstraint << new KachoufuugetuDetacher << new KachoufuugetuViewas;

    skills << new SwitchMode;
    addMetaObject<SwitchModeCard>();

    addMetaObject<GuifuCard>();
    addMetaObject<PerfectFreezeCard>();
    addMetaObject<PhoenixSoarCard>();
    addMetaObject<DeathlureCard>();
    addMetaObject<FuujinSaishiCard>();
    addMetaObject<MosesMiracleCard>();
    addMetaObject<DaremoinaiCard>();
    addMetaObject<PhilosopherStoneCard>();
    addMetaObject<WorldJarCard>();
    addMetaObject<RealSumiSakuraCard>();
    addMetaObject<FiveProblemCard>();
    addMetaObject<MisfortuneCard>();
    addMetaObject<OnbashiraCard>();
    addMetaObject<IronWheelCard>();
    addMetaObject<NinetailCard>();
    addMetaObject<MindreaderCard>();
    addMetaObject<UltimateBuddhistCard>();
}

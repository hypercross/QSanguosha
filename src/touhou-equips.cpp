#include "touhou-equips.h"

Hakkero::Hakkero(Suit suit, int number)
    :Weapon(suit, number, 1)
{
    setObjectName("hakkero");
}

TenguFan::TenguFan(Card::Suit suit, int number)
    :Weapon(suit, number, 1)
{
    setObjectName("tengu_fan");
}

class HisonokenSkill : public WeaponSkill
{
public:
    HisonokenSkill():WeaponSkill("hisonoken")
    {
        events << CombatReveal;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatRevealStruct reveal = data.value<CombatRevealStruct>();
        if(!reveal.attacker)return false;

        Room * room = player->getRoom();

        QList<ServerPlayer *> to;
        foreach(ServerPlayer* aplayer, reveal.opponets)
            if(aplayer->getPile("Defense").length() > 0) to << aplayer ;

        if(to.length()<1 || !room -> askForSkillInvoke(player,objectName()))return false;

        ServerPlayer * target = room->askForPlayerChosen(player,to,objectName());

        int cid = target->getPile("Defense").first();
        room ->showCard(target,cid,player);

        const Card* newCombat = room -> askForCard(player,".combat","hisonoken-combat",false);
        if(!newCombat)return false;

        room->throwCard(player->getPile("Attack").first());
        player->addToPile("Attack",newCombat->getEffectiveId(),false);

        reveal.revealed = newCombat;
        data = QVariant::fromValue(reveal);

        return false;
    }
};

Hisonoken::Hisonoken(Card::Suit suit, int number)
    :Weapon(suit, number, 1)
{
    setObjectName("hisonoken");
    skill = new HisonokenSkill;
}

class GungnirSkill : public WeaponSkill
{
public:
    GungnirSkill():WeaponSkill("gungnir")
    {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        JudgeStar judge = data.value<JudgeStar>();

        player->tag["Judge"] = data;
        if(!room->askForSkillInvoke(player,objectName()))return false;



            room->throwCard(judge->card);

            judge->card = player->getWeapon();
            room->moveCardTo(judge->card, NULL, Player::Special);

            LogMessage log;
            log.type = "$ChangedJudge";
            log.from = player;
            log.to << judge->who;
            log.card_str = judge->card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);


        return false;
    }
};

Gungnir::Gungnir(Card::Suit suit, int number)
    :Weapon(suit,number,3)
{
    setObjectName("gungnir");
    skill = new GungnirSkill;
}

class MagicBookSkill : public WeaponSkill
{
public:
    MagicBookSkill():WeaponSkill("magic_book")
    {
        events << CombatRevealed << Predamage << CombatFinished;

    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        if(event == CombatRevealed)
        {
            CombatRevealStruct reveal = data.value<CombatRevealStruct>();
            if(!reveal.attacker || !reveal.revealed->inherits("Barrage"))return false;

            Room * room = player->getRoom();

            while( room->askForCard(player,"barrage","magic-book-barrage"))player->addMark("MagicBookCount");
            return false;
        }

        if(event == CombatFinished)
        {
            player->setMark("MagicBookCount",0);
            return false;
        }

        if(event == Predamage)
        {
            DamageStruct dmg = data.value<DamageStruct>();

            if(dmg.card == NULL || !dmg.card->inherits("Barrage"))return false;
            int marks = player->getMark("MagicBookCount");
            if(marks < 1)return false;

            LogMessage log;
            log.type = "#MagicBookBuff";
            log.from = player ;
            log.to << dmg.to;
            log.arg  = QString::number(dmg.damage);
            log.arg2 = QString::number(dmg.damage + marks);

            dmg.damage += marks;
            data = QVariant::fromValue(dmg);
            return false;
        }

        return false;
    }
};

MagicBook::MagicBook(Card::Suit suit, int number)
    :Weapon(suit,number,2)
{
    setObjectName("magic_book");
    skill = new MagicBookSkill;
}

class UmbrellaSkill: public WeaponSkill{
public:
    UmbrellaSkill():WeaponSkill("umbrella"){
        events << CombatTargetDeclare;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        CombatStruct effect = data.value<CombatStruct>();
        Room *room = player->getRoom();

        if(effect.to->isChained()){
            if(effect.from->askForSkillInvoke(objectName())){
                bool draw_card = false;

                if(effect.to->isKongcheng())
                    draw_card = true;
                else{
                    QString prompt = "umbrella-card:" + effect.from->getGeneralName();
                    const Card *card = room->askForCard(effect.to, ".", prompt);
                    if(card){
                        room->throwCard(card);
                    }else
                        draw_card = true;
                }

                if(draw_card)
                    effect.from->drawCards(1);
            }
        }

        return false;
    }
};

Umbrella::Umbrella(Card::Suit suit, int number)
    :Weapon(suit,number,2)
{
    setObjectName("umbrella");
    skill = new UmbrellaSkill;
}

class PadSkill : public ArmorSkill
{
public:
    PadSkill():ArmorSkill("pad")
    {
        events << TargetFinish;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();
        Room *room = player->getRoom();

        if(combat.block->inherits("DummyCard"))return false;
        if(!room->askForSkillInvoke(player,objectName()))return false;

        if(!combat.block->inherits("CombatCard"))
        {
            Barrage *barrage = new Barrage(combat.block->getSuit(),combat.block->getNumber());
            barrage->setSkillName(objectName());
            barrage->addSubcard(combat.block);
            combat.block = barrage;

            LogMessage log;
            log.type = "#PadConvert";
            log.from = combat.to ;
            log.card_str = barrage->toString();
            room->sendLog(log);

            data =QVariant::fromValue(combat);
        }

        return false;
    }
};

Pad::Pad(Card::Suit suit, int number)
    :Armor(suit,number)
{
    setObjectName("pad");
    skill = new PadSkill;
}

class DollSkill : public ArmorSkill
{
public:
    DollSkill():ArmorSkill("doll")
    {
        events << TargetFinish;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const
    {
        CombatStruct combat = data.value<CombatStruct>();

        if(combat.combat->inherits("Strike"))
        {
            LogMessage log;
            log.from = player;
            log.type = "#DollNullify";
            player->getRoom()->sendLog(log);

            return true;
        }
        return false;
    }
};

Doll::Doll(Card::Suit suit, int number)
    :Armor(suit,number)
{
    setObjectName("doll");
    skill = new DollSkill;
}

void TouhouPackage::addEquips()
{
    QList<Card*> cards;

    cards << new Hisonoken(Card::Club,2)
          << new Gungnir(Card::Heart,3)
          << new MagicBook(Card::Spade,2)
          << new Umbrella(Card::Diamond,2)
          << new Pad(Card::Club,4)
          << new Doll(Card::Club,5);

    foreach(Card *card, cards)
        card->setParent(this);
}

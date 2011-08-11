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


void TouhouPackage::addEquips()
{
    QList<Card*> cards;

    cards << new Hisonoken(Card::Club,2)
          << new Gungnir(Card::Heart,3);

    foreach(Card *card, cards)
        card->setParent(this);
}

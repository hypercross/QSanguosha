#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "pixmap.h"
#include "carditem.h"
#include "player.h"
#include "skill.h"

#include <QPushButton>
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QLineEdit>
#include <QProgressBar>

class Dashboard : public Pixmap
{
    Q_OBJECT

public:
    Dashboard();
    virtual QRectF boundingRect() const;
    void setWidth(int width);
    QGraphicsProxyWidget *addWidget(QWidget *widget, int x, bool from_left);
    QPushButton *createButton(const QString &name);
    QPushButton *addButton(const QString &name, int x, bool from_left);
    QProgressBar *addProgressBar();

    void setTrust(bool trust);
    void addCardItem(CardItem *card_item);
    CardItem *takeCardItem(int card_id, Player::Place place);
    void setPlayer(const ClientPlayer *player);
    Pixmap *getAvatar();
    void selectCard(const QString &pattern, bool forward = true);
    void useSelected();
    const Card *getSelected() const;
    void unselectAll();
    void hideAvatar();
    void setFilter(const FilterSkill *filter);
    const FilterSkill *getFilter() const;

    void disableAllCards();
    void enableCards();
    void enableAllCards();

    void installEquip(CardItem *equip);
    void installDelayedTrick(CardItem *card);

    // pending operations
    void startPending(const ViewAsSkill *skill);
    void stopPending();
    void updatePending();
    const ViewAsSkill *currentSkill() const;
    const Card *pendingCard() const;

    void killPlayer();
    void revivePlayer();

public slots:
    void updateAvatar();
    void updateSmallAvatar();
    void refresh();
    void sortCards(int sort_type);
    void reverseSelection();

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    QPixmap left_pixmap, right_pixmap;
    QGraphicsRectItem *left, *middle, *right;
    int min_width;

    QList<CardItem*> card_items;
    CardItem *selected;
    Pixmap *avatar, *small_avatar;
    QGraphicsPixmapItem *kingdom;
    QGraphicsTextItem *mark_item;
    QGraphicsPixmapItem *action_item;

    int sort_type;
    QGraphicsSimpleTextItem *handcard_num;
    QList<CardItem *> judging_area;
    QList<QPixmap> delayed_tricks;
    QGraphicsPixmapItem *death_item;
    Pixmap *chain_icon, *back_icon, *slow_icon;

    QGraphicsRectItem *equip_rects[4];
    CardItem *weapon, *armor, *defensive_horse, *offensive_horse;
    QList<CardItem **> equips;

    QGraphicsRectItem *trusting_item;
    QGraphicsSimpleTextItem *trusting_text;

    // for parts creation
    void createLeft();
    void createRight();
    void createMiddle();
    void setMiddleWidth(int middle_width);

    // for pendings
    QList<CardItem *> pendings;
    const Card *pending_card;
    const ViewAsSkill *view_as_skill;
    const FilterSkill *filter;

    void adjustCards();
    void adjustCards(const QList<CardItem *> &list, int y);
    void drawEquip(QPainter *painter, const CardItem *equip, int order);
    void setSelectedItem(CardItem *card_item);
    void drawHp(QPainter *painter) const;

private slots:
    void onCardItemClicked();
    void onCardItemThrown();
    void onMarkChanged();
    void setActionState();

signals:
    void card_selected(const Card *card);
    void card_to_use();
};

#endif // DASHBOARD_H

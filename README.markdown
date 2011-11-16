Open Source Sanguosha - the Touhou MOD 
==========

Introduction
----------

Sanguosha is both a popular board game and online game,
this project try to clone the Sanguosha online version.
The whole project is written in C++, 
using Qt's graphics view framework as the game engine.
I've tried many other open source game engines, 
such as SDL, HGE, Clanlib and others, 
but many of them lack some important features. 
Although Qt is an application framework instead of a game engine, 
its graphics view framework is suitable for my game developing.

This is a modification of the original game and the 
theme is set to the touhou world. All game cards are re-made 
, new systems and characters are introduced.

The gameplay is my design, image & audio is from pixiv and 
everywhere on the web, or possibly the remainder of the

Features
----------

1. Framework
    * Open source with Qt graphics view framework
    * Use irrKlang as sound engine
    * Use plib as joystick backend 
    * Use Lua as AI script

2. Operation experience
    * Full package (include all yoka extension package)
    * Drag card to target to use card
    * Keyboard shortcut
    * Cards sorting (by card type and card suit)
    * Multilayer display when cards are more than an upperlimit

3. Extensible
    * Some MODs are available based on this game


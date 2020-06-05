/* NetHack 3.6	spell.h	$NHDT-Date: 1432512780 2015/05/25 00:13:00 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright 1986, M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SPELL_H
#define SPELL_H

#define NO_SPELL 0

/* spellbook re-use control; used when reading and when polymorphing */
#define MAX_SPELL_STUDY 3

struct spell {
    short sp_id;  /* spell id (== object.otyp) */
    xchar sp_lev; /* power level */
    int sp_know;  /* knowlege of spell */
};

/* levels of memory destruction with a scroll of amnesia */
#define ALL_MAP 0x1
#define ALL_SPELLS 0x2

#define decrnknow(spell) g.spl_book[spell].sp_know--
#define spellid(spell) g.spl_book[spell].sp_id
#define spellknow(spell) g.spl_book[spell].sp_know

#define ZT_MAGIC_MISSILE (AD_MAGM - 1)
#define ZT_FIRE (AD_FIRE - 1)
#define ZT_COLD (AD_COLD - 1)
#define ZT_SLEEP (AD_SLEE - 1)
#define ZT_DEATH (AD_DISN - 1) /* or disintegration */
#define ZT_LIGHTNING (AD_ELEC - 1)
#define ZT_POISON_GAS (AD_DRST - 1)
#define ZT_ACID (AD_ACID - 1)
#define ZT_SONIC (AD_LOUD - 1)
#define ZT_PSYCHIC (AD_PSYC - 1)

#define ZT_LAST                 (ZT_ACID) /*For checking of spells of a type*/
#define ZT_FIRST                (ZT_MAGIC_MISSILE)
/* 8 and 9 are now assigned to sonic and psychic */

#define ZT_WAND(x) (x)
#define ZT_SPELL(x) (10 + (x))
#define ZT_BREATH(x) (20 + (x))
#define ZT_MEGA(x)   (30+(x))

/* Vanilla magic spells */
#define MGC_PSI_BOLT 0
#define MGC_CURE_SELF 1
#define MGC_HASTE_SELF 2
#define MGC_STUN_YOU 3
#define MGC_DISAPPEAR 4
#define MGC_WEAKEN_YOU 5
#define MGC_DESTRY_ARMR 6
#define MGC_CURSE_ITEMS 7
#define MGC_AGGRAVATION 8
#define MGC_SUMMON_MONS 9
#define MGC_CLONE_WIZ 10
#define MGC_DEATH_TOUCH 11
/* Splice magic spells */
#define MGC_SONIC_SCREAM 12
#define MGC_GAS_CLOUD 13
#define MGC_LAST MGC_GAS_CLOUD

/* Vanilla clerical spells */
#define CLC_OPEN_WOUNDS 0
#define CLC_CURE_SELF 1
#define CLC_CONFUSE_YOU 2
#define CLC_PARALYZE 3
#define CLC_BLIND_YOU 4
#define CLC_INSECTS 5
#define CLC_CURSE_ITEMS 6
#define CLC_LIGHTNING 7
#define CLC_FIRE_PILLAR 8
#define CLC_GEYSER 9
#define CLC_LAST CLC_GEYSER

#endif /* SPELL_H */

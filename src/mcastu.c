/* NetHack 3.6	mcastu.c	$NHDT-Date: 1590904092 2020/05/31 05:48:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.67 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern void demonpet();

static const struct spellset {
    int spell_id;
    int min_level;
} mage_spells[] = {
        { SPE_PSI_BOLT, 1 },
        { SPE_HEALING, 2 },
        { SPE_HASTE_SELF, 3 },
        { SPE_STUN, 4 },
        { SPE_INVISIBILITY, 5 },
        { SPE_DRAIN_STRENGTH, 7 },
        { SPE_DESTROY_ARMOR, 9 },
        { SPE_CURSE_ITEMS, 11 },
        { SPE_AGGRAVATION, 14 },
        { SPE_SUMMON_NASTIES, 16 },
        { SPE_FINGER_OF_DEATH, 21 },
        { 0, 0 }
    },
    cleric_spells[] = {
        { SPE_OPEN_WOUNDS, 1 },
        { SPE_PROTECTION, 1 },
        { SPE_HEALING, 2 },
        { SPE_CONFUSE_MONSTER, 3 },
        { SPE_PARALYZE, 5 },
        { SPE_BLINDNESS, 6 },
        { SPE_INSECT_SWARM, 9 },
        { SPE_CURSE_ITEMS, 10 },
        { SPE_LIGHTNING, 12 },
        { SPE_FLAME_PILLAR, 13 },
        { SPE_GEYSER, 14 },
        { 0, 0 }
    },
    rodney_spells[] = {
        { SPE_SONICBOOM, 14 },
        { SPE_DOUBLE_TROUBLE, 19 },
        { SPE_TURN_UNDEAD, 19 },
        { 0, 0 }
    },
    undead_mage_spells[] = {
        { SPE_PSI_BOLT, 1 },
        { SPE_FREEZE_SPHERE, 2 },
        { SPE_HEALING, 2 },
        { SPE_HASTE_SELF, 3 },
        { SPE_WEB, 3 },
        { SPE_INVISIBILITY, 5 },
        { SPE_DRAIN_STRENGTH, 7 },
        { SPE_DESTROY_ARMOR, 9 },
        { SPE_CURSE_ITEMS, 11 },
        { SPE_TURN_UNDEAD, 11 },
        { SPE_SUMMON_NASTIES, 16 },
        { SPE_EXTRA_HEALING, 18 },
        { SPE_FINGER_OF_DEATH, 21 },
        { 0, 0 }
    },
    high_priest_spells[] = {
        { SPE_OPEN_WOUNDS, 1 },
        { SPE_FORCE_BOLT, 1 },
        { SPE_PARALYZE, 5 },
        { SPE_CURE_BLINDNESS, 5},
        { SPE_INSECT_SWARM, 9 },
        { SPE_EXTRA_HEALING, 10 },
        { SPE_SLOW_MONSTER, 10 },
        { SPE_TURN_UNDEAD, 11 },
        { SPE_CURE_SICKNESS, 11},
        { SPE_LIGHTNING, 12 },
        { SPE_FLAME_PILLAR, 13 },
        { SPE_GEYSER, 14 },
        { SPE_DIG, 16 },
        { 0, 0 }
    };

static int FDECL(choose_monster_spell, (struct monst *, BOOLEAN_P));
static void FDECL(m_learn_spell_list, (struct monst *, const struct spellset *, BOOLEAN_P));
static void FDECL(cursetxt, (struct monst *, BOOLEAN_P));
static int FDECL(m_cure_self, (struct monst *, int));
static int FDECL(m_extra_cure_self, (struct monst *, int));
static void FDECL(cast_monster_spell, (struct monst *, int, int, BOOLEAN_P));
static boolean FDECL(is_undirected_spell, (int));
static boolean
FDECL(spell_would_be_useless, (struct monst *, int));

#define IS_EXPERT(mtmp) ((mtmp)->m_lev > 20)

extern const char *const flash_types[]; /* from zap.c */

void
m_learn_spell(mtmp, spell_id, arcane)
struct monst* mtmp;
int spell_id;
boolean arcane;
{
    if (arcane)
		mtmp->arc_spls |= ((uint64_t)1 << spell_id);
    else
        mtmp->clr_spls |= ((uint64_t)1 << spell_id);
}

boolean
m_knows_spell(mtmp, spell_id)
struct monst *mtmp;
int spell_id;
{
    return (mtmp->arc_spls & ((uint64_t)1 << spell_id)) != 0L
        || (mtmp->clr_spls & ((uint64_t)1 << spell_id)) != 0L;
}

static int
choose_monster_spell(mtmp, arcane)
struct monst* mtmp;
boolean arcane;
{
    int tmp, tries = 0;
    do {
        tmp = SPE_DIG + rn2(SPE_BLANK_PAPER - SPE_DIG);
        if (m_knows_spell(mtmp, tmp)) {
            return tmp;
        }
        tries++;
    } while (tries < 50);
    return arcane ? SPE_PSI_BOLT : SPE_OPEN_WOUNDS;
}

static
void
m_learn_spell_list(mtmp, spells, arcane)
struct monst* mtmp;
const struct spellset* spells;
boolean arcane;
{
    while (spells && spells->spell_id && spells->min_level <= mtmp->m_lev) {
        m_learn_spell(mtmp, spells->spell_id, arcane);
        spells++;
    }
}

void
init_mon_spells(mtmp, arcane)
struct monst* mtmp;
boolean arcane;
{
    const struct spellset *spells = arcane ? mage_spells : cleric_spells;

    if (mtmp->minitspell)
        return;

    /* Default spells */
    m_learn_spell_list(mtmp, spells, arcane);
    /* Rodney spells */
    if (mtmp->data == &mons[PM_WIZARD_OF_YENDOR]) {
        m_learn_spell_list(mtmp, rodney_spells, TRUE);
    } else if (is_undead(mtmp->data) && arcane) {
        m_learn_spell_list(mtmp, undead_mage_spells, arcane);
    } else if (mtmp->data == &mons[PM_HIGH_PRIEST]) {
        m_learn_spell_list(mtmp, high_priest_spells, arcane);
    }
    mtmp->minitspell = 1;
}

/* feedback when frustrated monster couldn't cast a spell */
static
void
cursetxt(mtmp, undirected)
struct monst *mtmp;
boolean undirected;
{
    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
        const char *point_msg; /* spellcasting monsters are impolite */

        if (undirected)
            point_msg = "all around, then curses";
        else if ((Invis && !perceives(mtmp->data)
                  && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                 || is_obj_mappear(&g.youmonst, STRANGE_OBJECT)
                 || u.uundetected)
            point_msg = "and curses in your general direction";
        else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            point_msg = "and curses at your displaced image";
        else
            point_msg = "at you, then curses";
        nohands(mtmp->data) ? pline("%s looks %s.", Monnam(mtmp), point_msg)
                            : pline("%s points %s.", Monnam(mtmp), point_msg);
    } else if ((!(g.moves % 4) || !rn2(4))) {
        if (!Deaf)
            Norep("You hear a mumbled curse.");   /* Deaf-aware */
    }
}

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(mtmp, mattk, thinks_it_foundyou, foundyou)
register struct monst *mtmp;
register struct attack *mattk;
boolean thinks_it_foundyou;
boolean foundyou;
{
    int dmg, ml = mtmp->m_lev;
    int ret;
    int spellnum = 0;

    /* Three cases:
     * -- monster is attacking you.  Search for a useful spell.
     * -- monster thinks it's attacking you.  Search for a useful spell,
     *    without checking for undirected.  If the spell found is directed,
     *    it fails with cursetxt() and loss of mspec_used.
     * -- monster isn't trying to attack.  Select a spell once.  Don't keep
     *    searching; if that spell is not useful (or if it's directed),
     *    return and do something else.
     * Since most spells are directed, this means that a monster that isn't
     * attacking casts spells only a small portion of the time that an
     * attacking monster does.
     */

    if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
        int cnt = 40;

        do {
            spellnum = choose_monster_spell(mtmp, mattk->adtyp == AD_SPEL);
            /* not trying to attack?  don't allow directed spells */
            if (!thinks_it_foundyou) {
                if (!is_undirected_spell(spellnum)
                    || spell_would_be_useless(mtmp, spellnum)) {
                    if (foundyou)
                        impossible(
                       "spellcasting monster found you and doesn't know it?");
                    return 0;
                }
                break;
            }
        } while (--cnt > 0
                 && spell_would_be_useless(mtmp, spellnum));
        if (cnt == 0)
            return 0;
    }

    /* monster unable to cast spells? */
    if (mtmp->mcan || mtmp->mspec_used || !ml) {
        cursetxt(mtmp, is_undirected_spell(spellnum));
        return 0;
    }

    if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
        mtmp->mspec_used = 10 - mtmp->m_lev;
        if (mtmp->mspec_used < 2)
            mtmp->mspec_used = 2;
    }

    /* monster can cast spells, but is casting a directed spell at the
       wrong place?  If so, give a message, and return.  Do this *after*
       penalizing mspec_used. */
    if (!foundyou && thinks_it_foundyou
        && !is_undirected_spell(spellnum)) {
        pline("%s casts a spell at %s!",
              canseemon(mtmp) ? Monnam(mtmp) : "Something",
              levl[mtmp->mux][mtmp->muy].typ == WATER ? "empty water"
                                                      : "thin air");
        return 0;
    }

    nomul(0);
    if (rn2(ml * 10) < (mtmp->mconf ? 100 : 20)) { /* fumbled attack */
        if (canseemon(mtmp) && !Deaf)
            pline_The("air crackles around %s.", mon_nam(mtmp));
        return 0;
    }
    if (canspotmon(mtmp) || !is_undirected_spell(spellnum)) {
        pline("%s casts a spell%s!",
              canspotmon(mtmp) ? Monnam(mtmp) : "Something",
              is_undirected_spell(spellnum)
                  ? ""
                  : (Invis && !perceives(mtmp->data)
                     && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                        ? " at a spot near you"
                        : (Displaced
                           && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                              ? " at your displaced image"
                              : " at you");
    }

    /*
     * As these are spells, the damage is related to the level
     * of the monster casting the spell.
     */
    if (!foundyou) {
        dmg = 0;
        if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
            impossible(
              "%s casting non-hand-to-hand version of hand-to-hand spell %d?",
                       Monnam(mtmp), mattk->adtyp);
            return 0;
        }
    } else if (mattk->damd)
        dmg = d((int) ((ml / 2) + mattk->damn), (int) mattk->damd);
    else
        dmg = d((int) ((ml / 2) + 1), 6);
    if (Half_spell_damage)
        dmg = (dmg + 1) / 2;

    ret = 1;
    switch (mattk->adtyp) {
    case AD_FIRE:
        if (is_demon(mtmp->data)) {
            pline("You're enveloped in blazing pillar of hellfire!");
            if (Fire_resistance) {
                shieldeff(u.ux, u.uy);
                pline("You only partially resist the effects.");
                dmg = (dmg + 1) / 2;
            }
        } else {
            pline("You're enveloped in flames.");
            if (Fire_resistance) {
                shieldeff(u.ux, u.uy);
                pline("But you resist the effects.");
                dmg = 0;
            }
        }
        burn_away_slime();
        break;
    case AD_COLD:
        pline("You're covered in frost.");
        if (Cold_resistance) {
            shieldeff(u.ux, u.uy);
            pline("But you resist the effects.");
            dmg = 0;
        }
        break;
    case AD_PSYC:
        pline("Your mind is being attacked!");
        if (Psychic_resistance) {
            shieldeff(u.ux, u.uy);
            pline("You fend off the mental attack!");
            dmg = 0;
        }
        break;
    case AD_WIND:
        You("are blasted by wind!");
        hurtle(u.ux - mtmp->mx, u.uy - mtmp->my, dmg, TRUE);
        dmg = 0;
        break;
    case AD_MAGM:
        You("are hit by a shower of missiles!");
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            pline_The("missiles bounce off!");
            dmg = 0;
        } else
            dmg = d((int) mtmp->m_lev / 2 + 1, 6);
        break;
    case AD_LAWS:
        switch(rn2(4)) {
        case 0:
            pline("%s sues you!", Monnam(mtmp));
            stealgold(mtmp);
            dmg = 0;
            break;
        case 1:
            pline("%s hits you with an injunction!", Monnam(mtmp));
            break;
        case 2:
            pline("%s files a restraining order against you!", Monnam(mtmp));
            if (!Free_action) {
                nomul(-rnd(5));
                g.multi_reason = "restrained via restraining order";
                g.nomovemsg = You_can_move_again;
            } else {
                shieldeff(u.ux, u.uy);
                You("weasel your way out of it!");
            }
            dmg = 0;
            break;
        default:
            pline("%s slaps you with a slap suit!", Monnam(mtmp));
            make_stunned((HStun & TIMEOUT) + (long) rn2(10), TRUE);
            break;
        }
        break;
    case AD_SPEL: /* wizard spell */
    case AD_CLRC: /* clerical spell */
    {
        cast_monster_spell(mtmp, dmg, spellnum, mattk->adtyp == AD_SPEL);
        dmg = 0; /* done by the spell casting functions */
        break;
    }
    }
    if (dmg)
        mdamageu(mtmp, dmg);
    return ret;
}

static int
m_cure_self(mtmp, dmg)
struct monst *mtmp;
int dmg;
{
    if (mtmp->mhp < mtmp->mhpmax) {
        if (canseemon(mtmp))
            pline("%s looks better.", Monnam(mtmp));
        /* note: player healing does 6d4; this used to do 1d8 */
        if ((mtmp->mhp += d(3, 6)) > mtmp->mhpmax)
            mtmp->mhp = mtmp->mhpmax;
        dmg = 0;
    }
    return dmg;
}

static int
m_extra_cure_self(mtmp, dmg)
struct monst *mtmp;
int dmg;
{
    if (mtmp->mhp < mtmp->mhpmax) {
        if (canseemon(mtmp))
            pline("%s looks much better.", Monnam(mtmp));
        /* note: player healing does 6d4; this used to do 1d8 */
        if ((mtmp->mhp += d(6, 6)) > mtmp->mhpmax)
            mtmp->mhp = mtmp->mhpmax;
        dmg = 0;
    }
    if (!mtmp->mcansee) {
        mtmp->mcansee = 1;
    }
    return dmg;
}

/* monster wizard and cleric spellcasting function */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
static
void
cast_monster_spell(mtmp, dmg, spellnum, arcane)
struct monst *mtmp;
int dmg;
int spellnum;
boolean arcane;
{
    register struct trap *trtmp;
    struct obj* pseudo;
    int n, cx, cy;

    if (arcane && dmg == 0 && !is_undirected_spell(spellnum)) {
        impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
        return;
    } else if (!arcane && dmg == 0 && !is_undirected_spell(spellnum)) {
        impossible("cast directed cleric spell (%d) with dmg=0?", spellnum);
        return;
    }

    switch (spellnum) {
    case SPE_FINGER_OF_DEATH:
        pline("Oh no, %s's using the touch of death!", mhe(mtmp));
        if (nonliving(g.youmonst.data) || is_demon(g.youmonst.data)) {
            You("seem no deader than before.");
        } else if (!Antimagic && rn2(mtmp->m_lev) > 12) {
            if (Hallucination) {
                You("have an out of body experience.");
            } else {
                g.killer.format = KILLED_BY_AN;
                Strcpy(g.killer.name, "touch of death");
                done(DIED);
            }
        } else {
            if (Antimagic)
                shieldeff(u.ux, u.uy);
            pline("Lucky for you, it didn't work!");
        }
        dmg = 0;
        break;
    case SPE_DOUBLE_TROUBLE:
        if (mtmp->iswiz && g.context.no_of_wizards == 1) {
            pline("Double Trouble...");
            clonewiz();
            dmg = 0;
        } else
            impossible("bad wizard cloning?");
        break;
    case SPE_SUMMON_NASTIES: {
        int count;

        count = nasty(mtmp); /* summon something nasty */
        if (mtmp->iswiz) {
            verbalize("Destroy the thief, my pet%s!", plur(count));
        } else {
            const char *mappear = (count == 1) ? "A monster appears"
                                               : "Monsters appear";

            /* messages not quite right if plural monsters created but
               only a single monster is seen */
            if (Invis && !perceives(mtmp->data)
                && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                pline("%s around a spot near you!", mappear);
            else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                pline("%s around your displaced image!", mappear);
            else
                pline("%s from nowhere!", mappear);
        }
        dmg = 0;
        break;
    }
    case SPE_FLAME_SPHERE:
    case SPE_FREEZE_SPHERE: {
        coord cc;
        int count = max(1, mtmp->m_lev / 3);
        int i;
        for (i = 0; i < count; i++) {
            enexto(&cc, mtmp->mx, mtmp->my, mtmp->data);
            makemon(spellnum == SPE_FLAME_SPHERE ? 
                        &mons[PM_FLAMING_SPHERE] : &mons[PM_FREEZING_SPHERE], 
                        cc.x, cc.y, MM_ANGRY);
        }
        if (canseemon(mtmp)) {
            pline("A miasma of %s fills the air!", 
                spellnum == SPE_FLAME_SPHERE ? "flame" : "ice");
        }
        break;
    }
    case SPE_TURN_UNDEAD:
        {
            coord mm;
            if (Invis && !perceives(mtmp->data)) {
                mm.x = mtmp->mux;
                mm.y = mtmp->muy;
            }  else {
                mm.x = u.ux;   
                mm.y = u.uy; 
            }
            if (canseemon(mtmp))
                pline("%s turns undead!", Monnam(mtmp));
            if (!Blind)
                pline("Undead creatures are called forth from the grave!");   
            else
                pline("The smell of corpse dust fills the air.");
            mkundead(&mm, TRUE, NO_MINVENT);   
        }
        dmg = 0;   
        break;   
    case SPE_AGGRAVATION:
        You_feel("that monsters are aware of your presence.");
        aggravate();
        dmg = 0;
        break;
    case SPE_DESTROY_ARMOR:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            pline("A field of force surrounds you!");
        } else if (!destroy_arm(some_armor(&g.youmonst))) {
            Your("skin itches.");
        }
        dmg = 0;
        break;
    case SPE_DRAIN_STRENGTH: /* drain strength */
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            You_feel("momentarily weakened.");
        } else {
            You("suddenly feel weaker!");
            dmg = mtmp->m_lev - 6;
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            losestr(rnd(dmg));
            if (u.uhp < 1)
                done_in_by(mtmp, MURDERED);
        }
        dmg = 0;
        break;
    case SPE_INVISIBILITY: /* makes self invisible */
        if (!mtmp->minvis && !mtmp->invis_blkd) {
            if (canseemon(mtmp))
                pline("%s suddenly %s!", Monnam(mtmp),
                      !See_invisible ? "disappears" : "becomes transparent");
            mon_set_minvis(mtmp);
            if (cansee(mtmp->mx, mtmp->my) && !canspotmon(mtmp))
                map_invisible(mtmp->mx, mtmp->my);
            dmg = 0;
        } else
            impossible("no reason for monster to cast disappear spell?");
        break;
    case SPE_STUN:
        if (Antimagic || Free_action) {
            shieldeff(u.ux, u.uy);
            if (!Stunned)
                You_feel("momentarily disoriented.");
            make_stunned(1L, FALSE);
        } else {
            You(Stunned ? "struggle to keep your balance." : "reel...");
            dmg = d(ACURR(A_DEX) < 12 ? 6 : 4, 4);
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            make_stunned((HStun & TIMEOUT) + (long) dmg, FALSE);
        }
        dmg = 0;
        break;
    case SPE_HASTE_SELF:
        mon_adjust_speed(mtmp, 1, (struct obj *) 0);
        dmg = 0;
        break;
    case SPE_JUMPING:
        mtmp->mextrinsics |= MR2_JUMPING;
        if (canseemon(mtmp))
            pline("%s is %s", Monnam(mtmp), Hallucination ? "hopping mad!" : "very jumpy.");
        break;
    case SPE_WEB:
        trtmp = maketrap(mtmp->mux, mtmp->muy, WEB);
        if (trtmp && mtmp->mux == u.ux && mtmp->muy == u.uy) {
            pline("Webs weave themselves in thin air!");
            dotrap(trtmp, FORCETRAP);
        } else {
            You("smell cobwebs.");
        }
        break;
    case SPE_PSI_BOLT:
        /* prior to 3.4.0 Antimagic was setting the damage to 1--this
           made the spell virtually harmless to players with magic res. */
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            dmg = (dmg + 1) / 2;
        }
        if (dmg <= 5)
            You("get a slight %sache.", body_part(HEAD));
        else if (dmg <= 10)
            Your("brain is on fire!");
        else if (dmg <= 20)
            Your("%s suddenly aches painfully!", body_part(HEAD));
        else
            Your("%s suddenly aches very painfully!", body_part(HEAD));
        break;
    case SPE_GEYSER:
        /* this is physical damage (force not heat),
         * not magical damage or fire damage
         */
        pline("A sudden geyser slams into you from nowhere!");
        dmg = d(8, 6);
        if (Half_physical_damage)
            dmg = (dmg + 1) / 2;
        break;
    case SPE_FLAME_PILLAR:
        pline("A pillar of fire strikes all around you!");
        if (Fire_resistance) {
            shieldeff(u.ux, u.uy);
            dmg = 0;
        } else
            dmg = d(8, 6);
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        burn_away_slime();
        (void) burnarmor(&g.youmonst);
        destroy_item(SCROLL_CLASS, AD_FIRE);
        destroy_item(POTION_CLASS, AD_FIRE);
        destroy_item(SPBOOK_CLASS, AD_FIRE);
        (void) burn_floor_objects(u.ux, u.uy, TRUE, FALSE);
        break;
    case SPE_LIGHTNING: {
        boolean reflects;

        pline("A bolt of lightning strikes down at you from above!");
        reflects = ureflects("It bounces off your %s%s.", "");
        if (reflects || Shock_resistance) {
            shieldeff(u.ux, u.uy);
            dmg = 0;
            if (reflects)
                break;
        } else
            dmg = d(8, 6);
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        destroy_item(WAND_CLASS, AD_ELEC);
        destroy_item(RING_CLASS, AD_ELEC);
        (void) flashburn((long) rnd(100));
        break;
    }
    case SPE_CURSE_ITEMS:
        You_feel("as if you need some help.");
        rndcurse();
        dmg = 0;
        break;
    case SPE_INSECT_SWARM: {
        /* Try for insects, and if there are none
           left, go for (sticks to) snakes.  -3. */
        struct permonst *pm = mkclass(S_ANT, 0);
        struct monst *mtmp2 = (struct monst *) 0;
        char whatbuf[QBUFSZ], let = (pm ? S_ANT : S_SNAKE);
        boolean success = FALSE, seecaster;
        int i, quan, oldseen, newseen;
        coord bypos;
        const char *fmt, *what;

        oldseen = monster_census(TRUE);
        quan = (mtmp->m_lev < 2) ? 1 : rnd((int) mtmp->m_lev / 2);
        if (quan < 3)
            quan = 3;
        for (i = 0; i <= quan; i++) {
            if (!enexto(&bypos, mtmp->mux, mtmp->muy, mtmp->data))
                break;
            if ((pm = mkclass(let, 0)) != 0
                && (mtmp2 = makemon(pm, bypos.x, bypos.y, MM_ANGRY)) != 0) {
                success = TRUE;
                mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
                set_malign(mtmp2);
            }
        }
        newseen = monster_census(TRUE);

        /* not canspotmon() which includes unseen things sensed via warning */
        seecaster = canseemon(mtmp) || tp_sensemon(mtmp) || Detect_monsters;
        what = (let == S_SNAKE) ? "snakes" : "insects";
        if (Hallucination)
            what = makeplural(bogusmon(whatbuf, (char *) 0));

        fmt = 0;
        if (!seecaster) {
            if (newseen <= oldseen || Unaware) {
                /* unseen caster fails or summons unseen critters,
                   or unconscious hero ("You dream that you hear...") */
                You_hear("someone summoning %s.", what);
            } else {
                char *arg;

                if (what != whatbuf)
                    what = strcpy(whatbuf, what);
                /* unseen caster summoned seen critter(s) */
                arg = (newseen == oldseen + 1) ? an(makesingular(what))
                                               : whatbuf;
                if (!Deaf)
                    You_hear("someone summoning something, and %s %s.", arg,
                             vtense(arg, "appear"));
                else
                    pline("%s %s.", upstart(arg), vtense(arg, "appear"));
            }

        /* seen caster, possibly producing unseen--or just one--critters;
           hero is told what the caster is doing and doesn't necessarily
           observe complete accuracy of that caster's results (in other
           words, no need to fuss with visibility or singularization;
           player is told what's happening even if hero is unconscious) */
        } else if (!success) {
            fmt = "%s casts at a clump of sticks, but nothing happens.%s";
            what = "";
        } else if (let == S_SNAKE) {
            fmt = "%s transforms a clump of sticks into %s!";
        } else if (Invis && !perceives(mtmp->data)
                   && (mtmp->mux != u.ux || mtmp->muy != u.uy)) {
            fmt = "%s summons %s around a spot near you!";
        } else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy)) {
            fmt = "%s summons %s around your displaced image!";
        } else {
            fmt = "%s summons %s!";
        }
        if (fmt)
            pline(fmt, Monnam(mtmp), what);

        dmg = 0;
        break;
    }
    case SPE_BLINDNESS:
        /* note: resists_blnd() doesn't apply here */
        if (!Blinded) {
            int num_eyes = eyecount(g.youmonst.data);
            pline("Scales cover your %s!", (num_eyes == 1)
                                               ? body_part(EYE)
                                               : makeplural(body_part(EYE)));
            make_blinded(Half_spell_damage ? 100L : 200L, FALSE);
            if (!Blind)
                Your1(vision_clears);
            dmg = 0;
        } else
            impossible("no reason for monster to cast blindness spell?");
        break;
    case SPE_PARALYZE:
        if (Antimagic || Free_action) {
            shieldeff(u.ux, u.uy);
            if (g.multi >= 0)
                You("stiffen briefly.");
            nomul(-1);
            g.multi_reason = "paralyzed by a monster";
        } else {
            if (g.multi >= 0)
                You("are frozen in place!");
            dmg = 4 + (int) mtmp->m_lev;
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            nomul(-dmg);
            g.multi_reason = "paralyzed by a monster";
        }
        g.nomovemsg = 0;
        dmg = 0;
        break;
    case SPE_CONFUSE_MONSTER:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            You_feel("momentarily dizzy.");
        } else {
            boolean oldprop = !!Confusion;

            dmg = (int) mtmp->m_lev;
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            make_confused(HConfusion + dmg, TRUE);
            if (Hallucination)
                You_feel("%s!", oldprop ? "trippier" : "trippy");
            else
                You_feel("%sconfused!", oldprop ? "more " : "");
        }
        dmg = 0;
        break;
    case SPE_EXTRA_HEALING:
        dmg = m_extra_cure_self(mtmp, dmg);
        break;
    case SPE_HEALING:
        dmg = m_cure_self(mtmp, dmg);
        break;
    case SPE_CURE_BLINDNESS:
        if (canseemon(mtmp))
            pline("%s can see again.", Monnam(mtmp));
        dmg = 0;
        break;
    case SPE_CURE_SICKNESS:
        if (canseemon(mtmp))
            pline("%s is no longer withering away.", Monnam(mtmp));
        dmg = 0;
        break;
    case SPE_PROTECTION:
        if (canseemon(mtmp))
            pline("The area around %s begins to shimmer with %s haze.", mon_nam(mtmp), an(hcolor(NH_GOLDEN)));
        mtmp->prot_bon += IS_EXPERT(mtmp) ? 4 : 2;
        dmg = 0;
        break;
    case SPE_OPEN_WOUNDS:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            dmg = (dmg + 1) / 2;
        }
        if (dmg <= 5)
            Your("skin itches badly for a moment.");
        else if (dmg <= 10)
            pline("Wounds appear on your body!");
        else if (dmg <= 20)
            pline("Severe wounds appear on your body!");
        else
            Your("body is covered with painful wounds!");
        break;
    case SPE_LIGHT:
        litroom_mon(0, 0, u.ux, u.uy);
        dmg = 0;
        break;
    case SPE_DETECT_MONSTERS:
        if (canseemon(mtmp))
            pline("%s seems more certain of your location.", Monnam(mtmp));
        mtmp->mux = u.ux;
        mtmp->muy = u.uy;
        dmg = 0;
        break;
    case SPE_TELEPORT_AWAY:
        (void) rloc(mtmp, TRUE);
        dmg = 0;
        break;
    case SPE_STONE_TO_FLESH:
        if (u.umonnum == PM_STONE_GOLEM) {
            (void) polymon(PM_FLESH_GOLEM);
        }
        if (Stoned) {
            fix_petrification(); /* saved! */
        }
        dmg = 0;
        break;
    case SPE_MAGIC_MISSILE:
    case SPE_SLEEP:
        buzz((int) (-30 - (spellnum - SPE_MAGIC_MISSILE)), 
            spellnum == SPE_MAGIC_MISSILE ? (mtmp->m_lev / 2) + 1 : 6, 
            mtmp->mx, mtmp->my, sgn(g.tbx), sgn(g.tby));
        dmg = 0;
        break;
    case SPE_FIREBALL:
    case SPE_CONE_OF_COLD:
    case SPE_POISON_BLAST:
    case SPE_ACID_STREAM:
    case SPE_SONICBOOM:
    case SPE_PSYSTRIKE:
        cx = u.ux;
        cy = u.uy;
        n = rnd(8) + 1;
        while (n--) {
                explode(u.ux, u.uy,
                        spellnum - SPE_MAGIC_MISSILE + 10,
                        12, 0,
                        expltyp(spellnum));
            cx = cx + rnd(3) - 2;
            cy = cy + rnd(3) - 2;
        }
        dmg = 0;
        break;
    case SPE_FORCE_BOLT:
    case SPE_POLYMORPH:
    case SPE_CANCELLATION:
    case SPE_SLOW_MONSTER:
        pseudo = mksobj(spellnum, FALSE, FALSE);
        pseudo->blessed = pseudo->cursed = 0;
        pseudo->quan = 20L;
        mbhit(mtmp, rn1(8, 6), mbhitm, bhito, pseudo);
        obfree(pseudo, (struct obj *) 0);
        dmg = 0;
        break;
    case SPE_CREATE_FAMILIAR:
    case SPE_CREATE_MONSTER:
        makemon((struct permonst*) 0, u.ux, u.uy, MM_NOERID | MM_NOCOUNTBIRTH);
        dmg = 0;
        break;
    case SPE_KNOCK:
    case SPE_DRAIN_LIFE:
    case SPE_WIZARD_LOCK:
    case SPE_DETECT_FOOD:
    case SPE_CAUSE_FEAR:
    case SPE_CLAIRVOYANCE:
    case SPE_CHARM_MONSTER:
    case SPE_DETECT_UNSEEN:
    case SPE_LEVITATION:
    case SPE_REMOVE_CURSE:
    case SPE_MAGIC_MAPPING:
    case SPE_DETECT_TREASURE:
    case SPE_RESTORE_ABILITY:
    case SPE_IDENTIFY:
    case SPE_DIG:
        impossible("mcastu: useless magic spell (%d)", spellnum);
        dmg = 0;
        break;
    default:
        impossible("mcastu: invalid magic spell (%d)", spellnum);
        dmg = 0;
        break;
    }

    if (dmg)
        mdamageu(mtmp, dmg);
}

static
boolean
is_undirected_spell(spellnum)
int spellnum;
{
    switch (spellnum) {
    case SPE_DOUBLE_TROUBLE:
    case SPE_SUMMON_NASTIES:
    case SPE_FLAME_SPHERE:
    case SPE_FREEZE_SPHERE:
    case SPE_TURN_UNDEAD:
    case SPE_AGGRAVATION:
    case SPE_INVISIBILITY:
    case SPE_HASTE_SELF:
    case SPE_HEALING:
    case SPE_JUMPING:
    case SPE_WEB:
    case SPE_EXTRA_HEALING:
    case SPE_CURE_BLINDNESS:
    case SPE_CURE_SICKNESS:
    case SPE_LIGHT:
    case SPE_PROTECTION:
    case SPE_INSECT_SWARM:
    case SPE_DETECT_MONSTERS:
    case SPE_TELEPORT_AWAY:
    case SPE_FIREBALL:
    case SPE_CONE_OF_COLD:
    case SPE_POISON_BLAST:
    case SPE_ACID_STREAM:
    case SPE_SONICBOOM:
    case SPE_PSYSTRIKE:
    case SPE_CREATE_MONSTER:
    case SPE_CREATE_FAMILIAR:
    /* ranged spells */
    case SPE_FORCE_BOLT:
    case SPE_SLOW_MONSTER:
    case SPE_POLYMORPH:
    case SPE_CANCELLATION:
    case SPE_SLEEP:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/* Some spells are useless under some circumstances. */
static
boolean
spell_would_be_useless(mtmp, spellnum)
struct monst *mtmp;
int spellnum;
{
    /* Some spells don't require the player to really be there and can be cast
     * by the monster when you're invisible, yet still shouldn't be cast when
     * the monster doesn't even think you're there.
     * This check isn't quite right because it always uses your real position.
     * We really want something like "if the monster could see mux, muy".
     */
    boolean mcouldseeu = couldsee(mtmp->mx, mtmp->my);

    /* aggravate monsters, etc. won't be cast by peaceful monsters */
    if (mtmp->mpeaceful
        && (spellnum == SPE_AGGRAVATION || spellnum == SPE_SUMMON_NASTIES
            || spellnum == SPE_INSECT_SWARM || spellnum == SPE_CREATE_MONSTER
            || spellnum == SPE_CREATE_FAMILIAR
            || spellnum == SPE_DOUBLE_TROUBLE || spellnum == SPE_TURN_UNDEAD
            || spellnum == SPE_FLAME_SPHERE || spellnum == SPE_FREEZE_SPHERE
            || spellnum == SPE_WEB || spellnum == SPE_FIREBALL
            || spellnum == SPE_CONE_OF_COLD || spellnum == SPE_POISON_BLAST
            || spellnum == SPE_ACID_STREAM || spellnum == SPE_SONICBOOM
            || spellnum == SPE_PSYSTRIKE))
        return TRUE;
    /* haste self when already fast */
    if (mtmp->permspeed == MFAST && spellnum == SPE_HASTE_SELF)
        return TRUE;
    /* invisibility when already invisible */
    if ((mtmp->minvis || mtmp->invis_blkd) && spellnum == SPE_INVISIBILITY)
        return TRUE;
    /* peaceful monster won't cast invisibility if you can't see
        invisible,
        same as when monsters drink potions of invisibility.  This doesn't
        really make a lot of sense, but lets the player avoid hitting
        peaceful monsters by mistake */
    if (mtmp->mpeaceful && !See_invisible && spellnum == SPE_INVISIBILITY)
        return TRUE;
    /* healing when already healed */
    if (mtmp->mhp == mtmp->mhpmax && (spellnum == SPE_HEALING || spellnum == SPE_EXTRA_HEALING))
        return TRUE;
    if (can_jump(mtmp) && spellnum == SPE_JUMPING)
        return TRUE;
    if (!m_canseeu(mtmp) && (spellnum == SPE_WEB || spellnum == SPE_LIGHT))
        return TRUE;
    if ((!m_canseeu(mtmp) || dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 5) && 
            (spellnum == SPE_FIREBALL
            || spellnum == SPE_CONE_OF_COLD || spellnum == SPE_POISON_BLAST
            || spellnum == SPE_ACID_STREAM || spellnum == SPE_SONICBOOM
            || spellnum == SPE_PSYSTRIKE))
    /* cure blindness when already able to see */
    if (mtmp->mcansee && spellnum == SPE_CURE_BLINDNESS)
        return TRUE;
    /* don't spam protection infinitely */
    if (mtmp->prot_bon >= 15)
        return TRUE;
    /* don't summon monsters if it doesn't think you're around */
    if (!mcouldseeu && (spellnum == SPE_SUMMON_NASTIES
                        || spellnum == SPE_TURN_UNDEAD
                        || spellnum == SPE_FLAME_SPHERE
                        || spellnum == SPE_FREEZE_SPHERE
                        || (!mtmp->iswiz && spellnum == SPE_DOUBLE_TROUBLE)))
        return TRUE;
    if ((!mtmp->iswiz || g.context.no_of_wizards > 1)
        && spellnum == SPE_DOUBLE_TROUBLE)
        return TRUE;
    /* aggravation (global wakeup) when everyone is already active */
    if (spellnum == SPE_AGGRAVATION) {
        /* if nothing needs to be awakened then this spell is useless
            but caster might not realize that [chance to pick it then
            must be very small otherwise caller's many retry attempts
            will eventually end up picking it too often] */
        if (!has_aggravatables(mtmp))
            return rn2(100) ? TRUE : FALSE;
    }  
    /* cure sickness only when acutally sick */
    if (!mtmp->mwither && spellnum == SPE_CURE_SICKNESS)
        return TRUE;
    /* blindness spell on blinded player */
    if (Blinded && spellnum == SPE_BLINDNESS)
        return TRUE;
    /* teleportation on a notele level */
    if (spellnum == SPE_TELEPORT_AWAY && mtmp->mflee 
        && (noteleport_level(mtmp) || tele_restrict(mtmp)))
        return TRUE;
    if (spellnum == SPE_STONE_TO_FLESH && u.umonnum != PM_STONE_GOLEM)
        return TRUE;
    if ((mtmp->mpeaceful || !lined_up(mtmp)) && (spellnum == SPE_FORCE_BOLT || spellnum == SPE_SLEEP
        || spellnum == SPE_POLYMORPH || spellnum == SPE_CANCELLATION
        || spellnum == SPE_SLOW_MONSTER))
        return TRUE;
    /* Prevent monsters from dealing large amounts of damage to themselves via magic missile. */
    if (spellnum == SPE_MAGIC_MISSILE && Reflecting)
        return TRUE;
    if (spellnum == SPE_KNOCK || spellnum == SPE_DRAIN_LIFE || spellnum == SPE_WIZARD_LOCK
        || spellnum == SPE_DETECT_FOOD || spellnum == SPE_CAUSE_FEAR || spellnum == SPE_CLAIRVOYANCE
        || spellnum == SPE_CHARM_MONSTER || spellnum == SPE_DETECT_UNSEEN || spellnum == SPE_LEVITATION
        || spellnum == SPE_REMOVE_CURSE || spellnum == SPE_MAGIC_MAPPING
        || spellnum == SPE_DETECT_TREASURE || spellnum == SPE_RESTORE_ABILITY || spellnum == SPE_IDENTIFY
        || spellnum == SPE_DIG)
        return TRUE;
    return FALSE;
}

/* convert 1..10 to 0..9; add 10 for second group (spell casting) */
#define ad_to_typ(k) (10 + (int) k - 1)

/* monster uses spell (ranged) */
int
buzzmu(mtmp, mattk)
register struct monst *mtmp;
register struct attack *mattk;
{
    /* don't print constant stream of curse messages for 'normal'
       spellcasting monsters at range */
    if (mattk->adtyp > AD_PSYC)
        return 0;

    if (mtmp->mcan) {
        cursetxt(mtmp, FALSE);
        return 0;
    }
    if (lined_up(mtmp) && rn2(3)) {
        nomul(0);
        if (mattk->adtyp && (mattk->adtyp < 11)) { /* no cf unsigned >0 */
            if (canseemon(mtmp))
                pline("%s zaps you with a %s!", Monnam(mtmp),
                      flash_types[ad_to_typ(mattk->adtyp)]);
            buzz(-ad_to_typ(mattk->adtyp), (int) mattk->damn, mtmp->mx,
                 mtmp->my, sgn(g.tbx), sgn(g.tby));
        } else
            impossible("Monster spell %d cast", mattk->adtyp - 1);
    }
    return 1;
}
/* return values:
* 2: target died
* 1: successful spell
* 0: unsuccessful spell
*/
int
castmm(mtmp, mdef, mattk)
register struct monst *mtmp;
register struct monst *mdef;
register struct attack *mattk;
{
   	int	dmg, ml = mtmp->m_lev;
   	int ret;
   	int spellnum = 0;

   	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
   	    int cnt = 40;

   	    do {
   	        spellnum = choose_monster_spell(mtmp, mattk->adtyp == AD_SPEL);
           		   /* not trying to attack?  don't allow directed spells */
   	    } while(--cnt > 0 &&
   		    spell_would_be_useless(mtmp, spellnum));
   	    if (cnt == 0)
            return 0;

   	}

   	/* monster unable to cast spells? */
   	if(mtmp->mcan || mtmp->mspec_used || !ml) {
   	    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my))
   	    {
                   char buf[BUFSZ];
   		Sprintf(buf, "%s", Monnam(mtmp));

   		if (is_undirected_spell(spellnum))
   	            pline("%s points all around, then curses.", buf);
   		else
   	            pline("%s points at %s, then curses.",
   		          buf, mon_nam(mdef));

   	    } else if ((!(g.moves % 4) || !rn2(4))) {
   	        if (!Deaf) Norep("You hear a mumbled curse.");
   	    }
   	    return(0);
   	}

   	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
   	    mtmp->mspec_used = 10 - mtmp->m_lev;
   	    if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
   	}

   	if(rn2(ml*10) < (mtmp->mconf ? 100 : 20)) {	/* fumbled attack */
   	    if (canseemon(mtmp))
   		pline_The("air crackles around %s.", mon_nam(mtmp));
   	    return(0);
   	}
   	if (cansee(mtmp->mx, mtmp->my) ||
   	    canseemon(mtmp) ||
   	    (!is_undirected_spell(spellnum) &&
   	     (cansee(mdef->mx, mdef->my) || canseemon(mdef)))) {
               char buf[BUFSZ];
   	    Sprintf(buf, " at ");
   	    Strcat(buf, mon_nam(mdef));
   	    pline("%s casts a spell%s!",
   		  canspotmon(mtmp) ? Monnam(mtmp) : "Something",
   		  is_undirected_spell(spellnum) ? "" : buf);
   	}

   	if (mattk->damd)
   	    dmg = d((int)((ml/2) + mattk->damn), (int)mattk->damd);
   	else dmg = d((int)((ml/2) + 1), 6);

   	ret = 1;

   	switch (mattk->adtyp) {
   	  case AD_FIRE:
   	      if (canspotmon(mdef)) {
              is_demon(mtmp->data) ?
   		          pline("%s is enveloped in flames.", Monnam(mdef))
                : pline("%s is enveloped in a pillar of hellfire!",
                    Monnam(mdef));
          }
          if (is_demon(mtmp->data) && resists_fire(mdef)) {
              dmg = (dmg + 1) / 2;
              break;
          }
       		if(resists_fire(mdef)) {
         			shieldeff(mdef->mx, mdef->my);
         	                if (canspotmon(mdef))
         			    pline("But %s resists the effects.",
         			        mhe(mdef));
         			dmg = 0;
   		}
   		break;
   	  case AD_COLD:
   	      if (canspotmon(mdef))
   		        pline("%s is covered in frost.", Monnam(mdef));
       		if(resists_fire(mdef)) {
         			shieldeff(mdef->mx, mdef->my);
         	                if (canspotmon(mdef))
         			    pline("But %s resists the effects.",
         			        mhe(mdef));
         			dmg = 0;
       		}
   		    break;
      case AD_PSYC:
          if (canspotmon(mdef))
              pline("%s hits %s with a mental blast!",
                  Monnam(mtmp), mon_nam(mdef));
          if (resists_psychic(mdef)) {
              shieldeff(mdef->mx, mdef->my);
              pline("But %s resists the mental assault!", mhe(mdef));
              dmg = 0;
          }
          break;
      case AD_WIND:
        if (canspotmon(mdef))
            pline("%s is blasted by wind!", Monnam(mtmp));
        mhurtle(mdef, mtmp->mx - mdef->mx, mtmp->my - mdef->my, dmg);
        dmg = 0;
        break;
   	  case AD_MAGM:
            if (canspotmon(mdef))
         		    pline("%s is hit by a shower of missiles!", Monnam(mdef));
         		if(resists_magm(mdef)) {
           			shieldeff(mdef->mx, mdef->my);
           	                if (canspotmon(mdef))
           			    pline_The("missiles bounce off!");
           			dmg = 0;
         		} else
                dmg = d((int)mtmp->m_lev/2 + 1,6);
         		break;
        case AD_LAWS:
            if (canspotmon(mdef)) {
                pline("%s get hit with a cease and desist!", Monnam(mdef));
            }
            break;
   	    case AD_SPEL:	/* wizard spell */
   	    case AD_CLRC: /* clerical spell */
   	    {
   	        /*aggravation is a special case;*/
         		/*it's undirected but should still target the*/
         		/*victim so as to aggravate you*/
   	        if (is_undirected_spell(spellnum)
                && (mattk->adtyp != AD_SPEL
                || (spellnum != SPE_AGGRAVATION &&
                  spellnum != SPE_WEB &&
                  spellnum != SPE_SUMMON_NASTIES &&
                  spellnum != SPE_FLAME_SPHERE &&
                  spellnum != SPE_FREEZE_SPHERE &&
                  spellnum != SPE_TURN_UNDEAD))) {
     		    cast_monster_spell(mtmp, dmg, spellnum, mattk->adtyp == AD_SPEL);
         		dmg = 0; /* done by the spell casting functions */
         		break;
   	        }
   	    }
    }
   	if (dmg > 0 && mdef->mhp > 0) {
   	    mdef->mhp -= dmg;
   	    if (mdef->mhp < 1) monkilled(mdef, "", mattk->adtyp);
   	}
   	if (mdef && mdef->mhp < 1)
        return 2;
   	return(ret);
}

/* return values:
* 2: target died
* 1: successful spell
* 0: unsuccessful spell
*/
int
castum(mtmp, mattk)
register struct monst *mtmp;
register struct attack *mattk;
{
   	int dmg, ml = mons[u.umonnum].mlevel;
   	int ret;
   	boolean directed = FALSE;

   	/* unable to cast spells? */
   	if(u.uen < ml) {
   	    if (directed)
   	        You("point at %s, then curse.", mon_nam(mtmp));
   	    else
   	        You("point all around, then curse.");
   	    return(0);
   	}

   	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
   	    u.uen -= ml;
   	}

   	if (rn2(ml*10) < (Confusion ? 100 : 20)) {	/* fumbled attack */
  	    pline_The("air crackles around you.");
   	    return(0);
   	}

    You("cast a spell%s%s!",
	    directed ? " at " : "",
	    directed ? mon_nam(mtmp) : "");

   /*
    *	As these are spells, the damage is related to the level
    *	of the monster casting the spell.
    */
   	if (mattk->damd)
   	    dmg = d((int)((ml/2) + mattk->damn), (int)mattk->damd);
   	else
        dmg = d((int)((ml/2) + 1), 6);

   	ret = 1;

   	switch (mattk->adtyp) {
   	    case AD_FIRE:
         		pline("%s is enveloped in flames.", Monnam(mtmp));
         		if(resists_fire(mtmp)) {
           			shieldeff(mtmp->mx, mtmp->my);
           			pline("But %s resists the effects.",
           			    mhe(mtmp));
           			dmg = 0;
         		}
         		break;
   	    case AD_COLD:
         		pline("%s is covered in frost.", Monnam(mtmp));
         		if(resists_fire(mtmp)) {
         			shieldeff(mtmp->mx, mtmp->my);
         			pline("But %s resists the effects.",
         			    mhe(mtmp));
         			dmg = 0;
         		}
         		break;
        case AD_PSYC:
            You("hit %s with a mental blast!", mon_nam(mtmp));
            if (resists_psychic(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("But %s resists the mental assault!", mhe(mtmp));
                dmg = 0;
            }
            break;
        case AD_WIND:
            pline("%s is blasted by wind!", Monnam(mtmp));
            mhurtle(mtmp, u.ux - mtmp->mx, u.uy - mtmp->my, dmg);
            dmg = 0;
            break;
   	    case AD_MAGM:
         		pline("%s is hit by a shower of missiles!", Monnam(mtmp));
         		if (resists_magm(mtmp)) {
           			shieldeff(mtmp->mx, mtmp->my);
           			pline_The("missiles bounce off!");
           			dmg = 0;
         		} else
                dmg = d((int)ml/2 + 1,6);
         		break;
   	    case AD_SPEL:	/* wizard spell */
   	    case AD_CLRC: /* clerical spell */
   	    {
         		docast();
         		dmg = 0; /* done by the spell casting functions */
         		break;
   	    }
   	}

   	if (mtmp && dmg > 0 && mtmp->mhp > 0) {
   	    mtmp->mhp -= dmg;
   	    if (mtmp->mhp < 1) killed(mtmp);
   	}
   	if (mtmp && mtmp->mhp < 1)
        return 2;
 	  return(ret);
}

 extern NEARDATA const int nasties[];


/*mcastu.c*/

/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "game.h"
#include "map.h"
#include "support.h"
#include "tech.h"
#include "shared.h" /* ARRAY_SIZE */

#include "improvement.h"

/* get 'struct ceff_vector' functions: */
#define SPECVEC_TAG ceff
#define SPECVEC_TYPE struct eff_city
#include "specvec_c.h"

/* get 'struct geff_vector' functions: */
#define SPECVEC_TAG geff
#define SPECVEC_TYPE struct eff_global
#include "specvec_c.h"

/**************************************************************************
All the city improvements:
Use get_improvement_type(id) to access the array.
The improvement_types array is now setup in:
   server/ruleset.c (for the server)
   client/packhand.c (for the client)
**************************************************************************/
struct impr_type improvement_types[B_LAST];

/* Names of effect ranges.
 * (These must correspond to enum effect_range_id in improvement.h.)
 */
static const char *effect_range_names[] = {
  "None",
  "Building",
  "City",
  "Island",
  "Player",
  "World"
};

/* Names of effect types.
 * (These must correspond to enum effect_type_id in improvement.h.)
 */
static const char *effect_type_names[] = {
  "Adv_Parasite",
  "Airlift",
  "Any_Government",
  "Barb_Attack",
  "Barb_Defend",
  "Capital_City",
  "Capital_Exists",
  "Enable_Nuke",
  "Enable_Space",
  "Enemy_Peaceful",
  "Food_Add_Tile",
  "Food_Bonus",
  "Food_Inc_Tile",
  "Food_Per_Tile",
  "Give_Imm_Adv",
  "Growth_Food",
  "Have_Embassies",
  "Improve_Rep",
  "Luxury_Bonus",
  "Luxury_Pct",
  "Make_Content",
  "Make_Content_Mil",
  "Make_Content_Pct",
  "Make_Happy",
  "May_Declare_War",
  "No_Anarchy",
  "No_Sink_Deep",
  "Nuke_Proof",
  "Pollu_Adj",
  "Pollu_Adj_Pop",
  "Pollu_Adj_Prod",
  "Pollu_Set",
  "Pollu_Set_Pop",
  "Pollu_Set_Prod",
  "Prod_Add_Tile",
  "Prod_Bonus",
  "Prod_Inc_Tile",
  "Prod_Per_Tile",
  "Prod_To_Gold",
  "Reduce_Corrupt",
  "Reduce_Waste",
  "Reveal_Cities",
  "Reveal_Map",
  "Revolt_Dist",
  "Science_Bonus",
  "Science_Pct",
  "Size_Unlimit",
  "Slow_Nuke_Winter",
  "Slow_Global_Warm",
  "Space_Part",
  "Spy_Resistant",
  "Tax_Bonus",
  "Tax_Pct",
  "Trade_Add_Tile",
  "Trade_Bonus",
  "Trade_Inc_Tile",
  "Trade_Per_Tile",
  "Trade_Route_Pct",
  "Unit_Defend",
  "Unit_Move",
  "Unit_No_Lose_Pop",
  "Unit_Recover",
  "Unit_Repair",
  "Unit_Vet_Combat",
  "Unit_Veteran",
  "Upgrade_Units",
  "Upgrade_One_Step",
  "Upgrade_One_Leap",
  "Upgrade_All_Step",
  "Upgrade_All_Leap",
  "Upkeep_Free"
};

/**************************************************************************
  Convert effect range names to enum; case insensitive;
  returns EFR_LAST if can't match.
**************************************************************************/
enum effect_range effect_range_from_str(const char *str)
{
  enum effect_range ret_id;

  assert(ARRAY_SIZE(effect_range_names) == EFR_LAST);

  for (ret_id = 0; ret_id < EFR_LAST; ret_id++) {
    if (0 == mystrcasecmp(effect_range_names[ret_id], str)) {
      return ret_id;
    }
  }

  return EFR_LAST;
}

/**************************************************************************
  Return effect range name; NULL if bad id.
**************************************************************************/
const char *effect_range_name(enum effect_range id)
{
  assert(ARRAY_SIZE(effect_range_names) == EFR_LAST);

  if (id < EFR_LAST) {
    return effect_range_names[id];
  } else {
    return NULL;
  }
}

/**************************************************************************
  Convert effect type names to enum; case insensitive;
  returns EFT_LAST if can't match.
**************************************************************************/
enum effect_type effect_type_from_str(const char *str)
{
  enum effect_type ret_id;

  assert(ARRAY_SIZE(effect_type_names) == EFT_LAST);

  for (ret_id = 0; ret_id < EFT_LAST; ret_id++) {
    if (0 == mystrcasecmp(effect_type_names[ret_id], str)) {
      return ret_id;
    }
  }

  return EFT_LAST;
}

/**************************************************************************
  Return effect type name; NULL if bad id.
**************************************************************************/
const char *effect_type_name(enum effect_type id)
{
  assert(ARRAY_SIZE(effect_type_names) == EFT_LAST);

  if (id < EFT_LAST) {
    return effect_type_names[id];
  } else {
    return NULL;
  }
}

/**************************************************************************
  Frees the memory associated with this improvement.
**************************************************************************/
static void improvement_free(Impr_Type_id id)
{
  struct impr_type *p = get_improvement_type(id);

  free(p->terr_gate);
  p->terr_gate = NULL;

  free(p->spec_gate);
  p->spec_gate = NULL;

  free(p->equiv_dupl);
  p->equiv_dupl = NULL;

  free(p->equiv_repl);
  p->equiv_repl = NULL;

  free(p->effect);
  p->effect = NULL;

  free(p->helptext);
  p->helptext = NULL;
}

/***************************************************************
 Frees the memory associated with all improvements.
***************************************************************/
void improvements_free()
{
  impr_type_iterate(impr) {
    improvement_free(impr);
  } impr_type_iterate_end;
}

/**************************************************************************
Returns 1 if the improvement_type "exists" in this game, 0 otherwise.
An improvement_type doesn't exist if one of:
- id is out of range;
- the improvement_type has been flagged as removed by setting its
  tech_req to A_LAST;
- it is a space part, and the spacerace is not enabled.
Arguably this should be called improvement_type_exists, but that's too long.
**************************************************************************/
bool improvement_exists(Impr_Type_id id)
{
  if (id<0 || id>=B_LAST || id>=game.num_impr_types)
    return FALSE;

  if ((id==B_SCOMP || id==B_SMODULE || id==B_SSTRUCTURAL)
      && !game.spacerace)
    return FALSE;

  return (improvement_types[id].tech_req!=A_LAST);
}

/**************************************************************************
...
**************************************************************************/
struct impr_type *get_improvement_type(Impr_Type_id id)
{
  return &improvement_types[id];
}

/**************************************************************************
...
**************************************************************************/
const char *get_improvement_name(Impr_Type_id id)
{
  return get_improvement_type(id)->name; 
}

/**************************************************************************
...
**************************************************************************/
int improvement_value(Impr_Type_id id)
{
  return (improvement_types[id].build_cost);
}

/**************************************************************************
...
**************************************************************************/
bool is_wonder(Impr_Type_id id)
{
  return (improvement_types[id].is_wonder);
}

/**************************************************************************
Does a linear search of improvement_types[].name
Returns B_LAST if none match.
**************************************************************************/
Impr_Type_id find_improvement_by_name(const char *s)
{
  impr_type_iterate(i) {
    if (strcmp(improvement_types[i].name, s)==0)
      return i;
  } impr_type_iterate_end;

  return B_LAST;
}

/**************************************************************************
FIXME: remove when gen-impr obsoletes
**************************************************************************/
int improvement_variant(Impr_Type_id id)
{
  return improvement_types[id].variant;
}

/**************************************************************************
 Returns 1 if the improvement is obsolete, now also works for wonders
**************************************************************************/
bool improvement_obsolete(struct player *pplayer, Impr_Type_id id) 
{
  if (improvement_types[id].obsolete_by==A_NONE) 
    return FALSE;

  if (improvement_types[id].is_wonder) {
    /* a wonder is obsolette, as soon as *any* player researched the
       obsolete tech */
   return game.global_advances[improvement_types[id].obsolete_by] != 0;
  }

  return (get_invention(pplayer, improvement_types[id].obsolete_by)
	  ==TECH_KNOWN);
}

/**************************************************************************
 Fills in lists of improvements at all equiv_ranges that might affect the
 given city (owned by the given player)
**************************************************************************/
static void fill_ranges_improv_lists(Impr_Status *implist[EFR_LAST],
				     struct city *pcity,
				     struct player *pplayer)
{
  int i,cont=-1;
  for (i=0;i<EFR_LAST;i++) implist[i]=NULL;

  if (pcity) {
    implist[EFR_CITY]=pcity->improvements;
    cont = map_get_continent(pcity->x,pcity->y);
  }

  if (pplayer) {
    implist[EFR_PLAYER]=pplayer->improvements;
    if (cont >= 0) {
      assert(pplayer->island_improv != NULL);
      implist[EFR_ISLAND] = &pplayer->island_improv[cont*game.num_impr_types];
    }
  }

  implist[EFR_WORLD]=game.improvements;
}

/**************************************************************************
 Checks whether the building is within the equiv_range of a building that
 replaces it
**************************************************************************/
bool improvement_redundant(struct player *pplayer,struct city *pcity,
                          Impr_Type_id id, bool want_to_build) 
{
  int i;
  Impr_Status *equiv_list[EFR_LAST];
  Impr_Type_id *ept;

  /* Make lists of improvements that affect this city */
  fill_ranges_improv_lists(equiv_list,pcity,pplayer);

  /* For every improvement named in equiv_dupl or equiv_repl, check for
     its presence in any of the lists (we check only for its presence, and
     assume that it has the "equiv" effect even if it itself is redundant) */
  for (ept=improvement_types[id].equiv_repl;ept && *ept!=B_LAST;ept++) {
    for (i=0;i<EFR_LAST;i++) {
      if (equiv_list[i]) {
      	 Impr_Status stat = equiv_list[i][*ept];
      	 if (stat != I_NONE && stat != I_OBSOLETE) return TRUE;
      }
    }
  }

  /* equiv_dupl makes buildings redundant, but that shouldn't stop you
     from building them if you really want to */
  if (!want_to_build) {
    for (ept=improvement_types[id].equiv_dupl;ept && *ept!=B_LAST;ept++) {
      for (i=0;i<EFR_LAST;i++) {
	if (equiv_list[i]) {
	  Impr_Status stat = equiv_list[i][*ept];
	  if (stat != I_NONE && stat != I_OBSOLETE) return TRUE;
	}
      }
    }
  }
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
bool wonder_obsolete(Impr_Type_id id)
{ 
  if (improvement_types[id].obsolete_by==A_NONE)
    return FALSE;
  return (game.global_advances[improvement_types[id].obsolete_by] != 0);
}

/**************************************************************************
Barbarians don't get enough knowledges to be counted as normal players.
**************************************************************************/
bool is_wonder_useful(Impr_Type_id id)
{
  if ((id == B_GREAT) && (get_num_human_and_ai_players () < 3)) return FALSE;
  return TRUE;
}

/**************************************************************************
 Clears a list of improvements - sets them all to I_NONE
**************************************************************************/
void improvement_status_init(Impr_Status * improvements, size_t elements)
{
  /* 
   * Since this function is called with elements!=game.num_impr_types
   * impr_type_iterate can't used here.
   */
  Impr_Type_id i;

  for (i = 0; i < elements; i++) {
    improvements[i] = I_NONE;
  }
}

/**************************************************************************
  Whether player could build this improvement, assuming they had
  the tech req, and assuming a city with the right pre-reqs etc.
**************************************************************************/
bool could_player_eventually_build_improvement(struct player *p,
					      Impr_Type_id id)
{
  struct impr_type *impr;

  /* This also checks if tech req is Never */
  if (!improvement_exists(id))
    return FALSE;

  impr = get_improvement_type(id);

  if (impr->effect) {
    struct impr_effect *peffect = impr->effect;
    enum effect_type type;

    /* This if for a spaceship component is asked */
    while ((type = peffect->type) != EFT_LAST) {
      if (type == EFT_SPACE_PART) {
      	/* TODO: remove this */
	if (game.global_wonders[B_APOLLO] == 0)
	  return FALSE;
        if (p->spaceship.state >= SSHIP_LAUNCHED)
	  return FALSE;
	if (peffect->amount == 1 && p->spaceship.structurals >= NUM_SS_STRUCTURALS)
	  return FALSE;
	if (peffect->amount == 2 && p->spaceship.components >= NUM_SS_COMPONENTS)
	  return FALSE;
	if (peffect->amount == 3 && p->spaceship.modules >= NUM_SS_MODULES)
	  return FALSE;
      }
      peffect++;
    }
  }
  if (is_wonder(id)) {
    /* Can't build wonder if already built */
    if (game.global_wonders[id] != 0) return FALSE;
  } else {
    /* Can't build if obsolette */
    if (improvement_obsolete(p, id)) return FALSE;
  }
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool could_player_build_improvement(struct player *p, Impr_Type_id id)
{
  if (!could_player_eventually_build_improvement(p, id))
    return FALSE;

  /* Make sure we have the tech /now/.*/
  if (get_invention(p, improvement_types[id].tech_req) == TECH_KNOWN)
    return TRUE;
  return FALSE;
}
  
/**************************************************************************
Can a player build this improvement somewhere?  Ignores the
fact that player may not have a city with appropriate prereqs.
**************************************************************************/
bool can_player_build_improvement(struct player *p, Impr_Type_id id)
{
  if (!improvement_exists(id))
    return FALSE;
  if (!player_knows_improvement_tech(p,id))
    return FALSE;
  return(could_player_build_improvement(p, id));
}

/**************************************************************************
 Marks an improvment to the status
**************************************************************************/
void mark_improvement(struct city *pcity,Impr_Type_id id,Impr_Status status)
{
  enum effect_range equiv_range;
  Impr_Status *improvements,*equiv_list[EFR_LAST];
  struct player *pplayer;

  pcity->improvements[id] = status;
  pplayer = city_owner(pcity);
  equiv_range = improvement_types[id].equiv_range;

  /* Get the relevant improvement list */
  fill_ranges_improv_lists(equiv_list,pcity,pplayer);
  improvements = equiv_list[equiv_range];

  if (improvements) {
    /* And set the same status */
    improvements[id] = status;
  }
}

/**************************************************************************
...
**************************************************************************/
struct geff_vector *get_eff_world(void)
{
  return (&game.effects);
}

/**************************************************************************
...
**************************************************************************/
struct geff_vector *get_eff_player(struct player *plr)
{
  return (&plr->effects);
}

/**************************************************************************
...
**************************************************************************/
struct geff_vector *get_eff_island(int cont, struct player *plr)
{
  return (&plr->island_effects[cont]);
}

/**************************************************************************
...
**************************************************************************/
struct ceff_vector *get_eff_city(struct city *pcity)
{
  return (&pcity->effects);
}

/**************************************************************************
  Converts the given geff_vector into a ceff_vector (struct eff_global is
  a derived class of struct eff_city, and by the same token geff_vector
  is derived from ceff_vector). The returned vector functions exactly as
  the orginal geff_vector, but returns eff_city structures.
**************************************************************************/
#ifdef UNUSED
static struct ceff_vector *get_geff_parent(struct geff_vector *geff)
{
  return (struct ceff_vector *)geff;
}
#endif

/**************************************************************************
  Fills in the efflist pointer array with the eff_global lists that could
  be changed by the given improvement (in the given city) being built
  or destroyed.
**************************************************************************/
void get_effect_vectors(struct ceff_vector *ceffs[],
			struct geff_vector *geffs[],
			Impr_Type_id impr, struct city *pcity)
{
  struct impr_effect *ie;
  int j, i, cont;
  bool effects[EFR_LAST];
  struct player *plr;

  assert(pcity && impr>=0 && impr<game.num_impr_types);

  for (i=0; i<EFR_LAST; i++)
    effects[i]=FALSE;

  if ((ie=improvement_types[impr].effect)) {
    for (; ie->type<EFT_LAST; ie++) {
      effects[ie->range]=TRUE;
    }
  }

  cont = map_get_continent(pcity->x, pcity->y);
  plr = city_owner(pcity);

  i=0;
  for (j=0; j<EFR_LAST; j++) {
    if (effects[j]) {
      switch (j) {
      case EFR_ISLAND: geffs[i++]=get_eff_island(cont, plr);	break;
      case EFR_PLAYER: geffs[i++]=get_eff_player(plr);		break;
      case EFR_WORLD:  geffs[i++]=get_eff_world();		break;
      default:  						break;
      }
    }
  }
  geffs[i++]=NULL;

  ceffs[0]=get_eff_city(pcity);
  ceffs[1]=NULL;
}

/**************************************************************************
  Updates the relevant global effect structures, to bring them into
  line with the current activity status of their parent city structure,
  which should be owned by the passed pcity.
**************************************************************************/
void update_global_effect(struct city *pcity, struct eff_city *effect)
{
  struct geff_vector *effs[3];
  int i, j, cont;
  struct player *plr;

  cont = map_get_continent(pcity->x, pcity->y);
  plr = city_owner(pcity);

  effs[0] = get_eff_island(cont, plr);
  effs[1] = get_eff_player(plr);
  effs[2] = get_eff_world();

  for (j=0; j<3; j++) {
    for (i=0; i<geff_vector_size(effs[j]); i++) {
      struct eff_global *eff=geff_vector_get(effs[j], i);

      if (eff->cityid==pcity->id && eff->eff.impr==effect->impr) {
	eff->eff.active=effect->active;
	break;
      }
    }
  }
}

/**************************************************************************
...
**************************************************************************/
struct eff_city *append_ceff(struct ceff_vector *x)
{
  int i, n;
  struct eff_city *eff;

  /* Try for an unused vector instance if possible. */
  n=ceff_vector_size(x);
  for (i=0; i<n; i++) {
    eff=ceff_vector_get(x, i);
    if (eff->impr==B_LAST)
      return eff;
  }

  /* That didn't work, so add a new instance to the vector. */
  ceff_vector_reserve(x, n+1);
  return ceff_vector_get(x, n);
}

/**************************************************************************
...
**************************************************************************/
struct eff_global *append_geff(struct geff_vector *x)
{
  int i, n;
  struct eff_global *eff;

  /* Try for an unused vector instance if possible. */
  n=geff_vector_size(x);
  for (i=0; i<n; i++) {
    eff=geff_vector_get(x, i);
    if (eff->eff.impr==B_LAST)
      return eff;
  }

  /* That didn't work, so add a new instance to the vector. */
  geff_vector_reserve(x, n+1);
  return geff_vector_get(x, n);
}

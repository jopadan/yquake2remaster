/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) ZeniMax Media Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Miscellaneos entities, functs and functions.
 *
 * =======================================================================
 */

#include "header/local.h"
#include "monster/misc/player.h"

int debristhisframe;
int gibsthisframe;

extern void M_WorldEffects(edict_t *ent);

/*
 * QUAKED func_group (0 0 0) ?
 * Used to group brushes together just for editor convenience.
 */

/* ===================================================== */

void
Use_Areaportal(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!ent)
	{
		return;
	}

	ent->count ^= 1; /* toggle state */
	gi.SetAreaPortalState(ent->style, ent->count);
}

/*
 * QUAKED func_areaportal (0 0 0) ?
 *
 * This is a non-visible object that divides the world into
 * areas that are seperated when this portal is not activated.
 * Usually enclosed in the middle of a door.
 */
void
SP_func_areaportal(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->use = Use_Areaportal;
	ent->count = 0; /* always start closed; */
}

/* ===================================================== */

static void
VelocityForDamage(int damage, vec3_t v)
{
	v[0] = 100.0 * crandom();
	v[1] = 100.0 * crandom();
	v[2] = 200.0 + 100.0 * random();

	if (damage < 50)
	{
		VectorScale(v, 0.7, v);
	}
	else
	{
		VectorScale(v, 1.2, v);
	}
}

void
ClipGibVelocity(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (ent->velocity[0] < -300)
	{
		ent->velocity[0] = -300;
	}
	else if (ent->velocity[0] > 300)
	{
		ent->velocity[0] = 300;
	}

	if (ent->velocity[1] < -300)
	{
		ent->velocity[1] = -300;
	}
	else if (ent->velocity[1] > 300)
	{
		ent->velocity[1] = 300;
	}

	if (ent->velocity[2] < 200)
	{
		ent->velocity[2] = 200; /* always some upwards */
	}
	else if (ent->velocity[2] > 500)
	{
		ent->velocity[2] = 500;
	}
}

/* ===================================================== */

void
gib_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.frame++;
	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == 10)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 8 + random() * 10;
	}
}

void
gib_touch(edict_t *self, edict_t *other /* unused */, cplane_t *plane, csurface_t *surf /* unused */)
{
	vec3_t normal_angles, right;

	if (!self)
	{
		return;
	}

	if (!self->groundentity)
	{
		return;
	}

	self->touch = NULL;

	if (plane)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex(
						"misc/fhit3.wav"), 1, ATTN_NORM, 0);

		vectoangles(plane->normal, normal_angles);
		AngleVectors(normal_angles, NULL, right, NULL);
		vectoangles(right, self->s.angles);

		if (self->s.modelindex == sm_meat_index)
		{
			self->s.frame++;
			self->think = gib_think;
			self->nextthink = level.time + FRAMETIME;
		}
	}
}

void
gib_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */,
		int damage /* unused */, vec3_t point /* unused */)
{
	if (!self)
	{
		return;
	}

	G_FreeEdict(self);
}

void
ThrowGib(edict_t *self, const char *gibname, int damage, int type)
{
	edict_t *gib;
	vec3_t vd;
	vec3_t origin;
	vec3_t size;
	float vscale;

	if (!self || !gibname)
	{
		return;
	}

	if (gibsthisframe >= MAX_GIBS)
	{
		return;
	}

	gib = G_SpawnOptional();

	if (!gib)
	{
		return;
	}

	gibsthisframe++;

	VectorScale(self->size, 0.5, size);
	VectorAdd(self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel(gib, gibname);
	gib->solid = SOLID_BBOX;
	gib->svflags = SVF_DEADMONSTER;
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;
	gib->health = 250;

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity(gib);
	gib->avelocity[0] = random() * 600;
	gib->avelocity[1] = random() * 600;
	gib->avelocity[2] = random() * 600;

	gib->think = G_FreeEdict;
	gib->nextthink = level.time + 10 + random() * 10;
	gib->s.renderfx |= RF_IR_VISIBLE;

	gi.linkentity(gib);
}

void
ThrowHead(edict_t *self, const char *gibname, int damage, int type)
{
	vec3_t vd;
	float vscale;

	if (!self || !gibname)
	{
		return;
	}

	self->s.skinnum = 0;
	self->s.frame = 0;
	VectorClear(self->mins);
	VectorClear(self->maxs);

	self->s.modelindex2 = 0;
	gi.setmodel(self, gibname);
	self->solid = SOLID_BBOX;
	self->s.effects |= EF_GIB;
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_MONSTER;
	self->takedamage = DAMAGE_YES;
	self->targetname = NULL;
	self->die = gib_die;

	// The entity still has the monsters clipmaks.
	// Reset it to MASK_SHOT to be on the save side.
	// (MASK_SHOT is used by xatrix)
	self->clipmask = MASK_SHOT;

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		self->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, self->velocity);
	ClipGibVelocity(self);

	self->avelocity[YAW] = crandom() * 600;

	self->think = G_FreeEdict;
	self->nextthink = level.time + 10 + random() * 10;

	gi.linkentity(self);
}

void
ThrowGibACID(edict_t *self, const char *gibname, int damage, int type)
{
	edict_t *gib;
	vec3_t vd;
	vec3_t origin;
	vec3_t size;
	float vscale;

	if (!self || !gibname)
	{
		return;
	}

	gibsthisframe++;

	if (gibsthisframe > MAX_GIBS)
	{
		return;
	}

	gib = G_Spawn();

	VectorScale(self->size, 0.5, size);
	VectorAdd(self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	/* gi.setmodel (gib, gibname); */
	gib->s.modelindex = gi.modelindex(gibname);

	gib->clipmask = MASK_SHOT;
	gib->solid = SOLID_BBOX;

	gib->s.effects |= EF_GREENGIB;
	/* note to self check this */
	gib->s.renderfx |= RF_FULLBRIGHT;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;
	gib->dmg = 2;
	gib->health = 250;

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		vscale = 3.0;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity(gib);
	gib->avelocity[0] = random() * 600;
	gib->avelocity[1] = random() * 600;
	gib->avelocity[2] = random() * 600;

	gib->think = G_FreeEdict;
	gib->nextthink = level.time + 10 + random() * 10;

	gi.linkentity(gib);
}

void
ThrowHeadACID(edict_t *self, const char *gibname, int damage, int type)
{
	vec3_t vd;
	float vscale;

	if (!self || !gibname)
	{
		return;
	}

	self->s.skinnum = 0;
	self->s.frame = 0;
	VectorClear(self->mins);
	VectorClear(self->maxs);

	self->s.modelindex2 = 0;
	gi.setmodel(self, gibname);

	self->clipmask = MASK_SHOT;
	self->solid = SOLID_BBOX;

	self->s.effects |= EF_GREENGIB;
	self->s.effects &= ~EF_FLIES;
	self->s.effects |= RF_FULLBRIGHT;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_MONSTER;
	self->takedamage = DAMAGE_YES;
	self->die = gib_die;
	self->dmg = 2;

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		vscale = 0.5;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, self->velocity);
	ClipGibVelocity(self);

	self->avelocity[YAW] = crandom() * 600;

	self->think = G_FreeEdict;
	self->nextthink = level.time + 10 + random() * 10;

	gi.linkentity(self);
}

void
ThrowClientHead(edict_t *self, int damage)
{
	vec3_t vd;
	char *gibname;

	if (!self)
	{
		return;
	}

	if (randk() & 1)
	{
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1; /* second skin is player */
	}
	else
	{
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel(self, gibname);
	VectorSet(self->mins, -16, -16, 0);
	VectorSet(self->maxs, 16, 16, 16);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_BBOX;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	// The entity still has the monsters clipmaks.
	// Reset it to MASK_SHOT to be on the save side.
	// (MASK_SHOT is used by xatrix)
	self->clipmask = MASK_SHOT;

	self->movetype = MOVETYPE_BOUNCE;
	VelocityForDamage(damage, vd);
	VectorAdd(self->velocity, vd, self->velocity);

	if (self->client) /* bodies in the queue don't have a client anymore */
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity(self);
}

/* ===================================================== */

void
debris_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */,
		int damage /* unused */, vec3_t point /* unused */)
{
	if (!self)
	{
		return;
	}

	G_FreeEdict(self);
}

void
ThrowDebris(edict_t *self, char *modelname, float speed, vec3_t origin)
{
	edict_t *chunk;
	vec3_t v;

	if (!self || !modelname)
	{
		return;
	}

	if (debristhisframe >= MAX_DEBRIS)
	{
		return;
	}

	chunk = G_SpawnOptional();

	if (!chunk)
	{
		return;
	}

	debristhisframe++;

	VectorCopy(origin, chunk->s.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random() * 5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;
	chunk->health = 250;
	gi.linkentity(chunk);
}

void
BecomeExplosion1(edict_t *self)
{
	if (!self)
	{
		return;
	}

	/* flags are important */
	if (strcmp(self->classname, "item_flag_team1") == 0)
	{
		CTFResetFlag(CTF_TEAM1); /* this will free self! */
		gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n",
				CTFTeamName(CTF_TEAM1));
		return;
	}

	if (strcmp(self->classname, "item_flag_team2") == 0)
	{
		CTFResetFlag(CTF_TEAM2); /* this will free self! */
		gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n",
				CTFTeamName(CTF_TEAM2));
		return;
	}

	/* techs are important too */
	if (self->item && (self->item->flags & IT_TECH))
	{
		CTFRespawnTech(self); /* this frees self! */
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void
BecomeExplosion2(edict_t *self)
{
	if (!self)
	{
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

/* ===================================================== */

/*
 * QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
 *
 * Target: next path corner
 * Pathtarget: gets used when an entity that has
 *  this path_corner targeted touches it
 */
void
path_corner_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	vec3_t v;
	edict_t *next;

	if (!self || !other)
	{
		return;
	}

	if (other->movetarget != self)
	{
		return;
	}

	if (other->enemy)
	{
		return;
	}

	if (self->pathtarget)
	{
		char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	if (self->target)
	{
		next = G_PickTarget(self->target);
	}
	else
	{
		next = NULL;
	}

	if ((next) && (next->spawnflags & 1))
	{
		VectorCopy(next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy(v, other->s.origin);
		next = G_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;

	if (self->wait)
	{
		other->monsterinfo.pausetime = level.time + self->wait;
		other->monsterinfo.stand(other);
		return;
	}

	if (!other->movetarget)
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.stand(other);
	}
	else
	{
		VectorSubtract(other->goalentity->s.origin, other->s.origin, v);
		other->ideal_yaw = vectoyaw(v);
	}
}

void
SP_path_corner(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->targetname)
	{
		gi.dprintf("path_corner with no targetname at %s\n",
				vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

/* ===================================================== */

/*
 * QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
 *
 * Makes this the target of a monster and it will head here
 * when first activated before going after the activator.  If
 * hold is selected, it will stay here.
 */
void
point_combat_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	edict_t *activator;
	vec3_t v;

	if (!self || !other)
	{
		return;
	}

	if (other->movetarget != self)
	{
		return;
	}

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = G_PickTarget(other->target);

		if (other->goalentity)
		{
			VectorSubtract(other->goalentity->s.origin, other->s.origin, v);
			other->ideal_yaw = vectoyaw(v);
		}
		else
		{
			gi.dprintf("%s at %s target %s does not exist\n",
					self->classname,
					vtos(self->s.origin),
					self->target);

			other->movetarget = self;
		}

		self->target = NULL;
	}
	else if ((self->spawnflags & 1) && !(other->flags & (FL_SWIM | FL_FLY)))
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand(other);
	}

	if (other->movetarget == self)
	{
		other->target = NULL;
		other->movetarget = NULL;
		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;

		if (other->enemy && other->enemy->client)
		{
			activator = other->enemy;
		}
		else if (other->oldenemy && other->oldenemy->client)
		{
			activator = other->oldenemy;
		}
		else if (other->activator && other->activator->client)
		{
			activator = other->activator;
		}
		else
		{
			activator = other;
		}

		G_UseTargets(self, activator);
		self->target = savetarget;
	}
}

void
SP_point_combat(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	VectorSet(self->mins, -8, -8, -16);
	VectorSet(self->maxs, 8, 8, 16);
	self->svflags = SVF_NOCLIENT;
	gi.linkentity(self);
}

/* ===================================================== */

/*
 * QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)
 *
 * Just for the debugging level.  Don't use
 */
void
viewthing_think(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->s.frame = (ent->s.frame + 1) % 7;
	ent->nextthink = level.time + FRAMETIME;

	if (ent->spawnflags)
	{
		if (ent->s.frame == 0)
		{
			ent->spawnflags = (ent->spawnflags + 1) % 4 + 1;
		}
	}
}

void
SP_viewthing(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.dprintf("viewthing spawned\n");

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.renderfx = RF_FRAMELERP;
	VectorSet(ent->mins, -16, -16, -24);
	VectorSet(ent->maxs, 16, 16, 32);
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	gi.linkentity(ent);
	ent->nextthink = level.time + 0.5;
	ent->think = viewthing_think;
	return;
}

/* ===================================================== */

/*
 * QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
 *
 * Used as a positional target for spotlights, etc.
 */
void
SP_info_null(edict_t *self)
{
	if (!self)
	{
		return;
	}

	G_FreeEdict(self);
}

/*
 * QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
 *
 * Used as a positional target for lighting.
 */
void
SP_info_notnull(edict_t *self)
{
	if (!self)
	{
		return;
	}

	VectorCopy(self->s.origin, self->absmin);
	VectorCopy(self->s.origin, self->absmax);
}

#define START_OFF 1

/*
 * QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
 *
 * Non-displayed light.
 * Default light value is 300.
 * Default style is 0.
 * If targeted, will toggle between on and off.
 * Default _cone value is 10 (used to set size of light for spotlights)
 */
void
light_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->spawnflags & START_OFF)
	{
		gi.configstring(CS_LIGHTS + self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring(CS_LIGHTS + self->style, "a");
		self->spawnflags |= START_OFF;
	}
}

void
SP_light(edict_t *self)
{
	if (!self)
	{
		return;
	}

	/* no targeted lights in deathmatch, because they cause global messages */
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;

		if (self->spawnflags & START_OFF)
		{
			gi.configstring(CS_LIGHTS + self->style, "a");
		}
		else
		{
			gi.configstring(CS_LIGHTS + self->style, "m");
		}
	}
}

/* ===================================================== */

/*
 * QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
 * This is just a solid wall if not inhibited
 *
 * TRIGGER_SPAWN	the wall will not be present until triggered
 *                  it will then blink in to existance; it will
 *                  kill anything that was in it's way
 *
 * TOGGLE			only valid for TRIGGER_SPAWN walls
 *                  this allows the wall to be turned on and off
 *
 * START_ON		only valid for TRIGGER_SPAWN walls
 *              the wall will initially be present
 */

void
func_wall_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		KillBox(self);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}

	gi.linkentity(self);

	if (!(self->spawnflags & 2))
	{
		self->use = NULL;
	}
}

void
SP_func_wall(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags & 8)
	{
		self->s.effects |= EF_ANIM_ALL;
	}

	if (self->spawnflags & 16)
	{
		self->s.effects |= EF_ANIM_ALLFAST;
	}

	/* just a wall */
	if ((self->spawnflags & 7) == 0)
	{
		self->solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	/* it must be TRIGGER_SPAWN */
	if (!(self->spawnflags & 1))
	{
		self->spawnflags |= 1;
	}

	/* yell if the spawnflags are odd */
	if (self->spawnflags & 4)
	{
		if (!(self->spawnflags & 2))
		{
			gi.dprintf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= 2;
		}
	}

	self->use = func_wall_use;

	if (self->spawnflags & 4)
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}

	gi.linkentity(self);
}

/* ===================================================== */

/*
 * QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
 *
 * This is solid bmodel that will fall if it's support it removed.
 */

void
func_object_touch(edict_t *self, edict_t *other, cplane_t *plane,
		csurface_t *surf /* unused */)
{
	/* only squash thing we fall on top of */

	if (!self || !other || !plane)
	{
		return;
	}

	/* only squash thing we fall on top of */
	if (plane && plane->normal[2] < 1.0)
	{
		return;
	}

	if (other->takedamage == DAMAGE_NO)
	{
		return;
	}

	T_Damage(other, self, self, vec3_origin, self->s.origin,
			vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void
func_object_release(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void
func_object_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox(self);
	func_object_release(self);
}

void
SP_func_object(edict_t *self)
{
	if (!self)
	{
		return;
	}

	gi.setmodel(self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
	{
		self->dmg = 100;
	}

	if (self->spawnflags == 0)
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 2 * FRAMETIME;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & 2)
	{
		self->s.effects |= EF_ANIM_ALL;
	}

	if (self->spawnflags & 4)
	{
		self->s.effects |= EF_ANIM_ALLFAST;
	}

	self->clipmask = MASK_MONSTERSOLID;

	gi.linkentity(self);
}

/* ===================================================== */

/*
 * QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST INACTIVE
 *
 * Any brush that you want to explode or break apart.  If you want an
 * explosion, set dmg and it will do a radius explosion of that amount
 * at the center of the bursh.
 *
 * If targeted it will not be shootable.
 *
 * INACTIVE - specifies that the entity is not explodable until triggered. If you use this you must
 * target the entity you want to trigger it. This is the only entity approved to activate it.
 *
 * health defaults to 100.
 *
 * mass defaults to 75.  This determines how much debris is emitted when
 * it explodes.  You get one large chunk per 100 of mass (up to 8) and
 * one small chunk per 25 of mass (up to 16).  So 800 gives the most.
 */
void
func_explosive_explode(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage /* unused */, vec3_t point /* unused */)
{
	vec3_t origin;
	vec3_t chunkorigin;
	vec3_t size;
	int count;
	int mass;
	edict_t *master;

	if (!self || !inflictor || !attacker)
	{
		return;
	}

	/* bmodel origins are (0 0 0), we need to adjust that here */
	VectorScale(self->size, 0.5, size);
	VectorAdd(self->absmin, size, origin);
	VectorCopy(origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
	{
		T_RadiusDamage(self, attacker, self->dmg, NULL,
				self->dmg + 40, MOD_EXPLOSIVE);
	}

	VectorSubtract(self->s.origin, inflictor->s.origin, self->velocity);
	VectorNormalize(self->velocity);
	VectorScale(self->velocity, 150, self->velocity);

	/* start chunks towards the center */
	VectorScale(size, 0.5, size);

	mass = self->mass;

	if (!mass)
	{
		mass = 75;
	}

	/* big chunks */
	if (mass >= 100)
	{
		count = mass / 100;

		if (count > 8)
		{
			count = 8;
		}

		while (count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris(self, "models/objects/debris1/tris.md2", 1, chunkorigin);
		}
	}

	/* small chunks */
	count = mass / 25;

	if (count > 16)
	{
		count = 16;
	}

	while (count--)
	{
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];
		ThrowDebris(self, "models/objects/debris2/tris.md2", 2, chunkorigin);
	}

	if (self->flags & FL_TEAMSLAVE)
	{
		master = self->teammaster;

		/* because mappers (other than jim (usually)) are stupid.... */
		while (master)
		{
			if (master->teamchain == self)
			{
				master->teamchain = self->teamchain;
				break;
			}

			master = master->teamchain;
		}
	}

	G_UseTargets(self, attacker);

	if (self->dmg)
	{
		BecomeExplosion1(self);
	}
	else
	{
		G_FreeEdict(self);
	}
}

void
func_explosive_use(edict_t *self, edict_t *other, edict_t *activator /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	func_explosive_explode(self, self, other, self->health, vec3_origin);
}

void
func_explosive_activate(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	self->use = func_explosive_use;

	if (!self->health)
	{
		self->health = 100;
	}

	self->die = func_explosive_explode;
	self->takedamage = DAMAGE_YES;
}

void
func_explosive_spawn(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox(self);
	gi.linkentity(self);
}

void
SP_func_explosive(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");

	gi.setmodel(self, self->model);

	if (self->spawnflags & 1)
	{
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	else if (self->spawnflags & 8)
	{
		self->solid = SOLID_BSP;

		if (self->targetname)
		{
			self->use = func_explosive_activate;
		}
	}
	else
	{
		self->solid = SOLID_BSP;

		if (self->targetname)
		{
			self->use = func_explosive_use;
		}
	}

	if (self->spawnflags & 2)
	{
		self->s.effects |= EF_ANIM_ALL;
	}

	if (self->spawnflags & 4)
	{
		self->s.effects |= EF_ANIM_ALLFAST;
	}

	if ((self->use != func_explosive_use) &&
		(self->use != func_explosive_activate))
	{
		if (!self->health)
		{
			self->health = 100;
		}

		self->die = func_explosive_explode;
		self->takedamage = DAMAGE_YES;
	}

	gi.linkentity(self);
}

/* ===================================================== */

/*
 * QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
 *
 * Large exploding box.  You can override its mass (100),
 * health (80), and dmg (150).
 */

void
barrel_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */, csurface_t *surf /*unused */)
{
	float ratio;
	vec3_t v;

	if (!self || !other)
	{
		return;
	}

	if ((!other->groundentity) || (other->groundentity == self))
	{
		return;
	}

	ratio = (float)other->mass / (float)self->mass;
	VectorSubtract(self->s.origin, other->s.origin, v);
	M_walkmove(self, vectoyaw(v), 20 * ratio * FRAMETIME);
}

void
barrel_explode(edict_t *self)
{
	vec3_t org;
	float spd;
	vec3_t save;

	if (!self)
	{
		return;
	}

	T_RadiusDamage(self, self->activator, self->dmg, NULL,
			self->dmg + 40, MOD_BARREL);
	VectorCopy(self->s.origin, save);
	VectorMA(self->absmin, 0.5, self->size, self->s.origin);

	/* a few big chunks */
	spd = 1.5 * (float)self->dmg / 200.0;
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);

	/* bottom corners */
	spd = 1.75 * (float)self->dmg / 200.0;
	VectorCopy(self->absmin, org);
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->absmin, org);
	org[0] += self->size[0];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->absmin, org);
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);

	/* a bunch of little chunks */
	spd = 2.0 * (float)self->dmg / 200.0;
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);

	VectorCopy(save, self->s.origin);

	if (self->groundentity)
	{
		BecomeExplosion2(self);
	}
	else
	{
		BecomeExplosion1(self);
	}
}

void
barrel_delay(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker,
		int damage /* unused */, vec3_t point /* unused */)
{
	if (!self || !attacker)
	{
		return;
	}

	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + 2 * FRAMETIME;
	self->think = barrel_explode;
	self->activator = attacker;
}

void
barrel_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	/* the think needs to be first since later stuff may override. */
	self->think = barrel_think;
	self->nextthink = level.time + FRAMETIME;

	M_CatagorizePosition(self);
	self->flags |= FL_IMMUNE_SLIME;
	self->air_finished = level.time + 100;
	M_WorldEffects(self);
}

void
barrel_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	M_droptofloor(self);
	self->think = barrel_think;
	self->nextthink = level.time + FRAMETIME;
}

void
SP_misc_explobox(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	self->model = "models/objects/barrels/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	VectorSet(self->mins, -16, -16, 0);
	VectorSet(self->maxs, 16, 16, 40);

	if (!self->mass)
	{
		self->mass = 400;
	}

	if (!self->health)
	{
		self->health = 10;
	}

	if (!self->dmg)
	{
		self->dmg = 150;
	}

	self->die = barrel_delay;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.aiflags = AI_NOSTEP;

	self->touch = barrel_touch;
	self->think = barrel_start;
	self->nextthink = level.time + 2 * FRAMETIME;

	gi.linkentity(self);
}


/* ===================================================== */

/*
 * QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8)
 */
void
misc_blackhole_use(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!ent)
	{
		return;
	}

	G_FreeEdict(ent);
}

void
misc_blackhole_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (++self->s.frame < 19)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		self->s.frame = 0;
		self->nextthink = level.time + FRAMETIME;
	}
}

void misc_blackhole_transparent (edict_t *ent)
{
	ent->s.renderfx = RF_TRANSLUCENT|RF_NOSHADOW;
	ent->prethink = NULL;
	gi.linkentity(ent);
}

void
SP_misc_blackhole(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	VectorSet(ent->mins, -64, -64, 0);
	VectorSet(ent->maxs, 64, 64, 8);
	ent->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->prethink = misc_blackhole_transparent;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
 */
void
misc_eastertank_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (++self->s.frame < 293)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		self->s.frame = 254;
		self->nextthink = level.time + FRAMETIME;
	}
}

void
SP_misc_eastertank(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, -16);
	VectorSet(ent->maxs, 32, 32, 32);
	ent->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	ent->s.frame = 254;
	ent->think = misc_eastertank_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
 */
void
misc_easterchick_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (++self->s.frame < 247)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		self->s.frame = 208;
		self->nextthink = level.time + FRAMETIME;
	}
}

void
SP_misc_easterchick(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, 0);
	VectorSet(ent->maxs, 32, 32, 32);
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->s.frame = 208;
	ent->think = misc_easterchick_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/*
 * QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
 */
void
misc_easterchick2_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (++self->s.frame < 287)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		self->s.frame = 248;
		self->nextthink = level.time + FRAMETIME;
	}
}

void
SP_misc_easterchick2(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, 0);
	VectorSet(ent->maxs, 32, 32, 32);
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->s.frame = 248;
	ent->think = misc_easterchick2_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
 *
 * Not really a monster, this is the Tank Commander's decapitated body.
 * There should be a item_commander_head that has this as it's target.
 */
void
commander_body_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (++self->s.frame < 24)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		self->nextthink = 0;
	}

	if (self->s.frame == 22)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex(
						"tank/thud.wav"), 1, ATTN_NORM, 0);
	}
}

void
commander_body_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	self->think = commander_body_think;
	self->nextthink = level.time + FRAMETIME;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);
}

void
commander_body_drop(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_TOSS;
	self->s.origin[2] += 2;
}

void
commander_body_die(edict_t *self, edict_t *inflictor /* unused */,
		edict_t *attacker /* unused */, int damage, vec3_t point /* unused */)
{
	int n;

	if (!self)
	{
		return;
	}

	/* check for gib */
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		for (n = 0; n < 4; n++)
		{
			ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		}

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		self->deadflag = DEAD_DEAD;

		return;
	}
}

void
SP_monster_commander_body(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	VectorSet(self->mins, -32, -32, 0);
	VectorSet(self->maxs, 32, 32, 48);
	self->use = commander_body_use;
	self->takedamage = DAMAGE_YES;
	self->s.renderfx |= RF_FRAMELERP;

	if (g_commanderbody_nogod->value)
	{
		self->deadflag = DEAD_DEAD;
		self->svflags |= SVF_MONSTER | SVF_DEADMONSTER;
		self->die = commander_body_die;
	}
	else
	{
		self->flags = FL_GODMODE;
	}

	gi.linkentity(self);

	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");

	self->think = commander_body_drop;
	self->nextthink = level.time + 5 * FRAMETIME;
}

/* ===================================================== */

/*
 * QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
 *
 * The origin is the bottom of the banner.
 * The banner is 128 tall.
 */
void
misc_banner_think(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + FRAMETIME;
}

void
SP_misc_banner(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->s.frame = randk() % 16;
	gi.linkentity(ent);

	ent->think = misc_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

/* ===================================================== */

/*
 * QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
 *
 * This is the dead player model. Comes in 6 exciting different poses!
 */
void
misc_deadsoldier_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */,
		int damage, vec3_t point /* unused */)
{
	int n;

	if (!self)
	{
		return;
	}

	if (self->health > -80)
	{
		return;
	}

	gi.sound(self, CHAN_BODY, gi.soundindex(
		"misc/udeath.wav"), 1, ATTN_NORM, 0);

	for (n = 0; n < 4; n++)
	{
		ThrowGib(self,
				"models/objects/gibs/sm_meat/tris.md2",
				damage,
				GIB_ORGANIC);
	}

	ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
}

void
SP_misc_deadsoldier(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	/* Defaults to frame 0 */
	if (ent->spawnflags & 2)
	{
		ent->s.frame = 1;
	}
	else if (ent->spawnflags & 4)
	{
		ent->s.frame = 2;
	}
	else if (ent->spawnflags & 8)
	{
		ent->s.frame = 3;
	}
	else if (ent->spawnflags & 16)
	{
		ent->s.frame = 4;
	}
	else if (ent->spawnflags & 32)
	{
		ent->s.frame = 5;
	}
	else
	{
		ent->s.frame = 0;
	}

	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 16);
	ent->deadflag = DEAD_DEAD;
	ent->takedamage = DAMAGE_YES;
	ent->svflags |= SVF_MONSTER | SVF_DEADMONSTER;
	ent->die = misc_deadsoldier_die;
	ent->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
 *
 * This is the Viper for the flyby bombing.
 * It is trigger_spawned, so you must have something use it for it to show up.
 * There must be a path for it to follow once it is activated.
 *
 * "speed"		How fast the Viper should fly
 */

extern void train_use(edict_t *self, edict_t *other, edict_t *activator);
extern void func_train_find(edict_t *self);

void
misc_viper_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self || !other || !activator)
	{
		return;
	}

	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void
SP_misc_viper(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!ent->target)
	{
		gi.dprintf("misc_viper without a target at %s\n", vtos(ent->absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
	{
		ent->speed = 300;
	}

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 32);

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = misc_viper_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed =
													ent->speed;

	gi.linkentity(ent);
}

/*
 * QUAKED misc_crashviper (1 .5 0) (-176 -120 -24) (176 120 72)
 * This is a large viper about to crash
 */
void
SP_misc_crashviper(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!ent->target)
	{
		gi.dprintf("misc_viper without a target at %s\n", vtos(ent->absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
	{
		ent->speed = 300;
	}

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 32);

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = misc_viper_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*
 * QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72)
 *
 * This is a large stationary viper as seen in Paul's intro
 */
void
SP_misc_bigviper(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -176, -120, -24);
	VectorSet(ent->maxs, 176, 120, 72);
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * "dmg"	how much boom should the bomb make?
 */
void
misc_viper_bomb_touch(edict_t *self, edict_t *other /* unused */, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self)
	{
		return;
	}

	G_UseTargets(self, self->activator);

	self->s.origin[2] = self->absmin[2] + 1;
	T_RadiusDamage(self, self, self->dmg, NULL, self->dmg + 40, MOD_BOMB);
	BecomeExplosion2(self);
}

void
misc_viper_bomb_prethink(edict_t *self)
{
	vec3_t v;
	float diff;

	if (!self)
	{
		return;
	}

	self->groundentity = NULL;

	diff = self->timestamp - level.time;

	if (diff < -1.0)
	{
		diff = -1.0;
	}

	VectorScale(self->moveinfo.dir, 1.0 + diff, v);
	v[2] = diff;

	diff = self->s.angles[2];
	vectoangles(v, self->s.angles);
	self->s.angles[2] = diff + 10;
}

void
misc_viper_bomb_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	edict_t *viper;

	if (!self || !activator)
	{
		return;
	}

	self->solid = SOLID_BBOX;
	self->svflags &= ~SVF_NOCLIENT;
	self->s.effects |= EF_ROCKET;
	self->use = NULL;
	self->movetype = MOVETYPE_TOSS;
	self->prethink = misc_viper_bomb_prethink;
	self->touch = misc_viper_bomb_touch;
	self->activator = activator;

	viper = G_Find(NULL, FOFS(classname), "misc_viper");
	VectorScale(viper->moveinfo.dir, viper->moveinfo.speed, self->velocity);

	self->timestamp = level.time;
	VectorCopy(viper->moveinfo.dir, self->moveinfo.dir);
}

void
SP_misc_viper_bomb(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	if (!self->dmg)
	{
		self->dmg = 1000;
	}

	self->use = misc_viper_bomb_use;
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*
 * QUAKED misc_viper_missile (1 0 0) (-8 -8 -8) (8 8 8)
 * "dmg"	how much boom should the bomb make? the default value is 250
 */

void
misc_viper_missile_use(edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t forward, right, up;
	vec3_t start, dir;
	vec3_t vec;

	if (!self)
	{
		return;
	}

	AngleVectors(self->s.angles, forward, right, up);

	self->enemy = G_Find(NULL, FOFS(targetname), self->target);

	VectorCopy(self->enemy->s.origin, vec);

	VectorCopy(self->s.origin, start);
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	monster_fire_rocket(self, start, dir, self->dmg, 500, MZ2_CHICK_ROCKET_1);

	self->nextthink = level.time + 0.1;
	self->think = G_FreeEdict;
}

void
SP_misc_viper_missile(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);

	if (!self->dmg)
	{
		self->dmg = 250;
	}

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	self->use = misc_viper_missile_use;
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*
 * QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
 *
 * This is a Storgg ship for the flybys.
 * It is trigger_spawned, so you must have something use it for it to show up.
 * There must be a path for it to follow once it is activated.
 *
 * "speed"		How fast it should fly
 */

extern void train_use(edict_t *self, edict_t *other, edict_t *activator);
extern void func_train_find(edict_t *self);

void
misc_strogg_ship_use(edict_t *self, edict_t *other /* other */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void
SP_misc_strogg_ship(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!ent->target)
	{
		gi.dprintf("%s without a target at %s\n", ent->classname,
				vtos(ent->absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
	{
		ent->speed = 300;
	}

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 32);

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = misc_strogg_ship_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel =
		ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*
 * QUAKED misc_transport (1 0 0) (-8 -8 -8) (8 8 8) TRIGGER_SPAWN
 * Maxx's transport at end of game
 */
void
SP_misc_transport(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!ent->target)
	{
		gi.dprintf("%s without a target at %s\n", ent->classname,
				vtos(ent->absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
	{
		ent->speed = 300;
	}

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/ship/tris.md2");

	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 32);

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = misc_strogg_ship_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	if (!(ent->spawnflags & 1))
	{
		ent->spawnflags |= 1;
	}

	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
 */
void
misc_satellite_dish_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.frame++;

	if (self->s.frame < 38)
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

void
misc_satellite_dish_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	self->s.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextthink = level.time + FRAMETIME;
}

void
SP_misc_satellite_dish(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -64, -64, 0);
	VectorSet(ent->maxs, 64, 64, 128);
	ent->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
 */
void
SP_light_mine1(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex =
		gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.linkentity(ent);
}

/*
 * QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
 */
void
SP_light_mine2(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex =
		gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * Intended for use with the target_spawner
 */
void
SP_misc_gib_arm(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent->solid = SOLID_BBOX;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity(ent);
}

/*
 * QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
 * Intended for use with the target_spawner
 */
void
SP_misc_gib_leg(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent->solid = SOLID_BBOX;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity(ent);
}

/*
 * QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
 * Intended for use with the target_spawner
 */
void
SP_misc_gib_head(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent->solid = SOLID_BBOX;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity(ent);
}

/* ===================================================== */

/*
 * QUAKED target_character (0 0 1) ?
 *
 * used with target_string (must be on same "team")
 * "count" is position in the string (starts at 1)
 */

void
SP_target_character(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity(self);
	return;
}

/* ===================================================== */

/*
 * QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
 */
void
target_string_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	edict_t *e;
	size_t l;
	int n;
	char c;

	if (!self)
	{
		return;
	}

	l = strlen(self->message);

	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
		{
			continue;
		}

		n = e->count - 1;

		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];

		if ((c >= '0') && (c <= '9'))
		{
			e->s.frame = c - '0';
		}
		else if (c == '-')
		{
			e->s.frame = 10;
		}
		else if (c == ':')
		{
			e->s.frame = 11;
		}
		else
		{
			e->s.frame = 12;
		}
	}
}

void
SP_target_string(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->message)
	{
		self->message = "";
	}

	self->use = target_string_use;
}

/* ===================================================== */

/*
 * QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
 *
 * target a target_string with this
 *
 * The default is to be a time of day clock
 *
 * TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
 * If START_OFF, this entity must be used before it starts
 *
 * "style"	0 "xx"
 *          1 "xx:xx"
 *          2 "xx:xx:xx"
 */

#define CLOCK_MESSAGE_SIZE 16

/* don't let field width of any clock messages change, or it
   could cause an overwrite after a game load */

void
func_clock_reset(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->activator = NULL;

	if (self->spawnflags & 1)
	{
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & 2)
	{
		self->health = self->count;
		self->wait = 0;
	}
}

/*
 * This is an evil hack to
 * prevent a rare crahs at
 * biggun exit. */
typedef struct zhead_s
{
   struct zhead_s *prev, *next;
   short magic;
   short tag;
   int size;
} zhead_t;

void
func_clock_format_countdown(edict_t *self)
{
	if (!self)
	{
		return;
	}

	zhead_t *z = ( zhead_t * )self->message - 1;
	int size = z->size - sizeof (zhead_t);

	if (size < CLOCK_MESSAGE_SIZE)
	{
		gi.TagFree (self->message);
		self->message = gi.TagMalloc (CLOCK_MESSAGE_SIZE, TAG_LEVEL);
	}

	if (self->style == 0)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i",
				self->health / 60, self->health % 60);

		if (self->message[3] == ' ')
		{
			self->message[3] = '0';
		}

		return;
	}

	if (self->style == 2)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i",
				self->health / 3600,
				(self->health - (self->health / 3600) * 3600) / 60,
				self->health % 60);

		if (self->message[3] == ' ')
		{
			self->message[3] = '0';
		}

		if (self->message[6] == ' ')
		{
			self->message[6] = '0';
		}

		return;
	}
}

void
func_clock_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->enemy)
	{
		self->enemy = G_Find(NULL, FOFS(targetname), self->target);

		if (!self->enemy)
		{
			return;
		}
	}

	if (self->spawnflags & 1)
	{
		func_clock_format_countdown(self);
		self->health++;
	}
	else if (self->spawnflags & 2)
	{
		func_clock_format_countdown(self);
		self->health--;
	}
	else
	{
		struct tm *ltime;
		time_t gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i",
				ltime->tm_hour, ltime->tm_min, ltime->tm_sec);

		if (self->message[3] == ' ')
		{
			self->message[3] = '0';
		}

		if (self->message[6] == ' ')
		{
			self->message[6] = '0';
		}
	}

	self->enemy->message = self->message;
	self->enemy->use(self->enemy, self, self);

	if (((self->spawnflags & 1) && (self->health > self->wait)) ||
		((self->spawnflags & 2) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			char *savetarget;
			char *savemessage;

			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			G_UseTargets(self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & 8))
		{
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
			return;
		}

		func_clock_reset(self);

		if (self->spawnflags & 4)
		{
			return;
		}
	}

	self->nextthink = level.time + 1;
}

void
func_clock_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	if (!(self->spawnflags & 8))
	{
		self->use = NULL;
	}

	if (self->activator)
	{
		return;
	}

	self->activator = activator;
	self->think(self);
}

void
SP_func_clock(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname,
				vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if ((self->spawnflags & 2) && (!self->count))
	{
		gi.dprintf("%s with no count at %s\n", self->classname,
				vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if ((self->spawnflags & 1) && (!self->count))
	{
		self->count = 60 * 60;
	}

	func_clock_reset(self);

	self->message = gi.TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);

	self->think = func_clock_think;

	if (self->spawnflags & 4)
	{
		self->use = func_clock_use;
	}
	else
	{
		self->nextthink = level.time + 1;
	}
}

/* ================================================================================= */

void
teleporter_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	edict_t *dest;
	int i;

	if (!self || !other)
	{
		return;
	}

	if (!other->client)
	{
		return;
	}

	dest = G_Find(NULL, FOFS(targetname), self->target);

	if (!dest)
	{
		gi.dprintf("Couldn't find destination\n");
		return;
	}

	CTFPlayerResetGrapple(other);

	/* unlink to make sure it can't possibly interfere with KillBox */
	gi.unlinkentity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	VectorCopy(dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	/* clear the velocity and hold them in place briefly */
	VectorClear(other->velocity);
	other->client->ps.pmove.pm_time = 160 >> 3;       /* hold time */
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	/* draw the teleport splash at source and on the player */
	self->owner->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	/* set angles */
	for (i = 0; i < 3; i++)
	{
		other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
				dest->s.angles[i] - other->client->resp.cmd_angles[i]);
	}

	VectorClear(other->s.angles);
	VectorClear(other->client->ps.viewangles);
	VectorClear(other->client->v_angle);

	/* kill anything at the destination */
	KillBox(other);

	gi.linkentity(other);
}

/*
 * QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
 *
 * Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
 */
void
SP_misc_teleporter(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	edict_t *trig;

	if (!ent->target)
	{
		gi.dprintf("teleporter without a target.\n");
		G_FreeEdict(ent);
		return;
	}

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 1;
	ent->s.effects = EF_TELEPORTER;
	ent->s.sound = gi.soundindex("world/amb10.wav");
	ent->solid = SOLID_BBOX;

	VectorSet(ent->mins, -32, -32, -24);
	VectorSet(ent->maxs, 32, 32, -16);
	gi.linkentity(ent);

	trig = G_Spawn();
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	VectorCopy(ent->s.origin, trig->s.origin);
	VectorSet(trig->mins, -8, -8, 8);
	VectorSet(trig->maxs, 8, 8, 24);
	gi.linkentity(trig);
}

/*
 * QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
 *
 * Point teleporters at these.
 */
void
SP_misc_teleporter_dest(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 0;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, -24);
	VectorSet(ent->maxs, 32, 32, -16);
	gi.linkentity(ent);
}

/*
 * QUAKED misc_amb4 (1 0 0) (-16 -16 -16) (16 16 16)
 * Mal's amb4 loop entity
 */
static int amb4sound;

void
amb4_think(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->nextthink = level.time + 2.0;
	gi.sound(ent, CHAN_VOICE, amb4sound, 1, ATTN_NONE, 0);
}

void
SP_misc_amb4(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->think = amb4_think;
	ent->nextthink = level.time + 1;
	amb4sound = gi.soundindex("world/amb4.wav");
	gi.linkentity(ent);
}

/*
 * QUAKED misc_nuke (1 0 0) (-16 -16 -16) (16 16 16)
 */
void
use_nuke(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *from = g_edicts;

	if (!self)
	{
		return;
	}

	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (from == self)
		{
			continue;
		}

		if (from->client)
		{
			T_Damage(from, self, self, vec3_origin, from->s.origin,
					vec3_origin, 100000, 1, 0, MOD_TRAP);
		}
		else if (from->svflags & SVF_MONSTER)
		{
			G_FreeEdict(from);
		}
	}

	self->use = NULL;
}

void
SP_misc_nuke(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->use = use_nuke;
}

void
misc_nuke_core_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->svflags & SVF_NOCLIENT)
	{
		self->svflags &= ~SVF_NOCLIENT;
	}
	else
	{
		self->svflags |= SVF_NOCLIENT;
	}
}

/*
 * QUAKED misc_nuke_core (1 0 0) (-16 -16 -16) (16 16 16)
 *
 * toggles visible/not visible. starts visible.
 */
void
SP_misc_nuke_core(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.setmodel(ent, "models/objects/core/tris.md2");
	gi.linkentity(ent);

	ent->use = misc_nuke_core_use;
}

/*
 * QUAKED misc_flare (1.0 1.0 0.0) (-32 -32 -32) (32 32 32) RED GREEN BLUE LOCK_ANGLE
 * Creates a flare seen in the N64 version.
 */
#define SPAWNFLAG_FLARE_RED 1
#define SPAWNFLAG_FLARE_GREEN 2
#define SPAWNFLAG_FLARE_BLUE 4
#define SPAWNFLAG_FLARE_LOCK_ANGLE 8

void
misc_flare_use(edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags ^= SVF_NOCLIENT;
	gi.linkentity(ent);
}

void
SP_misc_flare(edict_t* ent)
{
	int i;

	ent->s.modelindex = gi.modelindex("misc/flare.tga");
	ent->s.frame = 0;
	ent->s.renderfx = RF_FLARE;
	ent->solid = SOLID_NOT;

	/* Radius saved to scale */
	for (i = 0; i < 3; i++)
	{
		ent->rrs.scale[i] = st.radius;
	}

	if (ent->spawnflags & SPAWNFLAG_FLARE_RED)
	{
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->spawnflags & SPAWNFLAG_FLARE_GREEN)
	{
		ent->s.renderfx |= RF_SHELL_GREEN;
	}

	if (ent->spawnflags & SPAWNFLAG_FLARE_BLUE)
	{
		ent->s.renderfx |= RF_SHELL_BLUE;
	}

	if (ent->spawnflags & SPAWNFLAG_FLARE_LOCK_ANGLE)
	{
		ent->s.renderfx |= RF_FLARE_LOCK_ANGLE;
	}

	if (st.image && *st.image)
	{
		ent->s.renderfx |= RF_CUSTOMSKIN;
		ent->s.modelindex = gi.modelindex(st.image);
	}

	VectorSet(ent->mins, -32, -32, -32);
	VectorSet(ent->maxs, 32, 32, 32);

	ent->s.modelindex2 = st.fade_start_dist;
	ent->s.modelindex3 = st.fade_end_dist;
	ent->s.skinnum = st.rgba;
	if (!ent->s.skinnum)
	{
		ent->s.skinnum = -1;
	}

	if (ent->targetname)
	{
		ent->use = misc_flare_use;
	}

	gi.linkentity(ent);
}

void
misc_player_mannequin_use(edict_t * self, edict_t * other, edict_t * activator)
{
	self->monsterinfo.aiflags |= AI_TARGET_ANGER;
	self->enemy = activator;

	switch ( self->count )
	{
		case GESTURE_FLIP_OFF:
			self->s.frame = FRAME_flip01;
			self->monsterinfo.nextframe = FRAME_flip12;
			break;

		case GESTURE_SALUTE:
			self->s.frame = FRAME_salute01;
			self->monsterinfo.nextframe = FRAME_salute11;
			break;

		case GESTURE_TAUNT:
			self->s.frame = FRAME_taunt01;
			self->monsterinfo.nextframe = FRAME_taunt17;
			break;

		case GESTURE_WAVE:
			self->s.frame = FRAME_wave01;
			self->monsterinfo.nextframe = FRAME_wave11;
			break;

		case GESTURE_POINT:
			self->s.frame = FRAME_point01;
			self->monsterinfo.nextframe = FRAME_point12;
			break;
	}
}

void
misc_player_mannequin_think(edict_t * self)
{
	if (self->last_sound_time <= level.time)
	{
		self->s.frame++;

		if ((self->monsterinfo.aiflags & AI_TARGET_ANGER) == 0)
		{
			if (self->s.frame > FRAME_stand40)
			{
				self->s.frame = FRAME_stand01;
			}
		}
		else
		{
			if (self->s.frame > self->monsterinfo.nextframe)
			{
				self->s.frame = FRAME_stand01;
				self->monsterinfo.aiflags &= ~AI_TARGET_ANGER;
				self->enemy = NULL;
			}
		}

		self->last_sound_time = level.time + FRAMETIME;
	}

	if (self->enemy)
	{
		vec3_t vec;

		VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
		self->ideal_yaw = vectoyaw(vec);
		M_ChangeYaw(self);
	}

	self->nextthink = level.time + FRAMETIME;
}

void
SetupMannequinModel(edict_t * self, int modelType, const char *weapon, const char *skin)
{
	const char *model_name = NULL;
	const char *default_skin = NULL;

	switch (modelType)
	{
		case 1:
			{
				self->s.skinnum = (MAX_CLIENTS - 1);
				model_name = "female";
				default_skin = "venus";
				break;
			}

		case 2:
			{
				self->s.skinnum = (MAX_CLIENTS - 2);
				model_name = "male";
				default_skin = "rampage";
				break;
			}

		case 3:
			{
				self->s.skinnum = (MAX_CLIENTS - 3);
				model_name = "cyborg";
				default_skin = "oni911";
				break;
			}

		default:
			{
				self->s.skinnum = (MAX_CLIENTS - 1);
				model_name = "female";
				default_skin = "venus";
				break;
			}
	}

	if (model_name)
	{
		char line[MAX_QPATH] = {0};

		snprintf(line, sizeof(line), "players/%s/tris.md2", model_name);
		self->model = ED_NewString(line, true);

		if (weapon)
		{
			snprintf(line, sizeof(line), "players/%s/%s.md2", model_name, weapon);
		}
		else
		{
			snprintf(line, sizeof(line), "players/%s/w_hyperblaster.md2", model_name);
		}
		self->s.modelindex2 = gi.modelindex(line);

		if (skin)
		{
			snprintf(line, sizeof(line), "%s/%s", model_name, skin);
		}
		else
		{
			snprintf(line, sizeof(line), "%s/%s", model_name, default_skin);
		}
		gi.configstring(CS_PLAYERSKINS + self->s.skinnum, line);
	}
}

/*
 * QUAKED misc_player_mannequin (1.0 1.0 0.0) (-32 -32 -32) (32 32 32)
 * Creates a player mannequin that stands around.
 *
 * NOTE: this is currently very limited, and only allows one unique model
 * from each of the three player model types.
 *
 *
 * "distance"		- Sets the type of gesture mannequin when use when triggered
 * "height"		- Sets the type of model to use ( valid numbers: 1 - 3 )
 * "goals"		- Name of the weapon to use.
 * "image"		- Name of the player skin to use.
 * "radius"		- How much to scale the model in-game
 */
void
SP_misc_player_mannequin(edict_t * self)
{
	int i;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	if (!st.effects)
	{
		self->s.effects = 0;
	}

	if (!st.renderfx)
	{
		self->s.renderfx = RF_MINLIGHT;
	}

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);
	self->yaw_speed = 30;
	self->ideal_yaw = 0;
	self->last_sound_time = level.time + FRAMETIME;
	self->s.modelindex = CUSTOM_PLAYER_MODEL;
	self->count = st.distance;

	SetupMannequinModel(self, st.height, st.goals, st.image);

	VectorSet(self->rrs.scale, 1.0f, 1.0f, 1.0f);
	if (ai_model_scale->value > 0.0f)
	{
		VectorSet(self->rrs.scale,
			ai_model_scale->value, ai_model_scale->value, ai_model_scale->value);
	}
	else if (st.radius > 0.0f)
	{
		VectorSet(self->rrs.scale,
			st.radius, st.radius, st.radius);
	}

	for (i = 0;i < 3; i++)
	{
		self->mins[i] *= self->rrs.scale[i];
		self->maxs[i] *= self->rrs.scale[i];
	}

	self->think = misc_player_mannequin_think;
	self->nextthink = level.time + FRAMETIME;

	if (self->targetname)
	{
		self->use = misc_player_mannequin_use;
	}

	gi.linkentity(self);
}

/*
 * QUAKED misc_model (1 0 0) (-8 -8 -8) (8 8 8)
 */
void SP_misc_model(edict_t *ent)
{
	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
}

/*
 * QUAKED npc_timeminder (0 1 0) (-8 -8 -8) (8 8 8)
 *
 * Anachronox: Save menu open.
 */
void
touch_npc_timeminder(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	gi.AddCommandString("menu_savegame\n");
}

void
npc_timeminder_think(edict_t *self)
{
	M_SetAnimGroupFrame(self, "amb_b");
	self->nextthink = level.time + FRAMETIME;
}

void
SP_npc_timeminder(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->touch = touch_npc_timeminder;
	self->think = npc_timeminder_think;

	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}

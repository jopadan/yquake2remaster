/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../../header/local.h"
#include "ogre.h"

static int sound_death;
static int sound_attack;
static int sound_melee;
static int sound_sight;
static int sound_search;
static int sound_idle;
static int sound_pain;

static void
ogre_idle(edict_t *self)
{
	if (random() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

// Stand
static mframe_t ogre_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},

	{ai_stand, 0, ogre_idle},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},

	{ai_stand, 0, NULL},
};
mmove_t ogre_move_stand =
{
	FRAME_stand1,
	FRAME_stand9,
	ogre_frames_stand,
	NULL
};

void
ogre_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_move_stand;
}

// Run
static mframe_t ogre_frames_run [] =
{
	{ai_run, 9, NULL},
	{ai_run, 12, NULL},
	{ai_run, 8, NULL},
	{ai_run, 22, NULL},
	{ai_run, 16, NULL},

	{ai_run, 4, NULL},
	{ai_run, 13, NULL},
	{ai_run, 24, NULL}
};
mmove_t ogre_move_run =
{
	FRAME_run1,
	FRAME_run8,
	ogre_frames_run,
	NULL
};

void
ogre_run(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_move_run;
}

static void
OgreChainsaw(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
		return;
	damage = (random() + random() + random()) * 4;

	fire_hit(self, aim, damage, damage);
}

// Smash
static mframe_t ogre_frames_smash [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 1, NULL},
	{ai_charge, 4, NULL},

	{ai_charge, 14, OgreChainsaw},
	{ai_charge, 14, OgreChainsaw},
	{ai_charge, 20, OgreChainsaw},
	{ai_charge, 23, OgreChainsaw},

	{ai_charge, 10, OgreChainsaw},
	{ai_charge, 12, OgreChainsaw},
	{ai_charge, 1, NULL},
	{ai_charge, 4, NULL},

	{ai_charge, 12, NULL},
	{ai_charge, 0, NULL}
};
mmove_t ogre_move_smash =
{
	FRAME_smash1,
	FRAME_smash14,
	ogre_frames_smash,
	ogre_run
};

// Swing
static mframe_t ogre_frames_swing [] =
{
	{ai_charge, 11, NULL},
	{ai_charge, 1, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 19, OgreChainsaw},

	{ai_charge, 13, OgreChainsaw},
	{ai_charge, 10, OgreChainsaw},
	{ai_charge, 10, OgreChainsaw},
	{ai_charge, 10, OgreChainsaw},

	{ai_charge, 10, OgreChainsaw},
	{ai_charge, 10, OgreChainsaw},
	{ai_charge, 3, NULL},
	{ai_charge, 8, NULL},

	{ai_charge, 9, NULL},
	{ai_charge, 0, NULL}
};
mmove_t ogre_move_swing =
{
	FRAME_swing1,
	FRAME_swing14,
	ogre_frames_swing,
	ogre_run
};

// Melee
void
ogre_melee(edict_t *self)
{
	if (random() > 0.5)
		self->monsterinfo.currentmove = &ogre_move_smash;
	else
		self->monsterinfo.currentmove = &ogre_move_swing;
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

static void
FireOgreGrenade(edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	aim;
	vec3_t offset = {0, 0, 16};

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(forward, aim);

	monster_fire_grenade(self, start, aim, 40, 600, MZ2_GUNNER_GRENADE_1);
}

// Grenade
static mframe_t ogre_frames_attack [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, FireOgreGrenade},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t ogre_move_attack =
{
	FRAME_shoot1,
	FRAME_shoot6,
	ogre_frames_attack,
	ogre_run
};

void
ogre_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_move_attack;
}

// Pain (1)
static mframe_t ogre_frames_pain1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL}
};
mmove_t ogre_move_pain1 =
{
	FRAME_pain1,
	FRAME_pain5,
	ogre_frames_pain1,
	ogre_run
};

// Pain (2)
static mframe_t ogre_frames_pain2 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t ogre_move_pain2 =
{
	FRAME_painb1,
	FRAME_painb3,
	ogre_frames_pain2,
	ogre_run
};

// Pain (3)
static mframe_t ogre_frames_pain3 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t ogre_move_pain3 =
{
	FRAME_painc1,
	FRAME_painc6,
	ogre_frames_pain3,
	ogre_run
};

// Pain (4)
static mframe_t ogre_frames_pain4 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 10, NULL},
	{ai_move, 9, NULL},
	{ai_move, 4, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t ogre_move_pain4 =
{
	FRAME_paind1,
	FRAME_paind16,
	ogre_frames_pain4,
	ogre_run
};

// Pain (5)
static mframe_t ogre_frames_pain5 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 10, NULL},
	{ai_move, 9, NULL},
	{ai_move, 4, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t ogre_move_pain5 =
{
	FRAME_paine1,
	FRAME_paine15,
	ogre_frames_pain5,
	ogre_run
};

// Pain
void
ogre_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{
	float r;

	// decino: No pain animations in Nightmare mode
	if (skill->value == SKILL_HARDPLUS)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	r = random();
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (r < 0.25)
	{
		self->monsterinfo.currentmove = &ogre_move_pain1;
		self->pain_debounce_time = level.time + 1.0;
	}
	else if (r < 0.5)
	{
		self->monsterinfo.currentmove = &ogre_move_pain2;
		self->pain_debounce_time = level.time + 1.0;
	}
	else if (r < 0.75)
	{
		self->monsterinfo.currentmove = &ogre_move_pain3;
		self->pain_debounce_time = level.time + 1.0;
	}
	else if (r < 0.88)
	{
		self->monsterinfo.currentmove = &ogre_move_pain4;
		self->pain_debounce_time = level.time + 2.0;
	}
	else
	{
		self->monsterinfo.currentmove = &ogre_move_pain5;
		self->pain_debounce_time = level.time + 2.0;
	}
}

void
ogre_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
static mframe_t ogre_frames_death1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t ogre_move_death1 =
{
	FRAME_death1,
	FRAME_death14,
	ogre_frames_death1,
	ogre_dead
};

// Death (2)
static mframe_t ogre_frames_death2 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 5, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},

	{ai_move, 3, NULL},
	{ai_move, 7, NULL},
	{ai_move, 25, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t ogre_move_death2 =
{
	FRAME_bdeath1,
	FRAME_bdeath10,
	ogre_frames_death2,
	ogre_dead
};

// Death
void
ogre_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}
	if (self->deadflag == DEAD_DEAD)
		return;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	if (random() < 0.5)
		self->monsterinfo.currentmove = &ogre_move_death1;
	else
		self->monsterinfo.currentmove = &ogre_move_death2;
}

// Sight
void
ogre_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void
ogre_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

/*
 * QUAKED monster_ogre (1 .5 0) (-32, -32, -24) (32, 32, 64) Ambush Trigger_Spawn Sight
 */
void
SP_monster_ogre(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/ogre/tris.md2");
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->health = 200 * st.health_multiplier;

	sound_death = gi.soundindex("ogre/ogdth.wav");
	sound_attack = gi.soundindex("ogre/grenade.wav");
	sound_melee = gi.soundindex("ogre/ogsawatk.wav");
	sound_sight = gi.soundindex("ogre/ogwake.wav");
	sound_search = gi.soundindex("ogre/ogidle2.wav");
	sound_idle = gi.soundindex("ogre/ogidle.wav");
	sound_pain = gi.soundindex("ogre/ogpain1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -80;
	self->mass = 200;

	self->monsterinfo.stand = ogre_stand;
	self->monsterinfo.walk = ogre_run;
	self->monsterinfo.run = ogre_run;
	self->monsterinfo.attack = ogre_attack;
	self->monsterinfo.melee = ogre_melee;
	self->monsterinfo.sight = ogre_sight;
	self->monsterinfo.search = ogre_search;

	self->pain = ogre_pain;
	self->die = ogre_die;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}

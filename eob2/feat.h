/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "nameable.h"

enum featn : unsigned char {
	NoExeptionalStrenght,
	BonusSaveVsPoison, BonusSaveVsSpells,
	BonusVsElfWeapon, BonusAttackVsHated, BonusDamageVsEnemy, BonusACVsLargeEnemy, BonusHP,
	UseMetal, UseLeather, UseShield,
	UseMartial, UseElvish, UseRogish, UsePriest, UseMage, TheifSkills, StealthSkills, Sneaky, Alertness,
	TwoHanded, Precise, Deadly, Unique,
	DiseaseAttack, DispelEvilAttack, DrainStrenghtAttack, DrainEneryAttack, ParalizeAttack, PoisonAttack, VampiricAttack, VorpalAttack, Holy, WeaponSpecialist,
	Small, Large, Undead, Outsider, Paralized, Moved, Surprised,
	ProtectedFromEvil, Invisibled, Regenerated, Hasted, StoppedPoison, SlowMove, FeelPain, Displaced,
	Blinded, Blinked, Blurred, Paniced,
	ResistBludgeon, ResistSlashing, ResistPierce, ResistFire, ResistCold, ResistCharm, ResistSpells,
	ImmuneNormalWeapon, ImmuneFire, ImmuneFear, ImmuneCold, ImmuneCharm, ImmuneParalize, ImmunePoison, ImmuneSpells, ImmuneDisease,
	Enemy, Ally, Group, You, SummaryEffect,
	SeeMagical, SeeCursed, ImmuneIllusion,
};
struct feati : nameable {
};
struct featable {
	unsigned feats[4];
	bool is(featn v) const { return (feats[v / 32] & (1 << (v % 32))) != 0; }
	void remove(featn v) { feats[v / 32] &= ~(1 << (v % 32)); }
	void set(featn v) { feats[v / 32] |= (1 << (v % 32)); }
	void set(featn v, bool apply) { if(apply) set(v); else remove(v); }
};

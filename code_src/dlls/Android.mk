#HLSDK server Android port
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := server

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libserver_hardfp
endif

LOCAL_CFLAGS += -D_LINUX -DCLIENT_WEAPONS -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf \
	-fno-exceptions

LOCAL_CPPFLAGS := $(LOCAL_CFLAGS) -frtti

LOCAL_C_INCLUDES := ../cl_dll/ \
	 ../common/ \
	  ../engine/ \
        ../pm_shared/ \
        ../dlls/ \
        ../game_shared/ \
        ../public/

LOCAL_SRC_FILES := \
aflock.cpp \
agrunt.cpp \
AI_BaseNPC_Schedule.cpp \
airtank.cpp \
alias.cpp \
animating.cpp \
animation.cpp \
apache.cpp \
barnacle.cpp \
barney.cpp \
bigmomma.cpp \
bloater.cpp \
bmodels.cpp \
bullsquid.cpp \
buttons.cpp \
cbase.cpp \
client.cpp \
combat.cpp \
controller.cpp \
crossbow.cpp \
crowbar.cpp \
defaultai.cpp \
doors.cpp \
effects.cpp \
egon.cpp \
explode.cpp \
flyingmonster.cpp \
func_break.cpp \
func_tank.cpp \
game.cpp \
gamerules.cpp \
gargantua.cpp \
gauss.cpp \
genericmonster.cpp \
ggrenade.cpp \
globals.cpp \
gman.cpp \
h_ai.cpp \
h_battery.cpp \
h_cine.cpp \
h_cycler.cpp \
h_export.cpp \
handgrenade.cpp \
hassassin.cpp \
headcrab.cpp \
healthkit.cpp \
hgrunt.cpp \
hornet.cpp \
hornetgun.cpp \
houndeye.cpp \
ichthyosaur.cpp \
islave.cpp \
items.cpp \
leech.cpp \
lights.cpp \
locus.cpp \
maprules.cpp \
monstermaker.cpp \
monsters.cpp \
monsterstate.cpp \
mortar.cpp \
movewith.cpp \
mp5.cpp \
multiplay_gamerules.cpp \
nihilanth.cpp \
nodes.cpp \
osprey.cpp \
paranoia_ak47.cpp \
paranoia_aks.cpp \
paranoia_aps.cpp \
paranoia_glock.cpp \
paranoia_groza.cpp \
paranoia_military.cpp \
paranoia_rpk.cpp \
paranoia_shotgun.cpp \
paranoia_turret.cpp \
paranoia_val.cpp \
paranoia_wpn.cpp \
pathcorner.cpp \
plane.cpp \
plats.cpp \
player.cpp \
python.cpp \
roach.cpp \
rpg.cpp \
satchel.cpp \
scientist.cpp \
scripted.cpp \
singleplay_gamerules.cpp \
skill.cpp \
sound.cpp \
soundent.cpp \
spectator.cpp \
squadmonster.cpp \
squeakgrenade.cpp \
subs.cpp \
talkmonster.cpp \
teamplay_gamerules.cpp \
tempmonster.cpp \
tentacle.cpp \
triggers.cpp \
tripmine.cpp \
turret.cpp \
util.cpp \
../game_shared/voice_gamemgr.cpp \
weapons.cpp \
world.cpp \
xen.cpp \
zombie.cpp \
../pm_shared/pm_debug.c \
../pm_shared/pm_math.c \
../pm_shared/pm_shared.c

include $(BUILD_SHARED_LIBRARY)

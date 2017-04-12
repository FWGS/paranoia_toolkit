#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 2.6.0)
project (PARANOIA_DLL)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -Wno-conversion-null -Wl,--no-undefined -ggdb")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -O3")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb -O3")

set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ")

# force build 32bit libraries
if(CMAKE_SIZEOF_VOID_P MATCHES "8")
    if(APPLE)
        SET(CMAKE_OSX_ARCHITECTURES "i386")
    else()
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif()
endif()

set (PARANOIA_DLL_SHARED "paranoia" )

set (PARANOIA_DLL_SRC
aflock.cpp
agrunt.cpp
AI_BaseNPC_Schedule.cpp
airtank.cpp
alias.cpp
animating.cpp
animation.cpp
apache.cpp
barnacle.cpp
barney.cpp
bigmomma.cpp
bloater.cpp
bmodels.cpp
bullsquid.cpp
buttons.cpp
cbase.cpp
client.cpp
combat.cpp
controller.cpp
crossbow.cpp
crowbar.cpp
defaultai.cpp
doors.cpp
effects.cpp
egon.cpp
explode.cpp
flyingmonster.cpp
func_break.cpp
func_tank.cpp
game.cpp
gamerules.cpp
gargantua.cpp
gauss.cpp
genericmonster.cpp
ggrenade.cpp
globals.cpp
gman.cpp
h_ai.cpp
h_battery.cpp
h_cine.cpp
h_cycler.cpp
h_export.cpp
handgrenade.cpp
hassassin.cpp
headcrab.cpp
healthkit.cpp
hgrunt.cpp
hornet.cpp
hornetgun.cpp
houndeye.cpp
ichthyosaur.cpp
islave.cpp
items.cpp
leech.cpp
lights.cpp
locus.cpp
maprules.cpp
monstermaker.cpp
monsters.cpp
monsterstate.cpp
mortar.cpp
movewith.cpp
mp5.cpp
multiplay_gamerules.cpp
nihilanth.cpp
nodes.cpp
osprey.cpp
paranoia_ak47.cpp
paranoia_aks.cpp
paranoia_aps.cpp
paranoia_glock.cpp
paranoia_groza.cpp
paranoia_military.cpp
paranoia_rpk.cpp
paranoia_shotgun.cpp
paranoia_turret.cpp
paranoia_val.cpp
paranoia_wpn.cpp
pathcorner.cpp
plane.cpp
plats.cpp
player.cpp
../pm_shared/pm_debug.c
../pm_shared/pm_math.c
../pm_shared/pm_shared.c
python.cpp
roach.cpp
rpg.cpp
satchel.cpp
scientist.cpp
scripted.cpp
singleplay_gamerules.cpp
skill.cpp
sound.cpp
soundent.cpp
spectator.cpp
squadmonster.cpp
squeakgrenade.cpp
subs.cpp
talkmonster.cpp
teamplay_gamerules.cpp
tempmonster.cpp
tentacle.cpp
triggers.cpp
tripmine.cpp
turret.cpp
util.cpp
../game_shared/voice_gamemgr.cpp
weapons.cpp
world.cpp
xen.cpp
zombie.cpp
activity.h
activitymap.h
animation.h
basemonster.h
cbase.h
cdll_dll.h
client.h
decals.h
defaultai.h
doors.h
effects.h
../engine/eiface.h
enginecallback.h
explode.h
extdll.h
flyingmonster.h
func_break.h
gamerules.h
hornet.h
items.h
locus.h
monsterevent.h
monsters.h
movewith.h
nodes.h
paranoia_wpn.h
plane.h
player.h
../pm_shared/pm_debug.h
../pm_shared/pm_defs.h
../pm_shared/pm_info.h
../pm_shared/pm_materials.h
../pm_shared/pm_movevars.h
../pm_shared/pm_shared.h
rushscript.h
saverestore.h
schedule.h
scripted.h
scriptevent.h
skill.h
soundent.h
spectator.h
squadmonster.h
talkmonster.h
teamplay_gamerules.h
trains.h
util.h
vector.h
weapons.h
)

include_directories (
	../cl_dll/
	../common/
	../engine/
	../pm_shared/
	../dlls/
	../game_shared/
	../public/)
add_library (${PARANOIA_DLL_SHARED} SHARED ${PARANOIA_DLL_SRC})
add_definitions( -DLINUX -D_LINUX -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp )
#add_defintions( -DUSE_VOICEGAMEMGR )
target_link_libraries( ${PARANOIA_DLL_SHARED} ${CMAKE_DL_LIBS})

set_target_properties (${PARANOIA_DLL_SHARED} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

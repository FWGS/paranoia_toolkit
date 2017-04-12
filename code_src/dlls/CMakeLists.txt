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
set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -w -Wl,--no-undefined -ggdb")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -O3")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb -O3")

set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -static-libstdc++")
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
activity.h
activitymap.h
aflock.cpp
agrunt.cpp
AI_BaseNPC_Schedule.cpp
airtank.cpp
alias.cpp
animating.cpp
animation.cpp
animation.h
apache.cpp
barnacle.cpp
barney.cpp
basemonster.h
bigmomma.cpp
bloater.cpp
bmodels.cpp
bullsquid.cpp
buttons.cpp
cbase.cpp
cbase.h
cdll_dll.h
client.cpp
client.h
combat.cpp
controller.cpp
crossbow.cpp
crowbar.cpp
decals.h
defaultai.cpp
defaultai.h
doors.cpp
doors.h
effects.cpp
effects.h
egon.cpp
enginecallback.h
explode.cpp
explode.h
extdll.h
flyingmonster.cpp
flyingmonster.h
func_break.cpp
func_break.h
func_tank.cpp
game.cpp
game.h
gamerules.cpp
gamerules.h
gargantua.cpp
gauss.cpp
genericmonster.cpp
ggrenade.cpp
globals.cpp
gman.cpp
h_ai.cpp
handgrenade.cpp
hassassin.cpp
h_battery.cpp
h_cine.cpp
h_cycler.cpp
headcrab.cpp
healthkit.cpp
h_export.cpp
hgrunt.cpp
hornet.cpp
hornetgun.cpp
hornet.h
houndeye.cpp
ichthyosaur.cpp
islave.cpp
items.cpp
items.h
leech.cpp
lights.cpp
locus.cpp
locus.h
maprules.cpp
maprules.h
monsterevent.h
monstermaker.cpp
monsters.cpp
monsters.h
monsterstate.cpp
mortar.cpp
movewith.cpp
movewith.h
mp5.cpp
multiplay_gamerules.cpp
nihilanth.cpp
nodes.cpp
nodes.h
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
paranoia_wpn.h
pathcorner.cpp
plane.cpp
plane.h
plats.cpp
player.cpp
player.h
playermonster.cpp
python.cpp
roach.cpp
rpg.cpp
rushscript.h
satchel.cpp
saverestore.h
schedule.h
scientist.cpp
scripted.cpp
scripted.h
scriptevent.h
#shotgun.cpp
singleplay_gamerules.cpp
skill.cpp
skill.h
sound.cpp
soundent.cpp
soundent.h
spectator.cpp
spectator.h
squad.h
squadmonster.cpp
squadmonster.h
squeakgrenade.cpp
stats.cpp
subs.cpp
talkmonster.cpp
talkmonster.h
teamplay_gamerules.cpp
teamplay_gamerules.h
tempmonster.cpp
tentacle.cpp
trains.h
triggers.cpp
tripmine.cpp
turret.cpp
util.cpp
util.h
vector.h
weapons.cpp
weapons.h
world.cpp
xen.cpp
zombie.cpp
../pm_shared/pm_shared.c 
../pm_shared/pm_math.c
../pm_shared/pm_debug.c
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
add_definitions( -DLINUX -D_LINUX -D_DEBUG  -D_CS16CLIENT_ALLOW_SPECIAL_SCRIPTING
	-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp )
target_link_libraries( ${PARANOIA_DLL_SHARED} ${CMAKE_DL_LIBS})

set_target_properties (${PARANOIA_DLL_SHARED} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

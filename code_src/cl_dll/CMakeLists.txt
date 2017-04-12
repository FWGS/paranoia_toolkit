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

set (PARANOIA_DLL_SHARED "client" )

set (PARANOIA_DLL_SRC
ev_hldm.cpp
hl/hl_events.cpp
hl/hl_objects.cpp
hl/hl_weapons.cpp
../common/interface.cpp
../game_shared/vgui_scrollbar2.cpp
../game_shared/vgui_slider2.cpp
../game_shared/voice_banmgr.cpp
../game_shared/voice_status.cpp
../game_shared/voice_vgui_tweakdlg.cpp
ammo.cpp
ammo_secondary.cpp
ammohistory.cpp
battery.cpp
cdll_int.cpp
death.cpp
demo.cpp
entity.cpp
ev_common.cpp
events.cpp
exception.cpp
flashlight.cpp
GameStudioModelRenderer.cpp
geiger.cpp
gl_debug.cpp
gl_decals.cpp
gl_light_dynamic.cpp
gl_lightmap.cpp
gl_model.cpp
gl_postprocess.cpp
gl_renderer.cpp
gl_rsurf.cpp
gl_sky.cpp
gl_studiomodel.cpp
gl_texloader.cpp
glmanager.cpp
glows.cpp
grass.cpp
health.cpp
hud.cpp
hud_msg.cpp
hud_redraw.cpp
hud_servers.cpp
hud_spectator.cpp
hud_update.cpp
in_camera.cpp
input.cpp
#inputw32.cpp
input_xash3d.cpp
log.cpp
menu.cpp
message.cpp
miniparticles.cpp
mp3.cpp
parsemsg.cpp
particlemgr.cpp
particlemsg.cpp
particlesys.cpp
../pm_shared/pm_debug.c
../pm_shared/pm_math.c
../pm_shared/pm_shared.c
quake_bsp.cpp
rain.cpp
saytext.cpp
status_icons.cpp
statusbar.cpp
studio_util.cpp
StudioModelRenderer.cpp
text_message.cpp
train.cpp
tri.cpp
triapiobjects.cpp
util.cpp
../game_shared/vgui_checkbutton2.cpp
vgui_ClassMenu.cpp
vgui_ConsolePanel.cpp
vgui_ControlConfigPanel.cpp
vgui_CustomObjects.cpp
vgui_gamma.cpp
../game_shared/vgui_grid.cpp
../game_shared/vgui_helpers.cpp
vgui_hud.cpp
vgui_int.cpp
../game_shared/vgui_listbox.cpp
../game_shared/vgui_loadtga.cpp
vgui_MOTDWindow.cpp
vgui_paranoiatext.cpp
vgui_radio.cpp
vgui_SchemeManager.cpp
vgui_ScorePanel.cpp
vgui_ServerBrowser.cpp
vgui_SpectatorPanel.cpp
vgui_subtitles.cpp
vgui_tabpanel.cpp
vgui_TeamFortressViewport.cpp
vgui_teammenu.cpp
vgui_tips.cpp
view.cpp
ammo.h
ammohistory.h
camera.h
cl_dll.h
com_weapons.h
custom_alloc.h
demo.h
ev_hldm.h
eventscripts.h
exception.h
GameStudioModelRenderer.h
getfont.h
gl_dlight.h
gl_renderer.h
gl_texloader.h
glmanager.h
grass.h
health.h
hud.h
hud_iface.h
hud_servers.h
hud_servers_priv.h
hud_spectator.h
in_defs.h
kbutton.h
log.h
mp3.h
overview.h
parsemsg.h
particlemgr.h
particlesys.h
../pm_shared/pm_debug.h
../pm_shared/pm_defs.h
../pm_shared/pm_info.h
../pm_shared/pm_materials.h
../pm_shared/pm_movevars.h
../pm_shared/pm_shared.h
quake_bsp.h
rain.h
studio_util.h
StudioModelRenderer.h
triapiobjects.h
util_vector.h
vgui_ConsolePanel.h
vgui_ControlConfigPanel.h
vgui_gamma.h
vgui_hud.h
vgui_int.h
vgui_paranoiatext.h
vgui_pickup.h
vgui_radio.h
vgui_SchemeManager.h
vgui_ScorePanel.h
vgui_screenmsg.h
vgui_ServerBrowser.h
vgui_shadowtext.h
vgui_SpectatorPanel.h
vgui_subtitles.h
vgui_tabpanel.h
vgui_TeamFortressViewport.h
vgui_tips.h
view.h
../game_shared/voice_banmgr.h
../game_shared/voice_status.h
../game_shared/voice_vgui_tweakdlg.h
wrect.h
)

include_directories (
	../cl_dll/
	../common/
	../engine/
	../pm_shared/
	../dlls/
	../game_shared/
	../public/
	../utils/vgui/include)
add_library (${PARANOIA_DLL_SHARED} SHARED ${PARANOIA_DLL_SRC})
add_definitions( -DLINUX -D_LINUX -D_DEBUG -DCLIENT_DLL
	-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf)
add_custom_command(TARGET ${PARANOIA_DLL_SHARED} PRE_LINK COMMAND
	${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../utils/vgui/lib/linux/vgui.so $<TARGET_SONAME_FILE_DIR:${PARANOIA_DLL_SHARED}>)
target_link_libraries(${PARANOIA_DLL_SHARED} -L. -l:vgui.so ${CMAKE_DL_LIBS})

set_target_properties (${PARANOIA_DLL_SHARED} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

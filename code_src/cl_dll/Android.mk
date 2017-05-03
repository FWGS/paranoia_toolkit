#HLSDK server Android port
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := client

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libclient_hardfp
endif

LOCAL_CFLAGS += -fsigned-char -DVGUI_TOUCH_SCROLL -DNO_STL -DXASH_GLES -DINTERNAL_VGUI_SUPPORT -DLINUX -DCLIENT_DLL -DCLIENT_WEAPONS -DHL_DLL -I/usr/include/malloc -D_snwprintf=swprintf -DDISABLE_JUMP_ORIGIN -DDISABLE_VEC_ORIGIN -fpermissive -fno-strict-aliasing -DNDEBUG -DPOSIX -D_POSIX -DLINUX -D_LINUX -DGNUC -DHL1 -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -DQUIVER -DQUAKE2 -DVALVE_DLL -D_alloca=alloca -fno-exceptions -fexpensive-optimizations -D_vsnprintf=vsnprintf -DNO_MALLOC_OVERRIDE -Werror=return-type

LOCAL_CPPFLAGS := $(LOCAL_CFLAGS) -frtti -fexceptions -Wno-write-strings -Wno-invalid-offsetof -std=gnu++98

HL1_DIR = .
HL1_SERVER_DIR = ../dlls
PUBLIC_DIR = ../public
COMMON_DIR = ../common
GAME_SHARED_DIR = ../game_shared
PM_SHARED_DIR = ../pm_shared

LOCAL_C_INCLUDES := \
		    $(LOCAL_PATH)/. \
		    $(LOCAL_PATH)/../dlls/ \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../public \
		    $(LOCAL_PATH)/../pm_shared \
		    $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../game_shared \
		    $(LOCAL_PATH)/../utils/vgui/include \
		    $(LOCAL_PATH)/../external \
		    $(VGUI_DIR)/lib-src/win32 \
		    $(VGUI_DIR)/lib-src/vgui \
		    $(VGUI_SUPPORT_PATH)/../common \
		    $(VGUI_SUPPORT_PATH)/../engine \
                    $(NANOGL_PATH)/ 


LOCAL_SRC_FILES := \
	ev_hldm.cpp \
	hl/hl_events.cpp \
	hl/hl_objects.cpp \
	hl/hl_weapons.cpp \
	../common/interface.cpp \
	../game_shared/vgui_scrollbar2.cpp \
	../game_shared/vgui_slider2.cpp \
	../game_shared/voice_banmgr.cpp \
	../game_shared/voice_status.cpp \
	../game_shared/voice_vgui_tweakdlg.cpp \
	ammo.cpp \
	ammo_secondary.cpp \
	ammohistory.cpp \
	battery.cpp \
	cdll_int.cpp \
	death.cpp \
	demo.cpp \
	entity.cpp \
	ev_common.cpp \
	events.cpp \
	exception.cpp \
	flashlight.cpp \
	GameStudioModelRenderer.cpp \
	geiger.cpp \
	gl_debug.cpp \
	gl_decals.cpp \
	gl_light_dynamic.cpp \
	gl_lightmap.cpp \
	gl_model.cpp \
	gl_postprocess.cpp \
	gl_renderer.cpp \
	gl_rsurf.cpp \
	gl_sky.cpp \
	gl_studiomodel.cpp \
	gl_texloader.cpp \
	glmanager.cpp \
	glows.cpp \
	grass.cpp \
	health.cpp \
	hud.cpp \
	hud_msg.cpp \
	hud_redraw.cpp \
	hud_servers.cpp \
	hud_spectator.cpp \
	hud_update.cpp \
	in_camera.cpp \
	input.cpp \
	input_xash3d.cpp \
	log.cpp \
	menu.cpp \
	message.cpp \
	miniparticles.cpp \
	mp3.cpp \
	parsemsg.cpp \
	particlemgr.cpp \
	particlemsg.cpp \
	particlesys.cpp \
	quake_bsp.cpp \
	rain.cpp \
	saytext.cpp \
	status_icons.cpp \
	statusbar.cpp \
	studio_util.cpp \
	StudioModelRenderer.cpp \
	text_message.cpp \
	train.cpp \
	tri.cpp \
	triapiobjects.cpp \
	util.cpp \
	../game_shared/vgui_checkbutton2.cpp \
	vgui_ClassMenu.cpp \
	vgui_ConsolePanel.cpp \
	vgui_ControlConfigPanel.cpp \
	vgui_CustomObjects.cpp \
	vgui_gamma.cpp \
	../game_shared/vgui_grid.cpp \
	../game_shared/vgui_helpers.cpp \
	vgui_hud.cpp \
	vgui_int.cpp \
	../game_shared/vgui_listbox.cpp \
	../game_shared/vgui_loadtga.cpp \
	vgui_MOTDWindow.cpp \
	vgui_paranoiatext.cpp \
	vgui_radio.cpp \
	vgui_SchemeManager.cpp \
	vgui_ScorePanel.cpp \
	vgui_ServerBrowser.cpp \
	vgui_SpectatorPanel.cpp \
	vgui_subtitles.cpp \
	vgui_tabpanel.cpp \
	vgui_TeamFortressViewport.cpp \
	vgui_teammenu.cpp \
	vgui_tips.cpp \
	view.cpp \
	../pm_shared/pm_shared.c \
	../pm_shared/pm_math.c

LOCAL_STATIC_LIBRARIES := vgui vgui_support

include $(BUILD_SHARED_LIBRARY)

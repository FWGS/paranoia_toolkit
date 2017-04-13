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
VGUI_DIR ?= ../../../vgui_dll/
VGUI_SUPPORT_DIR ?= ../../../xash3d/vgui_support/

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
		    $(VGUI_SUPPORT_DIR)/../common \
		    $(VGUI_SUPPORT_DIR)/../engine \
		    $(LOCAL_PATH)/$(VGUI_DIR)/lib-src/win32 \
		    $(LOCAL_PATH)/$(VGUI_DIR)/lib-src/vgui \
		    $(LOCAL_PATH)/$(VGUI_SUPPORT_DIR)/../common \
		    $(LOCAL_PATH)/$(VGUI_SUPPORT_DIR)/../engine
		    



VGUI_SRCS = $(VGUI_DIR)/lib-src/linux/App.cpp $(VGUI_DIR)/lib-src/linux/Cursor.cpp $(VGUI_DIR)/lib-src/linux/fileimage.cpp $(VGUI_DIR)/lib-src/linux/Font.cpp $(VGUI_DIR)/lib-src/linux/Surface.cpp $(VGUI_DIR)/lib-src/linux/vfontdata.cpp $(VGUI_DIR)/lib-src/linux/vgui.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/App.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Bitmap.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/BitmapTGA.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Border.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/BorderLayout.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/BorderPair.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/BuildGroup.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Button.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ButtonGroup.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/CheckButton.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Color.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ConfigWizard.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Cursor.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/DataInputStream.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Desktop.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/DesktopIcon.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/EditPanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/EtchedBorder.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/FileInputStream.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/FlowLayout.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/FocusNavGroup.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Font.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Frame.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/GridLayout.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/HeaderPanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Image.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ImagePanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/IntLabel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Label.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Layout.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/LineBorder.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ListPanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/LoweredBorder.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Menu.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/MenuItem.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/MenuSeparator.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/MessageBox.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/MiniApp.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Panel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/PopupMenu.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ProgressBar.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/RadioButton.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/RaisedBorder.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Scheme.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ScrollBar.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ScrollPanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Slider.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/StackLayout.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/String.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/SurfaceBase.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/Surface.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TablePanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TabPanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TaskBar.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TextEntry.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TextGrid.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TextImage.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TextPanel.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/ToggleButton.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/TreeFolder.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/vgui.cpp $(VGUI_DIR)/../vgui_dll/lib-src/vgui/WizardPanel.cpp

VGUI_SUPPORT_SRCS := $(VGUI_SUPPORT_DIR)/vgui_clip.cpp $(VGUI_SUPPORT_DIR)/vgui_font.cpp $(VGUI_SUPPORT_DIR)/vgui_input.cpp $(VGUI_SUPPORT_DIR)/vgui_int.cpp $(VGUI_SUPPORT_DIR)/vgui_surf.cpp

SRCS :=
SRCS += ./ev_hldm.cpp
SRCS += ./hl/hl_events.cpp
SRCS += ./hl/hl_objects.cpp
SRCS += ./hl/hl_weapons.cpp
SRCS += ../common/interface.cpp
SRCS += ../game_shared/vgui_scrollbar2.cpp
SRCS += ../game_shared/vgui_slider2.cpp
SRCS += ../game_shared/voice_banmgr.cpp
SRCS += ../game_shared/voice_status.cpp
SRCS += ../game_shared/voice_vgui_tweakdlg.cpp
SRCS += ./ammo.cpp
SRCS += ./ammo_secondary.cpp
SRCS += ./ammohistory.cpp
SRCS += ./battery.cpp
SRCS += ./cdll_int.cpp
SRCS += ./death.cpp
SRCS += ./demo.cpp
SRCS += ./entity.cpp
SRCS += ./ev_common.cpp
SRCS += ./events.cpp
SRCS += ./exception.cpp
SRCS += ./flashlight.cpp
SRCS += ./GameStudioModelRenderer.cpp
SRCS += ./geiger.cpp
SRCS += ./gl_debug.cpp
SRCS += ./gl_decals.cpp
SRCS += ./gl_light_dynamic.cpp
SRCS += ./gl_lightmap.cpp
SRCS += ./gl_model.cpp
SRCS += ./gl_postprocess.cpp
SRCS += ./gl_renderer.cpp
SRCS += ./gl_rsurf.cpp
SRCS += ./gl_sky.cpp
SRCS += ./gl_studiomodel.cpp
SRCS += ./gl_texloader.cpp
SRCS += ./glmanager.cpp
SRCS += ./glows.cpp
SRCS += ./grass.cpp
SRCS += ./health.cpp
SRCS += ./hud.cpp
SRCS += ./hud_msg.cpp
SRCS += ./hud_redraw.cpp
SRCS += ./hud_servers.cpp
SRCS += ./hud_spectator.cpp
SRCS += ./hud_update.cpp
SRCS += ./in_camera.cpp
SRCS += ./input.cpp
SRCS += ./input_xash3d.cpp
SRCS += ./log.cpp
SRCS += ./menu.cpp
SRCS += ./message.cpp
SRCS += ./miniparticles.cpp
SRCS += ./mp3.cpp
SRCS += ./parsemsg.cpp
SRCS += ./particlemgr.cpp
SRCS += ./particlemsg.cpp
SRCS += ./particlesys.cpp
SRCS += ./quake_bsp.cpp
SRCS += ./rain.cpp
SRCS += ./saytext.cpp
SRCS += ./status_icons.cpp
SRCS += ./statusbar.cpp
SRCS += ./studio_util.cpp
SRCS += ./StudioModelRenderer.cpp
SRCS += ./text_message.cpp
SRCS += ./train.cpp
SRCS += ./tri.cpp
SRCS += ./triapiobjects.cpp
SRCS += ./util.cpp
SRCS += ../game_shared/vgui_checkbutton2.cpp
SRCS += ./vgui_ClassMenu.cpp
SRCS += ./vgui_ConsolePanel.cpp
SRCS += ./vgui_ControlConfigPanel.cpp
SRCS += ./vgui_CustomObjects.cpp
SRCS += ./vgui_gamma.cpp
SRCS += ../game_shared/vgui_grid.cpp
SRCS += ../game_shared/vgui_helpers.cpp
SRCS += ./vgui_hud.cpp
SRCS += ./vgui_int.cpp
SRCS += ../game_shared/vgui_listbox.cpp
SRCS += ../game_shared/vgui_loadtga.cpp
SRCS += ./vgui_MOTDWindow.cpp
SRCS += ./vgui_paranoiatext.cpp
SRCS += ./vgui_radio.cpp
SRCS += ./vgui_SchemeManager.cpp
SRCS += ./vgui_ScorePanel.cpp
SRCS += ./vgui_ServerBrowser.cpp
SRCS += ./vgui_SpectatorPanel.cpp
SRCS += ./vgui_subtitles.cpp
SRCS += ./vgui_tabpanel.cpp
SRCS += ./vgui_TeamFortressViewport.cpp
SRCS += ./vgui_teammenu.cpp
SRCS += ./vgui_tips.cpp
SRCS += ./view.cpp


LOCAL_SRC_FILES := $(SRCS) $(VGUI_SRCS) $(VGUI_SUPPORT_SRCS) ../pm_shared/pm_shared.c ../pm_shared/pm_math.c

include $(BUILD_SHARED_LIBRARY)

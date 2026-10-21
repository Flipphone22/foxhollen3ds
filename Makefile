DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
LIBCTRU ?= $(DEVKITPRO)/libctru
PORTLIBS ?= $(DEVKITPRO)/portlibs/3ds

PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
GCC := $(PREFIX)gcc
GXX := $(PREFIX)g++
AR := $(PREFIX)ar
OBJCOPY := $(PREFIX)objcopy
CLANG ?= clang
PICASSO ?= $(DEVKITPRO)/tools/bin/picasso
3DSXTOOL ?= $(DEVKITPRO)/tools/bin/3dsxtool
MAKEROM ?= makerom
BANNERTOOL ?= bannertool
PYTHON ?= python3

BUILD ?= build
OBJ := $(BUILD)/obj
GEN := $(BUILD)/gen
OUT := $(BUILD)/app

BUILD_TAG ?= $(shell git -c safe.directory='*' rev-parse HEAD 2>/dev/null | sha256sum | cut -c1-64)
BUILD_ID := $(shell printf %.12s $(BUILD_TAG))
PREFIX_MAP ?= -fmacro-prefix-map=$(CURDIR)/=

.DEFAULT_GOAL := all
.SECONDARY:
.DELETE_ON_ERROR:

# Game library

GAME_UNITS := $(shell sed -e '/^\#/d' -e '/^[[:space:]]*$$/d' port/compiling.txt)
GAME_OBJS := $(addprefix $(OBJ)/game/src/,$(GAME_UNITS:.c=.o))

ARM_CLANG := --target=arm-none-eabi -mcpu=mpcore -mfpu=vfp -mfloat-abi=hard
GAME_CFLAGS := $(ARM_CLANG) -mtp=soft -isystem $(DEVKITARM)/arm-none-eabi/include -std=gnu11 -O2 -g \
	-fno-short-enums -fsigned-char -fcommon -fno-strict-aliasing -fwrapv -ffp-contract=off \
	-ffunction-sections -fdata-sections -DTARGET_PC=1 -DFOXHOLLOW_N3DS=1 -DBUILD_VERSION=0 \
	-DVERSION_GSAE01 -DNDEBUG=1 -DMUSY_TARGET=MUSY_TARGET_PC -DMUSY_VERSION_MAJOR=2 \
	-DMUSY_VERSION_MINOR=0 -DMUSY_VERSION_PATCH=0 -Wno-incompatible-pointer-types \
	-Wno-pointer-sign -Wno-unused-value $(PREFIX_MAP) -include $(CURDIR)/port/include/foxhollow_compat.h \
	-I$(CURDIR)/native-3ds/include -I$(CURDIR)/port/include -I$(CURDIR)/extern/aurora/include \
	-I$(CURDIR)/game/include -I$(CURDIR)/game -I$(CURDIR)/extern/musyx/include -I$(PORTLIBS)/include

$(OBJ)/game/%.o: game/%.c
	@mkdir -p $(@D)
	$(CLANG) $(GAME_CFLAGS) -MMD -MP -c $(CURDIR)/$< -o $@

$(BUILD)/libgame.a: $(GAME_OBJS)
	@rm -f $@
	$(AR) rcsD $@ $^

# Application support library

APP_DEFINES := -DFH_AUDIO_DSP_COMMAND_CAPACITY=256 -DFH_REPLAY_BUILD=1 -DFH_UNIFIED_APP=1 -DFH_CPU_PANEL_BUILD=1
MUSYX := extern/musyx/src/musyx/runtime
MUSYX_UNITS := seq synth seq_api snd_synthapi stream synthdata synthmacros synthvoice synth_ac synth_adsr \
	synth_vsamples synth_dbtab s_data hw_dspctrl hw_volconv snd3d snd_init snd_math snd_midictrl \
	snd_service hardware hw_aramdma hw_dolphin hw_pc_output hw_pc_rate hw_pc_vsample hw_pc_song \
	hw_pc_samples hw_pc_assets hw_pc hw_memory hw_lib_dolphin CheapReverb/creverb_fx \
	CheapReverb/creverb StdReverb/reverb_fx StdReverb/reverb Delay/delay_fx Chorus/chorus_fx
SHIMS := game_data_gen game_endian game_math_port model_anim_joints foxhollow_breadcrumb \
	foxhollow_config os_font_shim os_stopwatch_shim ppc_shim thp_shim

NATIVE_C := $(sort $(wildcard native-3ds/source/*.c))
NATIVE_PLATFORM := $(filter native-3ds/source/platform_%,$(NATIVE_C))
SUPPORT_SRCS := $(addprefix port/src/,$(addsuffix .c,$(SHIMS))) \
	$(sort $(wildcard extern/aurora/lib/dolphin/mtx/*.c)) \
	$(addprefix $(MUSYX)/,$(addsuffix .c,$(MUSYX_UNITS))) \
	$(filter-out $(NATIVE_PLATFORM) native-3ds/source/diagnostic.c,$(NATIVE_C)) \
	$(sort $(wildcard native-3ds/source/*.S)) \
	$(NATIVE_PLATFORM)
SUPPORT_OBJS := $(addprefix $(OBJ)/,$(addsuffix .o,$(basename $(SUPPORT_SRCS))))

GCC_UNITS := $(filter native-3ds/source/audio_% native-3ds/source/platform_%,$(SUPPORT_SRCS)) \
	$(addprefix native-3ds/source/,pica_distortion_geometry.c pica_distortion_draw.c pica_actor_fog_tev.c \
	install_worker.c pica_water_material.c pica_water_grid.c pica_ripple_reduce.c pica_draw_stream.c \
	pica_draw_resident.c pica_command_chain.c pica_world_packet.c pica_actor_commands.c)
GCC_OBJS := $(addprefix $(OBJ)/,$(GCC_UNITS:.c=.o))

GCC_CFLAGS := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -O2 -g -std=gnu11 -Wall -Wextra \
	-Werror -ffunction-sections -fdata-sections $(PREFIX_MAP) -I$(LIBCTRU)/include -I$(CURDIR)/native-3ds/include \
	-I$(CURDIR)/$(GEN) $(APP_DEFINES)
MUSYX_CFLAGS := -fwrapv -ffp-contract=off -fsigned-char -fno-short-enums -DTARGET_PC=1 -DMUSY_TARGET=MUSY_TARGET_PC \
	-DMUSY_VERSION_MAJOR=2 -DMUSY_VERSION_MINOR=0 -DMUSY_VERSION_PATCH=0 -I$(CURDIR)/extern/musyx/include
PORTLIBS_UNITS := $(addprefix $(OBJ)/native-3ds/source/,platform_model_lod.o platform_texture_pack.o \
	platform_native_model.o platform_native_scene.o platform_actor_fog_resident.o platform_replay.o \
	platform_input_stream.o platform_pica_hud.o)

$(filter $(OBJ)/native-3ds/source/audio_% $(OBJ)/native-3ds/source/platform_audio_game_%,$(GCC_OBJS)): EXTRA := $(MUSYX_CFLAGS)
$(PORTLIBS_UNITS): EXTRA := -I$(PORTLIBS)/include
$(GEN)/build-tag: FORCE
	@mkdir -p $(@D)
	@echo $(BUILD_TAG) | cmp -s - $@ || echo $(BUILD_TAG) > $@
$(OBJ)/native-3ds/source/platform_3ds.o $(OBJ)/native-3ds/source/platform_replay.o: $(GEN)/build-tag
$(OBJ)/native-3ds/source/platform_3ds.o: EXTRA := -DFH_INSTALL_STARTUP=1 -DFH_INSTALL_BUILD_ID=\"$(BUILD_ID)\"
$(OBJ)/native-3ds/source/platform_replay.o: EXTRA += -DFH_REPLAY_TAG=\"$(BUILD_TAG)\"
$(OBJ)/native-3ds/source/install_fingerprint.o: EXTRA := -D__3DS__
$(OBJ)/port/src/thp_shim.o $(OBJ)/native-3ds/source/install_movie_decode.o: EXTRA := -fshort-enums
$(OBJ)/$(MUSYX)/hw_pc.o: EXTRA := -include $(CURDIR)/$(MUSYX)/hw_pc_decode.c

SHADER_HEADERS := $(addprefix $(GEN)/,distortion_shader.h game_shader.h heat_shader.h minimal_shared_shader.h \
	native_model_shader.h native_sky_shader.h pica_actor_height_fog_shader.h pica_ambient_shader.h \
	pica_fur_gpu_shader.h pica_glow_adaptive_shader.h pica_glow_reflected_shader.h pica_lightning_shader.h \
	pica_material_shader.h pica_scene_actor_hud_shader.h pica_scene_shader.h pica_scene_texture_shader.h \
	pica_shadow_pair_shader.h pica_shadow_points_shader.h pica_specular_map_shader.h \
	pica_splash_reflection_shader.h pica_water_sheet_shader.h pica_weather_shader.h \
	pica_weather_shared_shader.h post_sample_shader.h sphere_shader.h)

$(GCC_OBJS): $(OBJ)/%.o: %.c | $(SHADER_HEADERS)
	@mkdir -p $(@D)
	$(GCC) $(GCC_CFLAGS) $(EXTRA) -MMD -MP -c $(CURDIR)/$< -o $@

$(OBJ)/%.o: %.c | $(SHADER_HEADERS)
	@mkdir -p $(@D)
	$(CLANG) $(GAME_CFLAGS) $(EXTRA) $(APP_DEFINES) $(if $(filter extern/% port/%,$<),-I$(CURDIR)/$(<D)) -MMD -MP -c $(CURDIR)/$< -o $@

$(OBJ)/%.o: %.S
	@mkdir -p $(@D)
	$(CLANG) $(ARM_CLANG) $(APP_DEFINES) -c $(CURDIR)/$< -o $@

$(BUILD)/libsupport.a: $(SUPPORT_OBJS)
	@rm -f $@
	$(AR) rcsD $@ $^

# Shaders

SHADERS := native-3ds/shaders

define shbin_header
	@mkdir -p $(@D)
	{ printf '$(3)static const unsigned char %s[] __attribute__((aligned($(2))))={' $(1); \
	  od -An -v -tu1 $< | tr -s ' \n' ',,' | sed -e 's/^,//' -e 's/,$$//'; printf '};\n'; } > $@
endef

$(GEN)/%.shbin:
	@mkdir -p $(@D)
	$(PICASSO) -o $@ $^

$(GEN)/game.shbin: $(SHADERS)/game.v.pica $(SHADERS)/game_point.g.pica
$(GEN)/sphere.shbin: $(SHADERS)/game_sphere.v.pica
$(GEN)/native-sky.shbin: $(SHADERS)/native_sky.v.pica $(SHADERS)/game_point.g.pica
$(GEN)/native-model.shbin: $(SHADERS)/native_model.v.pica
$(GEN)/distortion.shbin: $(SHADERS)/pica_distortion.v.pica
$(GEN)/heat.shbin: $(SHADERS)/pica_heat.v.pica
$(GEN)/post-sample.shbin: $(SHADERS)/pica_post_sample.v.pica
$(GEN)/pica-scene.shbin: $(SHADERS)/pica_scene.v.pica
$(GEN)/pica-scene-texture.shbin: $(SHADERS)/pica_scene_texture.v.pica
$(GEN)/pica-specular-map.shbin: $(SHADERS)/pica_specular_map.v.pica
$(GEN)/pica-splash-reflection.shbin: $(SHADERS)/pica_splash_reflection.v.pica
$(GEN)/water-sheet.shbin: $(SHADERS)/pica_water_sheet.v.pica
$(GEN)/pica-weather.shbin: $(SHADERS)/pica_weather.v.pica
$(GEN)/pica-weather-shared.shbin: $(SHADERS)/pica_weather_shared.v.pica
$(GEN)/pica-lightning.shbin: $(SHADERS)/pica_lightning.v.pica
$(GEN)/pica-fur-gpu.shbin: $(SHADERS)/pica_fur_gpu.v.pica
$(GEN)/pica-glow-adaptive.shbin: $(SHADERS)/pica_glow_adaptive.v.pica
$(GEN)/pica-actor-height-fog.shbin: $(SHADERS)/pica_actor_height_fog.v.pica
$(GEN)/part/minimal.shbin: $(SHADERS)/minimal.v.pica
$(GEN)/part/minimal-ui.shbin: $(SHADERS)/minimal_ui.v.pica
$(GEN)/part/minimal-world.shbin: $(SHADERS)/minimal_world.v.pica
$(GEN)/part/minimal-water.shbin: $(SHADERS)/minimal_water.v.pica
$(GEN)/part/minimal-hologram.shbin: $(SHADERS)/minimal_hologram.v.pica
$(GEN)/part/layered.shbin: $(SHADERS)/pica_layered.v.pica
$(GEN)/part/diffuse-map.shbin: $(SHADERS)/pica_diffuse_map.v.pica
$(GEN)/part/height-fog.shbin: $(SHADERS)/pica_height_fog.v.pica
$(GEN)/part/glow.shbin: $(SHADERS)/pica_glow_instance.v.pica
$(GEN)/part/glow-geometry.shbin: $(SHADERS)/pica_glow.g.pica
$(GEN)/part/glow-dynamic.shbin: $(SHADERS)/pica_glow_dynamic.v.pica
$(GEN)/part/glow-reflected-gs.shbin: $(SHADERS)/pica_glow_reflected.g.pica
$(GEN)/part/scene-actor.shbin: $(SHADERS)/pica_scene_actor.v.pica
$(GEN)/part/hud.shbin: $(SHADERS)/pica_hud.v.pica
$(GEN)/part/hologram.shbin: $(SHADERS)/pica_hologram.v.pica
$(GEN)/part/stars.shbin: $(SHADERS)/pica_stars.v.pica
$(GEN)/part/splash.shbin: $(SHADERS)/pica_splash.v.pica
$(GEN)/part/shadow-points-v.shbin: $(SHADERS)/pica_shadow_points.v.pica
$(GEN)/part/shadow-points-g.shbin: $(SHADERS)/pica_shadow_points.g.pica
$(GEN)/part/shadow-entry-0.shbin: $(SHADERS)/pica_ui_model.v.pica
$(GEN)/part/shadow-entry-1.shbin: $(SHADERS)/pica_shadow_entry_1.v.pica
$(GEN)/part/shadow-entry-2.shbin: $(SHADERS)/pica_shadow_entry_2.v.pica
$(GEN)/part/bank-0.shbin: $(SHADERS)/pica_bank_shared_0.v.pica
$(GEN)/part/bank-1.shbin: $(SHADERS)/pica_bank_shared_1.v.pica
$(GEN)/part/bank-2.shbin: $(SHADERS)/pica_bank_shared_2.v.pica
$(GEN)/part/bank-3.shbin: $(SHADERS)/pica_bank_shared_3.v.pica

$(GEN)/linked/minimal-shared.shbin: $(addprefix $(GEN)/part/,minimal.shbin minimal-ui.shbin minimal-world.shbin minimal-water.shbin minimal-hologram.shbin)
$(GEN)/linked/pica-material.shbin: $(addprefix $(GEN)/part/,layered.shbin diffuse-map.shbin height-fog.shbin)
$(GEN)/linked/pica-glow-reflected.shbin: $(addprefix $(GEN)/part/,glow.shbin glow-reflected-gs.shbin glow-dynamic.shbin)
$(GEN)/linked/pica-actor-hud.shbin: $(addprefix $(GEN)/part/,scene-actor.shbin hud.shbin hologram.shbin stars.shbin glow.shbin glow-geometry.shbin glow-dynamic.shbin splash.shbin)
$(GEN)/linked/pica-shadow-points.shbin: $(addprefix $(GEN)/part/,shadow-points-v.shbin shadow-points-g.shbin)
$(GEN)/linked/pica-shadow-pair.shbin: $(addprefix $(GEN)/part/,shadow-entry-0.shbin shadow-entry-1.shbin shadow-entry-2.shbin)
$(GEN)/linked/%.shbin:
	@mkdir -p $(@D)
	$(PYTHON) tools/shbin_link.py link $@ $^
$(GEN)/linked/pica-ambient.shbin: $(addprefix $(GEN)/part/,bank-0.shbin bank-1.shbin bank-2.shbin bank-3.shbin)
	@mkdir -p $(@D)
	$(PYTHON) tools/shbin_link.py link-compact $@ $^

$(GEN)/game_shader.h: $(GEN)/game.shbin ; $(call shbin_header,game_shader,4)
$(GEN)/sphere_shader.h: $(GEN)/sphere.shbin ; $(call shbin_header,sphere_shader,4)
$(GEN)/native_sky_shader.h: $(GEN)/native-sky.shbin ; $(call shbin_header,native_sky_shader,4)
$(GEN)/native_model_shader.h: $(GEN)/native-model.shbin ; $(call shbin_header,native_model_shader,4)
$(GEN)/distortion_shader.h: $(GEN)/distortion.shbin ; $(call shbin_header,distortion_shader,4)
$(GEN)/heat_shader.h: $(GEN)/heat.shbin ; $(call shbin_header,heat_shader,4)
$(GEN)/post_sample_shader.h: $(GEN)/post-sample.shbin ; $(call shbin_header,post_sample_shader,4)
$(GEN)/pica_scene_shader.h: $(GEN)/pica-scene.shbin ; $(call shbin_header,pica_scene_shader,4)
$(GEN)/pica_scene_texture_shader.h: $(GEN)/pica-scene-texture.shbin ; $(call shbin_header,pica_scene_texture_shader,4)
$(GEN)/pica_specular_map_shader.h: $(GEN)/pica-specular-map.shbin ; $(call shbin_header,pica_specular_map_shader,4)
$(GEN)/pica_splash_reflection_shader.h: $(GEN)/pica-splash-reflection.shbin ; $(call shbin_header,pica_splash_reflection_shader,4)
$(GEN)/pica_water_sheet_shader.h: $(GEN)/water-sheet.shbin ; $(call shbin_header,pica_water_sheet_shader,4)
$(GEN)/pica_weather_shader.h: $(GEN)/pica-weather.shbin ; $(call shbin_header,pica_weather_shader,4)
$(GEN)/pica_weather_shared_shader.h: $(GEN)/pica-weather-shared.shbin ; $(call shbin_header,pica_weather_shared_shader,4)
$(GEN)/pica_lightning_shader.h: $(GEN)/pica-lightning.shbin ; $(call shbin_header,pica_lightning_shader,4)
$(GEN)/pica_fur_gpu_shader.h: $(GEN)/pica-fur-gpu.shbin ; $(call shbin_header,pica_fur_gpu_shader,4)
$(GEN)/pica_glow_adaptive_shader.h: $(GEN)/pica-glow-adaptive.shbin ; $(call shbin_header,pica_glow_adaptive_shader,4)
$(GEN)/pica_actor_height_fog_shader.h: $(GEN)/pica-actor-height-fog.shbin ; $(call shbin_header,pica_actor_height_fog_shader,16)
$(GEN)/minimal_shared_shader.h: $(GEN)/linked/minimal-shared.shbin ; $(call shbin_header,minimal_shared_shader,4)
$(GEN)/pica_material_shader.h: $(GEN)/linked/pica-material.shbin ; $(call shbin_header,pica_material_shader,4)
$(GEN)/pica_glow_reflected_shader.h: $(GEN)/linked/pica-glow-reflected.shbin ; $(call shbin_header,pica_glow_reflected_shader,4)
$(GEN)/pica_shadow_points_shader.h: $(GEN)/linked/pica-shadow-points.shbin ; $(call shbin_header,pica_shadow_points_shader,4)
$(GEN)/pica_shadow_pair_shader.h: $(GEN)/linked/pica-shadow-pair.shbin ; $(call shbin_header,pica_shadow_pair_shader,4)
$(GEN)/pica_ambient_shader.h: $(GEN)/linked/pica-ambient.shbin ; $(call shbin_header,pica_ambient_shader,4)
$(GEN)/pica_scene_actor_hud_shader.h: $(GEN)/linked/pica-actor-hud.shbin ; $(call shbin_header,pica_scene_actor_hud_shader,4,enum { FH_PICA_SHARED_ENTRIES = 8 }$(semicolon)\n)

semicolon := ;

# Installer codecs and mesh reduction

MESHOPT := native-3ds/vendor/meshoptimizer
CODEC_CXXFLAGS := -std=c++11 -O2 -mcpu=mpcore -mfpu=vfp -mfloat-abi=hard -mtp=soft -fno-short-enums -ffp-contract=off \
	-fno-strict-aliasing -ffunction-sections -fdata-sections -DMESHOPTIMIZER_ALLOC_EXPORT $(PREFIX_MAP) \
	-I$(CURDIR)/native-3ds/include -I$(CURDIR)/$(MESHOPT)

$(OBJ)/water-reducer/%.o:
	@mkdir -p $(@D)
	$(GXX) $(CODEC_CXXFLAGS) -c $(CURDIR)/$< -o $@
$(OBJ)/water-reducer/install_simplify.o: native-3ds/source/install_simplify.cpp
$(OBJ)/water-reducer/simplifier.o: $(MESHOPT)/simplifier.cpp

$(BUILD)/libwater-reducer.a: $(OBJ)/water-reducer/install_simplify.o $(OBJ)/water-reducer/simplifier.o
	@rm -f $@
	$(AR) rcs $@ $^

$(OBJ)/codecs/%.o:
	@mkdir -p $(@D)
	$(GXX) -D__3DS__ $(subst -O2,$(OPT),$(CODEC_CXXFLAGS)) -I$(PORTLIBS)/include -c $(CURDIR)/$< -o $@
$(OBJ)/codecs/%.o: OPT := -O2
$(OBJ)/codecs/rg_etc1.o: OPT := -O3
$(OBJ)/codecs/install_texture.o: native-3ds/source/install_texture.cpp
$(OBJ)/codecs/install_movie_frame.o: native-3ds/source/install_movie_frame.cpp
$(OBJ)/codecs/install_simplify.o: native-3ds/source/install_simplify.cpp
$(OBJ)/codecs/rg_etc1.o: native-3ds/vendor/rg_etc1/rg_etc1.cpp
$(OBJ)/codecs/simplifier.o: $(MESHOPT)/simplifier.cpp
$(OBJ)/codecs/install-hints.o: native-3ds/source/install_hints.s native-3ds/data/install-texture-hints.zlib
	@mkdir -p $(@D)
	$(GXX) -mcpu=mpcore -Wa,-I$(CURDIR)/native-3ds/data -c $(CURDIR)/$< -o $@

CODEC_OBJS := $(addprefix $(OBJ)/codecs/,install_texture.o install_movie_frame.o install_simplify.o rg_etc1.o simplifier.o install-hints.o)
$(BUILD)/libinstall-codecs.a: $(CODEC_OBJS)
	@rm -f $@
	$(AR) rcs $@ $^

# Private citro3d and libctru archives

CITRO3D_OBJS := $(addprefix $(OBJ)/citro3d/,texture.o renderqueue.o uniforms.o base.o buffers.o drawElements.o drawArrays.o effect.o)
$(OBJ)/citro3d/%.o: native-3ds/citro3d/%.c
	@mkdir -p $(@D)
	$(GCC) -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -DCITRO3D_BUILD -O2 -g \
		-ffunction-sections -fdata-sections -I$(LIBCTRU)/include -I$(CURDIR)/native-3ds/citro3d -c $(CURDIR)/$< -o $@

$(BUILD)/lib/libcitro3d.a: $(CITRO3D_OBJS)
	@mkdir -p $(@D)
	cp $(LIBCTRU)/lib/libcitro3d.a $@
	$(AR) r $@ $^

$(OBJ)/ctru/gspgpu.o:
	@mkdir -p $(@D)
	cd $(@D) && $(AR) x $(LIBCTRU)/lib/libctru.a gspgpu.o
	$(OBJCOPY) --redefine-sym gxCmdQueueInterrupt=__wrap_gxCmdQueueInterrupt $@

$(OBJ)/ctru/gxqueue.o: native-3ds/ctru/gxqueue.c
	@mkdir -p $(@D)
	$(GCC) -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -O2 -g -Wall -Wextra -Werror \
		-ffunction-sections -fdata-sections -I$(LIBCTRU)/include -c $(CURDIR)/$< -o $@

$(BUILD)/lib/libctru.a: $(OBJ)/ctru/gspgpu.o $(OBJ)/ctru/gxqueue.o
	@mkdir -p $(@D)
	cp $(LIBCTRU)/lib/libctru.a $@
	$(AR) r $@ $(OBJ)/ctru/gspgpu.o
	$(AR) r $@ $(OBJ)/ctru/gxqueue.o

# Link

WRAPS := -Wl,--wrap=waitNextFrame,--wrap=audioUpdate,--wrap=loadDataFiles,--wrap=uiDll_runFrameEndAndLoadNext,--wrap=gameTextRun,--wrap=Sfx_UpdateLoopedObjectSounds,--wrap=uiDll_runFrameStartAndLoadNext,--wrap=Obj_UpdateAllObjects,--wrap=updateEnvironment,--wrap=trackIntersect,--wrap=sceneRender,--wrap=curUiDllDraw,--wrap=drawReflectionTexture,--wrap=updateReflectionTextures,--wrap=Rcp_UpdateDistortionTextures,--wrap=renderParticles,--wrap=objRender \
	-Wl,--wrap=FSUSER_OpenFile,--wrap=FSFILE_Read,--wrap=FSFILE_Write,--wrap=FSFILE_Flush,--wrap=FSFILE_Close,--wrap=FSFILE_GetSize,--wrap=FSFILE_SetSize,--wrap=FSUSER_RenameFile,--wrap=FSUSER_DeleteFile,--wrap=FSUSER_CreateFile,--wrap=FSUSER_OpenDirectory,--wrap=FSDIR_Read,--wrap=FSDIR_Close \
	-Wl,--wrap=gspSubmitGxCommand,--wrap=gxCmdQueueInterrupt \
	-Wl,--wrap=shaderProgramConfigure,--wrap=shaderProgramInit,--wrap=shaderProgramFree,--wrap=shaderProgramSetVsh,--wrap=shaderProgramSetGsh,--wrap=shaderProgramSetGshInputPermutation \
	-Wl,--wrap=GX_ProcessCommandList \
	-Wl,--wrap=exit,--wrap=__system_allocateHeaps,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=loadAndDecompressDataFile,--wrap=THPVideoDecode

LINK_LIBS := $(BUILD)/libgame.a $(BUILD)/libsupport.a $(BUILD)/libwater-reducer.a $(BUILD)/libinstall-codecs.a

$(OUT)/foxhollow.elf: $(LINK_LIBS) $(BUILD)/lib/libcitro3d.a $(BUILD)/lib/libctru.a
	@mkdir -p $(@D)
	$(GXX) $(WRAPS) -L$(BUILD)/lib -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -specs=3dsx.specs \
		-Wl,--gc-sections,-Map,$(OUT)/foxhollow.map -L$(LIBCTRU)/lib -L$(PORTLIBS)/lib -Wl,--start-group \
		$(LINK_LIBS) -lcitro3d -lctru -ljpeg -lz -lmbedcrypto -lm -lsupc++ -lpthread -llz4 -Wl,--end-group -o $@

$(OUT)/foxhollow.3dsx: $(OUT)/foxhollow.elf
	$(3DSXTOOL) $< $@

# Home Menu package

HOME := native-3ds/home-menu
$(OUT)/icon.smdh: $(HOME)/icon.png
	@mkdir -p $(@D)
	$(BANNERTOOL) makesmdh -s Foxhollow -l "Unified Old/New 3DS native port" -p "Foxhollow contributors" \
		-i $< -o $@ -r regionfree -f visible,recordusage
$(OUT)/banner.bin: $(HOME)/banner.png $(HOME)/silent.wav
	@mkdir -p $(@D)
	$(BANNERTOOL) makebanner -i $(HOME)/banner.png -a $(HOME)/silent.wav -o $@
$(OUT)/foxhollow.cxi: $(OUT)/foxhollow.elf $(HOME)/foxhollow.rsf $(OUT)/banner.bin $(OUT)/icon.smdh
	$(MAKEROM) -f cxi -target t -exefslogo -elf $< -rsf $(HOME)/foxhollow.rsf -banner $(OUT)/banner.bin \
		-icon $(OUT)/icon.smdh -o $@
$(OUT)/foxhollow.cia: $(OUT)/foxhollow.cxi
	$(MAKEROM) -f cia -target t -content $<:0:0 -ver 1 -o $@

all: $(OUT)/foxhollow.3dsx $(OUT)/foxhollow.cia

clean:
	rm -rf $(BUILD)

.PHONY: all clean FORCE
FORCE:

-include $(GAME_OBJS:.o=.d) $(SUPPORT_OBJS:.o=.d) $(HOST_OBJS:.o=.d)

# PC asset converter

HOSTCC ?= clang
HOSTCXX ?= clang++
HOST_OBJ := $(BUILD)/host
HOST_JPEG_CFLAGS ?=
HOST_JPEG_LIBS ?= -ljpeg
HOST_FLAGS := $(HOST_JPEG_CFLAGS) -O2 -g -ffp-contract=off -fno-strict-aliasing -ffunction-sections -fdata-sections \
	-DMESHOPTIMIZER_ALLOC_EXPORT $(PREFIX_MAP) -I$(CURDIR)/native-3ds/include -I$(CURDIR)/$(MESHOPT)
HOST_C := native-3ds/host/fh_convert.c $(filter-out native-3ds/source/install_bindings.c \
	native-3ds/source/install_boot.c native-3ds/source/install_worker.c,$(wildcard native-3ds/source/install_*.c)) \
	native-3ds/source/native_scene.c native-3ds/source/model_lod.c native-3ds/source/platform_texture_pack.c
HOST_CXX := $(wildcard native-3ds/source/install_*.cpp) native-3ds/vendor/rg_etc1/rg_etc1.cpp $(MESHOPT)/simplifier.cpp
HOST_OBJS := $(addprefix $(HOST_OBJ)/,$(HOST_C:.c=.o) $(HOST_CXX:.cpp=.o)) $(HOST_OBJ)/install_hints.o

$(HOST_OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(HOSTCC) -std=gnu11 $(HOST_FLAGS) -MMD -MP -c $(CURDIR)/$< -o $@
$(HOST_OBJ)/%.o: %.cpp
	@mkdir -p $(@D)
	$(HOSTCXX) -std=c++11 $(HOST_FLAGS) -MMD -MP -c $(CURDIR)/$< -o $@
$(HOST_OBJ)/install_hints.o: native-3ds/source/install_hints.s native-3ds/data/install-texture-hints.zlib
	@mkdir -p $(@D)
	$(HOSTCC) -Wa,-I$(CURDIR)/native-3ds/data -c $(CURDIR)/$< -o $@

$(BUILD)/fh-convert: $(HOST_OBJS)
	$(HOSTCXX) -Wl,--gc-sections $^ $(HOST_JPEG_LIBS) -llz4 -lmbedcrypto -lz -lm -o $@

convert: $(BUILD)/fh-convert

.PHONY: convert

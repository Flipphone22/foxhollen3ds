ROOT := $(abspath ..)
N := $(ROOT)/native-3ds
NS := $(N)/source
NI := $(N)/include
T := $(ROOT)/tests
B ?= $(ROOT)/build/host-tests
G := $(B)/gen

CC := clang
CXX := clang++
SAN := -fsanitize=address,undefined -fno-sanitize-recover=all
CFLAGS_BASE := -std=gnu11 -O1 -g $(SAN)
CXXFLAGS_BASE := -std=gnu++17 -O1 -g $(SAN)
export ASAN_OPTIONS := detect_leaks=0:abort_on_error=0
export UBSAN_OPTIONS := print_stacktrace=1:halt_on_error=1

TESTS :=

TESTS += graphics_settings
graphics_settings_SRC := $(T)/graphics_settings.c
graphics_settings_ARGS := .

B0_MUSYX_RT := $(ROOT)/extern/musyx/src/musyx/runtime
B0_MUSYX_DEFS := -DTARGET_PC=1 -DMUSY_TARGET=MUSY_TARGET_PC -DMUSY_VERSION_MAJOR=2 -DMUSY_VERSION_MINOR=0 -DMUSY_VERSION_PATCH=0

TESTS += alpha_cache
alpha_cache_SRC := $(T)/alpha_cache.c
alpha_cache_BASE := -std=c11 -O2 -g $(SAN)

TESTS += aram
aram_SRC := $(NS)/aram.c $(T)/aram.c
aram_CFLAGS := -DTARGET_PC=1 -I$(ROOT)/port/include -I$(ROOT)/extern/aurora/include

TESTS += async_queue
async_queue_SRC := $(T)/async_queue.c
async_queue_DEPS := $(G)/async_queue/queue-under-test.inc $(G)/async_queue/early-queue-under-test.inc
async_queue_BASE := -O2 -g -Wall -Wextra $(SAN)
async_queue_CFLAGS := -I$(G)/async_queue

$(G)/async_queue/queue-under-test.inc: $(N)/citro3d/renderqueue.c
	mkdir -p $(@D)
	sed -e '/^#include /d' -e '/^static C3D_RenderTarget\* C3Di_RenderTargetNew(void)/,/^static void C3Di_SafeDisplayTransfer(/{/^static void C3Di_SafeDisplayTransfer(/!d}' $< > $@

$(G)/async_queue/early-queue-under-test.inc: $(NS)/platform_early_queue.c
	mkdir -p $(@D)
	sed -e '/^#include /d' -e 's/^extern void fh_gx_queue_hold(gxCmdQueue_s \*queue);/static void fh_gx_queue_hold(gxCmdQueue_s *queue){(void)queue;}/' $< > $@

TESTS += audio_aux_capture
audio_aux_capture_SRC := $(NS)/audio_source_bridge.c $(NS)/audio_sample_clock.c $(T)/audio_aux_capture.c $(B0_MUSYX_RT)/hw_pc_decode.c $(B0_MUSYX_RT)/synth_adsr.c $(B0_MUSYX_RT)/synth_dbtab.c
audio_aux_capture_DEPS := $(G)/audio_aux_capture/mixer.c $(G)/audio_aux_capture/source-fixture.c
audio_aux_capture_BASE := -std=gnu11 -O2 -g -fwrapv -ffunction-sections -fdata-sections $(SAN) -fno-sanitize=shift
audio_aux_capture_CFLAGS := $(B0_MUSYX_DEFS) -I$(G)/audio_aux_capture -I$(B0_MUSYX_RT) -I$(ROOT)/extern/musyx/include
audio_aux_capture_LIBS := -Wl,--gc-sections

$(G)/audio_aux_capture/mixer.c: $(B0_MUSYX_RT)/hw_pc.c
	mkdir -p $(@D)
	cp $< $@

$(G)/audio_aux_capture/source-fixture.c: $(T)/audio_source_bridge.c
	mkdir -p $(@D)
	cp $< $@

B0_AUX_NATIVE_UNITS := $(addprefix $(NS)/,audio_aux_spans.c audio_aux_rate.c audio_effect_routes.c audio_effect_epoch.c audio_aux_native_shared.c audio_aux_input.c)

TESTS += audio_aux_first_phase
audio_aux_first_phase_SRC := $(G)/audio_aux_first_phase/test.c $(NS)/audio_aux_native.c $(B0_AUX_NATIVE_UNITS)
audio_aux_first_phase_BASE := -std=gnu11 -O2 -g -Wall -Wextra -Werror $(SAN)

$(G)/audio_aux_first_phase/test.c: $(T)/audio_aux_native.c $(T)/audio_aux_first_phase.c
	mkdir -p $(@D)
	awk -v extra=$(T)/audio_aux_first_phase.c 'index($$0,"int main(void){mapping();")==1{while((getline l<extra)>0)print l;sub(/^int main\(void\)\{mapping\(\);/,"int main(void){first_phase();mapping();")}{print}' $(T)/audio_aux_native.c > $@

TESTS += audio_aux_input
audio_aux_input_SRC := $(T)/audio_aux_input.c $(NS)/audio_aux_input.c
audio_aux_input_BASE := -std=gnu11 -O2 -g -Wall -Wextra -Werror $(SAN)
audio_aux_input_LIBS := -pthread

TESTS += audio_aux_native
audio_aux_native_SRC := $(T)/audio_aux_native.c $(NS)/audio_aux_native.c $(B0_AUX_NATIVE_UNITS)
audio_aux_native_BASE := -std=gnu11 -O2 -g $(SAN)

TESTS += audio_aux_rate
audio_aux_rate_SRC := $(T)/audio_aux_rate.c $(NS)/audio_aux_rate.c
audio_aux_rate_BASE := -std=gnu11 -O2 -g -fno-omit-frame-pointer $(SAN)
audio_aux_rate_CFLAGS := -I$(ROOT)/extern/musyx/include $(B0_MUSYX_DEFS)

B1_MUSY := -I$(ROOT)/extern/musyx/include -DTARGET_PC=1 -DMUSY_TARGET=MUSY_TARGET_PC -DMUSY_VERSION_MAJOR=2 -DMUSY_VERSION_MINOR=0 -DMUSY_VERSION_PATCH=0
B1_O2 := -std=gnu11 -O2 -g $(SAN) -fno-omit-frame-pointer
B1_CREVERB := $(ROOT)/extern/musyx/src/musyx/runtime/CheapReverb/creverb.c
B1_HOOKS := $(G)/batch1/effect_hooks.c

$(B1_HOOKS):
	mkdir -p $(@D)
	printf '%s\n' 'void fh_audio_game_effect_created(const void *w,float a,float b,float c,float d,float e){(void)w;(void)a;(void)b;(void)c;(void)d;(void)e;}' 'void fh_audio_game_effect_destroyed(const void *w){(void)w;}' > $@

TESTS += audio_effect_epoch
audio_effect_epoch_SRC := $(T)/audio_effect_epoch.c $(NS)/audio_effect_epoch.c $(B1_CREVERB) $(B1_HOOKS)
audio_effect_epoch_BASE := $(B1_O2)
audio_effect_epoch_CFLAGS := $(B1_MUSY)
audio_effect_epoch_LIBS := -pthread

TESTS += audio_effect_routes
audio_effect_routes_SRC := $(T)/audio_effect_routes.c $(NS)/audio_effect_routes.c $(NS)/audio_effect_epoch.c $(B1_CREVERB) $(B1_HOOKS)
audio_effect_routes_BASE := $(B1_O2)
audio_effect_routes_CFLAGS := $(B1_MUSY)

b2_O2 := -std=gnu11 -O2 -g $(SAN)
b2_RT := $(ROOT)/extern/musyx/src/musyx/runtime
b2_MUSYX := -DTARGET_PC=1 -DMUSY_TARGET=MUSY_TARGET_PC -DMUSY_VERSION_MAJOR=2 -DMUSY_VERSION_MINOR=0 -DMUSY_VERSION_PATCH=0 -I$(ROOT)/extern/musyx/include
b2_GXINC := -DTARGET_PC=1 -I$(ROOT)/port/include -I$(ROOT)/extern/aurora/include -I$(ROOT)/game/include
b2_GXFLAGS := -I$(NS) -ffp-contract=off -ffunction-sections -fdata-sections $(b2_GXINC)
b2_GXSTUB := '/^static void output_vertex\(/{print substr($$0,1,index($$0,"{")-1) "{ if(!compiled_capture)abort();if(compiled_capture!=v)memcpy(compiled_capture,v,sizeof(*v)); }";skip=1;next} /^static void flush\(/{print substr($$0,1,index($$0,"{")-1) "{ abort(); }";skip=1;next} skip{if($$0=="}")skip=0;next} {print}'
b2_TGFIX := 's/memset(prepared_texgen,0,sizeof(prepared_texgen));/&fh_gx_texgen_inputs=~0u;/'

TESTS += audio_ramp
audio_ramp_SRC := $(T)/audio_ramp.c
audio_ramp_BASE := $(b2_O2)

TESTS += audio_sample_clock
audio_sample_clock_SRC := $(NS)/audio_sample_clock.c $(T)/audio_sample_clock.c $(b2_RT)/hw_pc_decode.c
audio_sample_clock_BASE := $(b2_O2)
audio_sample_clock_CFLAGS := -fwrapv -ffunction-sections -fdata-sections $(b2_MUSYX) -I$(b2_RT)
audio_sample_clock_LIBS := -Wl,--gc-sections

TESTS += audio_sample_pool
audio_sample_pool_SRC := $(NS)/audio_sample_pool.c $(NS)/audio_sample_plan.c $(T)/audio_sample_pool.c
audio_sample_pool_BASE := $(b2_O2)
audio_sample_pool_CFLAGS := -Wall -Wextra -Werror

TESTS += audio_sample_registry
audio_sample_registry_SRC := $(b2_RT)/hw_pc_samples.c $(NS)/audio_sample_registry.c $(NS)/audio_sample_pool.c $(NS)/audio_sample_plan.c $(T)/audio_sample_registry.c
audio_sample_registry_BASE := $(b2_O2)
audio_sample_registry_CFLAGS := -Wall -Wextra -Werror $(b2_MUSYX) -I$(b2_RT)

TESTS += audio_software_prefix
audio_software_prefix_SRC := $(T)/audio_software_prefix.c $(NS)/audio_aux_shared.c $(NS)/audio_aux_spans.c $(NS)/audio_aux_rate.c $(NS)/audio_aux_input.c $(NS)/audio_effect_routes.c $(NS)/audio_effect_epoch.c $(NS)/audio_output_clock.c $(NS)/audio_voice_window.c
audio_software_prefix_DEPS := $(G)/audio_software_prefix/prefix-owner.inc
audio_software_prefix_CFLAGS := -Wall -Wextra -Werror -I$(G)/audio_software_prefix

$(G)/audio_software_prefix/prefix-owner.inc: $(NS)/platform_audio_game_dsp.c
	@mkdir -p $(@D)
	awk '/^static void native_source_window\(/{on=1} /^void fh_audio_game_dsp_service\(/{on=0} on' $< > $@

TESTS += azahar_scripted_hid
azahar_scripted_hid_SRC := $(T)/azahar_scripted_hid.cpp
azahar_scripted_hid_CC := $(CXX)
azahar_scripted_hid_BASE := $(CXXFLAGS_BASE)
azahar_scripted_hid_ARGS := trace.jsonl

b2_BLOCK_SETUP := python3 -c 'import random;open("source.bin","wb").write(b"palette and positions\0"*4000+random.Random(617).randbytes(100000)+bytes(65539))' && python3 $(ROOT)/tools/prepare_block_cache.py source.bin cache.bin > /dev/null

TESTS += block_cache
block_cache_SRC := $(T)/block_cache.c
block_cache_BASE := -std=c11 -O1 -g $(SAN)
block_cache_CFLAGS := -Wall -Wextra -Werror
block_cache_LIBS := -lz
block_cache_SETUP := $(b2_BLOCK_SETUP)
block_cache_ARGS := source.bin cache.bin

TESTS += block_cache_window
block_cache_window_SRC := $(T)/block_cache_window.c
block_cache_window_BASE := -std=c11 -O1 -g $(SAN)
block_cache_window_CFLAGS := -Wall -Wextra -Werror
block_cache_window_LIBS := -lz
block_cache_window_SETUP := $(b2_BLOCK_SETUP)
block_cache_window_ARGS := source.bin cache.bin

TESTS += card_storage
card_storage_SRC := $(NS)/card.c $(T)/card_storage.c
card_storage_CFLAGS := -DTARGET_PC=1 -Wall -Wextra -Werror -I$(ROOT)/port/include -I$(ROOT)/extern/aurora/include
card_storage_LIBS := -Wl,--wrap=fopen -Wl,--wrap=opendir -Wl,--wrap=mkdir -Wl,--wrap=readdir -Wl,--wrap=closedir -lz
card_storage_ARGS := .

TESTS += chained_uv
chained_uv_SRC := $(T)/chained_uv.c $(NS)/material_layers.c

TESTS += compiled_decode
compiled_decode_SRC := $(G)/compiled_decode/decoder.c
compiled_decode_BASE := $(b2_O2)
compiled_decode_CFLAGS := $(b2_GXFLAGS)
compiled_decode_LIBS := -Wl,--gc-sections

$(G)/compiled_decode/decoder.c: $(NS)/gx_vertices.c $(T)/compiled_decode.c
	@mkdir -p $(@D)
	{ printf '#include <stdlib.h>\n'; awk $(b2_GXSTUB) $(NS)/gx_vertices.c; printf '\n'; cat $(T)/compiled_decode.c; } | sed $(b2_TGFIX) > $@

TESTS += compiled_color_decode
compiled_color_decode_SRC := $(G)/compiled_color_decode/decoder.c
compiled_color_decode_BASE := $(b2_O2)
compiled_color_decode_CFLAGS := $(b2_GXFLAGS)
compiled_color_decode_LIBS := -Wl,--gc-sections

$(G)/compiled_color_decode/decoder.c: $(NS)/gx_vertices.c $(T)/compiled_decode.c $(T)/fused_native_decode.c $(T)/compiled_color_decode.c
	@mkdir -p $(@D)
	{ printf '#include <stdlib.h>\n'; awk $(b2_GXSTUB) $(NS)/gx_vertices.c; printf '\n'; sed '0,/int main(void){/s//int baseline_main(void){/' $(T)/compiled_decode.c; printf '\n'; cat $(T)/fused_native_decode.c; printf '\n'; cat $(T)/compiled_color_decode.c; } | sed $(b2_TGFIX) > $@

TESTS += cpu_native_decode
cpu_native_decode_SRC := $(G)/cpu_native_decode/decoder.c
cpu_native_decode_BASE := $(b2_O2)
cpu_native_decode_CFLAGS := $(b2_GXFLAGS)
cpu_native_decode_LIBS := -Wl,--gc-sections

$(G)/cpu_native_decode/decoder.c: $(NS)/gx_vertices.c $(T)/compiled_decode.c $(T)/cpu_native_decode.c
	@mkdir -p $(@D)
	{ printf '#include <stdlib.h>\n'; awk $(b2_GXSTUB) $(NS)/gx_vertices.c; printf '\n'; sed '0,/int main(void){/s//int baseline_main(void){/' $(T)/compiled_decode.c; printf '\n'; cat $(T)/cpu_native_decode.c; } | sed $(b2_TGFIX) > $@

TESTS += draw_capture
draw_capture_SRC := $(T)/draw_capture.c $(NS)/platform_draw_capture.c
draw_capture_CFLAGS := -DFH_DRAW_CAPTURE=1 -DFH_EMULATOR_TEST=1

TESTS += efb
efb_SRC := $(NS)/efb.c $(T)/efb.c

TESTS += efb_stride
efb_stride_SRC := $(T)/efb_stride.c $(NS)/efb.c

TESTS += fragment
fragment_SRC := $(NS)/fragment.c $(NS)/fur_mesh.c $(NS)/texture.c $(NS)/material_layers.c $(T)/fragment.c
fragment_LIBS := -Wl,--wrap=fh_fur_samples

TESTS += fragment_arguments
fragment_arguments_SRC := $(T)/fragment_arguments.c
fragment_arguments_BASE := $(b2_O2)
fragment_arguments_CFLAGS := -ffunction-sections -fdata-sections
fragment_arguments_LIBS := -Wl,--gc-sections

TESTS += fragment_raster
fragment_raster_SRC := $(NS)/fragment.c $(NS)/fragment_raster.c $(NS)/texture.c $(T)/fragment_raster.c
fragment_raster_BASE := $(b2_O2)
fragment_raster_CFLAGS := -ffp-contract=off

TESTS += fused_native_decode
fused_native_decode_SRC := $(G)/fused_native_decode/decoder.c
fused_native_decode_BASE := $(b2_O2)
fused_native_decode_CFLAGS := $(b2_GXFLAGS)
fused_native_decode_LIBS := -Wl,--gc-sections

$(G)/fused_native_decode/decoder.c: $(NS)/gx_vertices.c $(T)/compiled_decode.c $(T)/fused_native_decode.c
	@mkdir -p $(@D)
	{ printf '#include <stdlib.h>\n'; awk $(b2_GXSTUB) $(NS)/gx_vertices.c; printf '\n'; awk -v f=$(T)/fused_native_decode.c '/^int main\(void\)\{/ && !d {while((getline l < f)>0) print l; d=1} {sub(/compared\+\+;/,"check_native_packet(\&reference,\&fast,packet,trial);compared++;")} 1' $(T)/compiled_decode.c; } | sed $(b2_TGFIX) > $@

b3_AUR := $(ROOT)/extern/aurora/include
b3_HOSTC := -std=c11 -O1 -g -fsanitize=address,undefined
b3_SLICE = perl -0777 -ne 'BEGIN{($$s,$$e)=splice(@ARGV,0,2)} /(\Q$$s\E.*?)\Q$$e\E/s or die "slice $$s"; print $$1'
b3_SUB = perl -0777 -pe 'BEGIN{@r=@ARGV;@ARGV=()} for(my $$i=0;$$i<@r;$$i+=2){my($$p,$$q)=@r[$$i,$$i+1]; s/$$p/$$q/g or die "miss $$p"}'
b3_NEG = ! $(1) >$(3).out 2>$(3).err && grep -qF '$(2)' $(3).err

TESTS += fragment_sampling
fragment_sampling_SRC := $(T)/fragment_sampling.c
fragment_sampling_BASE := -std=gnu11 -O2 -g $(SAN) -ffunction-sections -fdata-sections -Wl,--gc-sections

TESTS += fur_detail
fur_detail_SRC := $(T)/fur_detail.c

b3_FENCE_WAIT := 'static void gpu_copy_wait(' 'const char *fh_renderer_texture_copy_error('
b3_FENCE_COPY := 'static void copy_owned_gpu(' 'static unsigned depth_snapshot_valid'
b3_fence_fence := 'gpu_copy_wait\("producer",serial\);' '(void)serial;' 'if\(gpu_copy_fences\)gpu_copy_wait\("transfer",serial\);' ''
b3_fence_capacity := 'C3D_FrameSplit\(GX_CMDLIST_FLUSH\);\n {16}(?=if\(R_FAILED\(GX_RequestDma)' ''
b3_fence_consumer := 'if\(gpu_copy_fences\)gpu_copy_wait\("row transfer",serial\);' ''
b3_fence_capture_consumer := 'if\(gpu_copy_fences\)gpu_copy_wait\("capture DMA",serial\);' ''
b3_ordered_split := 'C3D_FrameSplit\(GX_CMDLIST_FLUSH\);' '(void)0;'

define b3_fence_variant
TESTS += $(1)
$(1)_SRC := $(T)/gpu_copy_fence.c
$(1)_DEPS := $(G)/$(1)/copy-fence-under-test.inc
$(1)_BASE := -O1 -g -Wall -Wextra $(SAN)
$(1)_CFLAGS := -I$(G)/$(1) $(3)
$(G)/$(1)/copy-fence-under-test.inc: $(NS)/platform_graphics.c
	mkdir -p $$(@D)
	{ $$(b3_SLICE) $(b3_FENCE_WAIT) $$<; $$(b3_SLICE) $(b3_FENCE_COPY) $$<; } > $$@.tmp
	$$(if $$($(2)),$$(b3_SUB) $$($(2)) < $$@.tmp > $$@ && rm $$@.tmp,mv $$@.tmp $$@)
endef
$(eval $(call b3_fence_variant,gpu_copy_fence,,))
$(eval $(call b3_fence_variant,gpu_copy_fence_fence,b3_fence_fence,-DFH_NEGATIVE_FENCE=1))
$(eval $(call b3_fence_variant,gpu_copy_fence_capacity,b3_fence_capacity,-DFH_NEGATIVE_CAPACITY=1))
$(eval $(call b3_fence_variant,gpu_copy_fence_consumer,b3_fence_consumer,-DFH_NEGATIVE_CONSUMER=1))
$(eval $(call b3_fence_variant,gpu_copy_fence_capture_consumer,b3_fence_capture_consumer,-DFH_NEGATIVE_CAPTURE_CONSUMER=1))

define b3_ordered_variant
TESTS += $(1)
$(1)_SRC := $(T)/gpu_copy_ordered.c
$(1)_DEPS := $(G)/$(1)/copy-fence-under-test.inc
$(1)_BASE := -std=gnu11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function $(SAN)
$(1)_CFLAGS := -I$(G)/$(1) $(3)
$(G)/$(1)/copy-fence-under-test.inc: $(NS)/platform_graphics.c $(NS)/platform_efb_color.h
	mkdir -p $$(@D)
	{ $$(b3_SLICE) $(b3_FENCE_WAIT) $$<; $$(b3_SLICE) $(b3_FENCE_COPY) $$<; $$(b3_SLICE) 'static void efb_view_boundary(' 'static void efb_color_end(' $(NS)/platform_efb_color.h; } > $$@.tmp
	$$(if $$($(2)),$$(b3_SUB) $$($(2)) < $$@.tmp > $$@ && rm $$@.tmp,mv $$@.tmp $$@)
endef
$(eval $(call b3_ordered_variant,gpu_copy_ordered,,))
$(eval $(call b3_ordered_variant,gpu_copy_ordered_missing_split,b3_ordered_split,-DFH_NEGATIVE_SPLIT=1))

TESTS += gpu_transfer
gpu_transfer_SRC := $(T)/gpu_transfer.c
gpu_transfer_BASE := -Wall -Wextra -Werror -O1 -g $(SAN)

TESTS += gpu_work
gpu_work_SRC := $(NS)/platform_gpu_work.c $(T)/gpu_work.c
gpu_work_BASE := -std=c11 -O1 -g -Wall -Wextra -Werror $(SAN) -I$(T)/gpu_work_host
gpu_work_SETUP := $(B)/bin/gpu_work failure > failure.log 2>&1

TESTS += ground_mask
ground_mask_SRC := $(T)/ground_mask.c
ground_mask_BASE := $(b3_HOSTC)

TESTS += ground_result
ground_result_SRC := $(T)/ground_result.c
ground_result_BASE := $(b3_HOSTC)

TESTS += gx_matrix_block
gx_matrix_block_SRC := $(T)/gx_matrix_block.c
gx_matrix_block_DEPS := $(NI)/fh_gx_matrix_block.h
gx_matrix_block_BASE := -O2 -g -Wall -Wextra -Werror $(SAN)

TESTS += gx_ring
gx_ring_SRC := $(T)/gx_ring.c
gx_ring_DEPS := $(NI)/fh_gx_ring.h
gx_ring_BASE := -O2 -g -Wall -Wextra -Werror $(SAN) -pthread

b3_GXSF_BASE := -O2 -g -std=gnu11 -DTARGET_PC=1 -ffunction-sections -fdata-sections -Wl,--gc-sections $(SAN)
$(G)/gx_state_filter/negative/fh_gx_state_filter.h: $(NI)/fh_gx_state_filter.h
	mkdir -p $(@D)
	$(b3_SUB) 'if\(id<16\)for\(unsigned i=0;i<4;i\+\+\)f->cells\[id\*9\+i\]\.epoch=0;' '' < $< > $@
$(G)/gx_state_filter/negative-test: $(G)/gx_state_filter/negative/fh_gx_state_filter.h $(T)/gx_state_filter.c $(NS)/gx_state.c
	$(CC) $(b3_GXSF_BASE) -I$(G)/gx_state_filter/negative -I$(NI) -I$(b3_AUR) $(T)/gx_state_filter.c $(NS)/gx_state.c -o $@ -lm

TESTS += gx_state_filter
gx_state_filter_SRC := $(T)/gx_state_filter.c $(NS)/gx_state.c
gx_state_filter_DEPS := $(NI)/fh_gx_state_filter.h $(NI)/fh_gx_matrix_filter.h $(G)/gx_state_filter/negative-test
gx_state_filter_BASE := $(b3_GXSF_BASE)
gx_state_filter_CFLAGS := -I$(b3_AUR)
gx_state_filter_SETUP := $(call b3_NEG,$(G)/gx_state_filter/negative-test,Assertion,negative)

TESTS += indirect_lighting
indirect_lighting_SRC := $(T)/indirect_lighting.c $(NS)/indirect_lighting.c $(NS)/fragment.c $(NS)/texture.c $(NS)/tev_pipeline.c $(NS)/material_layers.c
indirect_lighting_BASE := -O1 -g $(SAN)

TESTS += input_record
input_record_SRC := $(NS)/input_tape.c $(NS)/replay.c $(NS)/platform_input_stream.c $(NS)/platform_input_record.c $(T)/input_record.c
input_record_BASE := -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g $(SAN) -pthread -I$(T)/input_stub
input_record_LIBS := -lz
input_record_SETUP := $(B)/bin/input_record arm > arm.log 2>&1 && mkdir locked && (cd locked && $(B)/bin/input_record locked > ../locked.log 2>&1)
input_record_ARGS := record

TESTS += input_stream
input_stream_SRC := $(NS)/input_tape.c $(NS)/platform_input_stream.c $(T)/input_stream.c
input_stream_BASE := -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g $(SAN) -pthread -I$(T)/input_stub
input_stream_LIBS := -lz

TESTS += input_tape
input_tape_SRC := $(NS)/input_tape.c $(T)/input_tape.c
input_tape_BASE := -std=c11 -O1 -g $(SAN) -pthread

TESTS += light_lod
light_lod_SRC := $(T)/light_lod.c
light_lod_DEPS := $(NI)/fh_light_lod.h
light_lod_BASE := -std=gnu11 -O2 -g -ffp-contract=off -Wall -Wextra -Werror $(SAN)

TESTS += material
material_SRC := $(T)/material.c $(NS)/material.c
material_BASE := -O1 -g $(SAN) -fno-omit-frame-pointer

TESTS += material_key
material_key_SRC := $(T)/material_key.c
material_key_DEPS := $(NI)/fh_material_key.h $(NI)/fh_hash_bytes.h
material_key_BASE := -std=gnu11 -O2 -g -ffp-contract=off $(SAN)

$(G)/material_register/reference.c:
	mkdir -p $(@D)
	printf '#define FH_MATERIAL_GENERIC 1\n#define fh_material_compile fh_material_compile_reference\n#define fh_material_color fh_material_color_reference\n#define fh_material_subdivide fh_material_subdivide_reference\n#include "%s"\n' $(NS)/material.c > $@

TESTS += material_register
material_register_SRC := $(T)/material_register.c $(NS)/material.c $(G)/material_register/reference.c
material_register_DEPS := $(NI)/fh_material_register.h
material_register_BASE := -std=gnu11 -O2 -g -ffp-contract=off $(SAN)

TESTS += material_scope
material_scope_SRC := $(T)/material_scope.c $(NS)/material_scope.c
material_scope_DEPS := $(NI)/fh_material_scope.h
material_scope_BASE := -O2 -g -Wall -Wextra -Werror $(SAN) -pthread
material_scope_SETUP := { $(B)/bin/material_scope material-mutation > material.log 2>&1; test $$? -eq 42; } && { $(B)/bin/material_scope class-mutation > class.log 2>&1; test $$? -eq 42; }

B4_SLICE = awk 'index($$0,ENVIRON["A"])==1{p=1} index($$0,ENVIRON["Z"])==1{p=0} p' $<
B4_WERR := -Wall -Wextra -Werror
B4_AURORA := -I$(ROOT)/extern/aurora/include

TESTS += model_lod
model_lod_SRC := $(T)/model_lod.c $(NS)/model_lod.c
model_lod_LIBS := -lz

TESTS += model_resources
model_resources_SRC := $(T)/model_resources.c
model_resources_BASE := -std=gnu11 -O2 -g $(B4_WERR) $(SAN)

B4_WPASS := $(G)/model_write_barrier/model-writes.so
B4_WOBJ := $(G)/model_write_barrier/tracked.o

$(B4_WPASS): $(ROOT)/tools/model_write_pass.cpp
	mkdir -p $(@D)
	clang++-18 $$(llvm-config-18 --cxxflags) -fPIC -shared -O2 $< $$(llvm-config-18 --ldflags --libs core passes transformutils analysis) -o $@

$(B4_WOBJ): $(T)/model_write_targets.c $(B4_WPASS)
	mkdir -p $(@D)
	clang-18 -std=gnu11 -O2 -g $(B4_WERR) -fpass-plugin=$(B4_WPASS) -c $< -o $@

TESTS += model_write_barrier
model_write_barrier_SRC := $(T)/model_write_barrier.c $(B4_WOBJ)
model_write_barrier_BASE := -std=gnu11 -O2 -g $(B4_WERR) $(SAN)

TESTS += movie_disjoint
movie_disjoint_SRC := $(T)/movie_disjoint.c $(NS)/vertex_position.c
movie_disjoint_BASE := -O2 -g $(SAN)

TESTS += movie_prefetch
movie_prefetch_SRC := $(NS)/platform_movie_prefetch.c $(T)/movie_prefetch.c
movie_prefetch_BASE := -std=gnu11 -O2 -g $(B4_WERR) -Wno-deprecated-declarations $(SAN)
movie_prefetch_CFLAGS := -I$(T)/movie_prefetch_host
movie_prefetch_LIBS := -pthread

TESTS += musyx_output
musyx_output_SRC := $(T)/musyx_output.c $(ROOT)/extern/musyx/src/musyx/runtime/hw_pc_output.c
musyx_output_BASE := -O2 -g $(SAN)
musyx_output_CFLAGS := -DTARGET_PC=1 -DMUSY_TARGET=MUSY_TARGET_PC -DMUSY_VERSION_MAJOR=2 -DMUSY_VERSION_MINOR=0 -DMUSY_VERSION_PATCH=0 -I$(ROOT)/extern/musyx/include

TESTS += native_constant_light
native_constant_light_SRC := $(T)/native_constant_light.c
native_constant_light_BASE := -std=c11 -O1 -g $(B4_WERR) $(SAN)

TESTS += native_queue_batch
native_queue_batch_SRC := $(T)/native_queue_batch.c
native_queue_batch_DEPS := $(G)/native_queue_batch/batch-under-test.inc
native_queue_batch_BASE := -O2 $(B4_WERR) $(SAN)
native_queue_batch_CFLAGS := -I$(G)/native_queue_batch

$(G)/native_queue_batch/batch-under-test.inc: $(NI)/fh_batched_gxqueue.h
	mkdir -p $(@D)
	sed 's/^#include .*$$//' $< > $@

TESTS += native_scene_state
native_scene_state_SRC := $(T)/native_scene_state.c
native_scene_state_BASE := -std=gnu11 -O2 -g -ffp-contract=off $(B4_WERR) $(SAN)
native_scene_state_CFLAGS := -DTARGET_PC=1 $(B4_AURORA)

TESTS += native_splash_geometry
native_splash_geometry_SRC := $(T)/native_splash_geometry.c

TESTS += native_water_effect_hook
native_water_effect_hook_SRC := $(T)/native_water_effect_hook.c
native_water_effect_hook_DEPS := $(G)/native_water_effect_hook/waterfx_hook.inc
native_water_effect_hook_BASE := -std=c11 $(B4_WERR) -O1 -g $(SAN)
native_water_effect_hook_CFLAGS := -I$(G)/native_water_effect_hook

$(G)/native_water_effect_hook/waterfx_hook.inc: $(ROOT)/game/src/dlls/engine/19/19.c
	mkdir -p $(@D)
	A='static void fh_waterfx_triangles(' Z='void waterfx_render(' $(B4_SLICE) > $@

TESTS += owned_draw_sources
owned_draw_sources_SRC := $(T)/owned_draw_sources.c $(NS)/texture.c $(NS)/gx_compiled_geometry.c $(NS)/model_resources.c
owned_draw_sources_DEPS := $(G)/owned_draw_sources/host-types.h
owned_draw_sources_BASE := -O2 -g $(B4_WERR) -Wno-misleading-indentation -Wno-missing-field-initializers -Wno-unused-function $(SAN)
owned_draw_sources_CFLAGS := -DTARGET_PC=1 -DFH_GX_DISPATCH_BUILD=1 -include $(G)/owned_draw_sources/host-types.h $(B4_AURORA)
owned_draw_sources_LIBS := -pthread

$(G)/owned_draw_sources/host-types.h:
	mkdir -p $(@D)
	printf '%s\n' 'typedef void (*GXBreakPtCallback)(void);' > $@

b5_extract = awk -v names='$(1)' 'BEGIN{n=split(names,a,"|")} {if(!p)for(i=1;i<=n;i++)if(index($$0,a[i] "(")==1){p=1;d=0}} p{print;o=gsub(/\{/,"{");c=gsub(/\}/,"}");d+=o-c;if(d<=0&&o+c>0)p=0}' $(2)

TESTS += pica_command_chain
pica_command_chain_SRC := $(T)/pica_command_chain.c $(NS)/pica_command_chain.c
pica_command_chain_BASE := -std=gnu11 -O2 -g -Wall -Wextra -Werror $(SAN)

TESTS += pica_etc1
pica_etc1_SRC := $(T)/pica_etc1.c
pica_etc1_BASE := -O2 -g -Wall -Wextra $(SAN)
pica_etc1_SETUP := head -c 512 /dev/zero > in.etc1
pica_etc1_ARGS := in.etc1 32 32 out.rgba

TESTS += planar_shadow
planar_shadow_SRC := $(T)/planar_shadow.c
planar_shadow_BASE := -std=c11 -O1 $(SAN)

TESTS += primitive
primitive_SRC := $(T)/primitive.c
primitive_BASE := -std=c11 -O2 -g $(SAN)

TESTS += raster_channels
raster_channels_SRC := $(NS)/material.c $(NS)/tev_pipeline.c $(NS)/fragment.c $(NS)/fur_mesh.c $(NS)/texture.c $(T)/raster_channels.c
raster_channels_BASE := -Werror=incompatible-pointer-types -O1 -g $(SAN)

TESTS += receiver_plane
receiver_plane_SRC := $(T)/receiver_plane.c
receiver_plane_BASE := -std=c11 -O1 $(SAN)

TESTS += receiver_precision
receiver_precision_SRC := $(T)/receiver_precision.c
receiver_precision_BASE := -std=c11 -O1 $(SAN)

TESTS += render_floor
render_floor_SRC := $(T)/render_floor.c
render_floor_BASE := -std=c11 -O2 -g -Wall -Wextra -Werror $(SAN)

TESTS += replay
replay_SRC := $(NS)/replay.c $(T)/replay.c
replay_BASE := -std=c11 -O1 -g $(SAN)

TESTS += replay_io_gate
replay_io_gate_SRC := $(G)/replay_io_gate/gate.c $(NS)/input_tape.c
replay_io_gate_BASE := -std=c11 -O1 -g $(SAN)
$(G)/replay_io_gate/gate.c: $(T)/replay_io_gate.c $(NS)/platform_replay.c
	mkdir -p $(@D)
	sed '/ACTUAL_FUNCTIONS/,$$d' $(T)/replay_io_gate.c > $@.tmp
	$(call b5_extract,int fh_replay_dvd_complete|int fh_replay_dvd_wait,$(NS)/platform_replay.c) >> $@.tmp
	sed '1,/ACTUAL_FUNCTIONS/d' $(T)/replay_io_gate.c >> $@.tmp
	mv $@.tmp $@

TESTS += replay_walk
replay_walk_SRC := $(NS)/replay.c $(T)/replay_walk.c $(G)/replay_walk/source-oracle.c
replay_walk_BASE := -std=c11 -O1 $(SAN)
$(G)/replay_walk/source-oracle.c: $(ROOT)/game/src/main/vecmath.c $(T)/shims/replay_walk_pre.h $(T)/shims/replay_walk_post.h
	mkdir -p $(@D)
	cat $(T)/shims/replay_walk_pre.h > $@.tmp
	$(call b5_extract,int getAngle|void setMatrixFromObjectPos|void Matrix_TransformPoint,$(ROOT)/game/src/main/vecmath.c) >> $@.tmp
	cat $(T)/shims/replay_walk_post.h >> $@.tmp
	mv $@.tmp $@

TESTS += rigid_bounds
rigid_bounds_SRC := $(T)/rigid_bounds.c $(N)/wip/source/pica_rigid_bounds.c
rigid_bounds_BASE := -std=c11 -O1 $(SAN)
rigid_bounds_CFLAGS := -I$(N)/wip/include

TESTS += scene_cache_io
scene_cache_io_SRC := $(G)/scene_cache_io/scene_io.c
scene_cache_io_BASE := -std=c11 -O1 -g -Wall -Wextra -Werror $(SAN)
scene_cache_io_LIBS := -lz
scene_cache_io_SETUP := python3 -c "import sys,random;sys.path.insert(0,'$(ROOT)/tools');from prepare_block_cache import encode;raw=b'palette and positions\0'*4000+random.Random(617).randbytes(100000)+bytes(65539);open('source.bin','wb').write(raw);open('cache.bin','wb').write(encode(raw))"
scene_cache_io_ARGS := source.bin cache.bin
$(G)/scene_cache_io/scene_io.c: $(T)/scene_cache_io.c $(NS)/platform_native_scene.c $(T)/shims/scene_cache_io_pre.h $(T)/shims/scene_cache_io_post.h
	mkdir -p $(@D)
	cat $(T)/shims/scene_cache_io_pre.h > $@.tmp
	sed -n '/^static FILE \*file;/,/^static unsigned char \*directory;/p' $(NS)/platform_native_scene.c | sed '$$d' >> $@.tmp
	cat $(T)/shims/scene_cache_io_post.h >> $@.tmp
	cat $(T)/scene_cache_io.c >> $@.tmp
	mv $@.tmp $@

TESTS += scene_snapshot
scene_snapshot_SRC := $(T)/scene_snapshot.c
scene_snapshot_BASE := -std=c11 -O2 -Wall -Wextra -Werror $(SAN)

TESTS += shadow_point_routes
shadow_point_routes_SRC := $(T)/shadow_point_routes.c $(NS)/pica_shadow_points_routes.c
shadow_point_routes_BASE := -std=c11 -O1 $(SAN)

TESTS += shadow_project_depth
shadow_project_depth_SRC := $(G)/shadow_project_depth/depth.c
shadow_project_depth_BASE := -std=c11 -O1 $(SAN)
$(G)/shadow_project_depth/depth.c: $(ROOT)/game/src/main/camera.c $(T)/shims/shadow_project_depth_pre.h $(T)/shadow_project_depth.c
	mkdir -p $(@D)
	cat $(T)/shims/shadow_project_depth_pre.h > $@.tmp
	$(call b5_extract,void Camera_ApplyDecalViewport,$(ROOT)/game/src/main/camera.c) >> $@.tmp
	cat $(T)/shadow_project_depth.c >> $@.tmp
	mv $@.tmp $@

TESTS += shadow_projection
shadow_projection_SRC := $(T)/shadow_projection.c
shadow_projection_BASE := -std=c11 -O1 $(SAN)

TESTS += shadow_range_cache
shadow_range_cache_SRC := $(T)/shadow_range_cache.c $(NS)/pica_shadow_ranges.c
shadow_range_cache_BASE := -std=c11 -O1 $(SAN)

TESTS += shadow_ranges
shadow_ranges_SRC := $(T)/shadow_ranges.c $(NS)/pica_shadow_ranges.c
shadow_ranges_BASE := -std=c11 -O1 $(SAN)

B6_C11 := -std=c11 -O1 -g $(SAN)
B6_O2 := -O2 -g $(SAN)
B6_O2W := -O2 -g -Wall -Wextra -Werror $(SAN)

TESTS += shadow_receiver_batches
shadow_receiver_batches_SRC := $(T)/shadow_receiver_batches.c $(NS)/pica_shadow_receivers.c
shadow_receiver_batches_BASE := $(B6_C11)

TESTS += shadow_receiver_depth
shadow_receiver_depth_SRC := $(T)/shadow_receiver_depth.c
shadow_receiver_depth_BASE := $(B6_C11)

TESTS += shadow_spans
shadow_spans_SRC := $(T)/shadow_spans.c $(N)/wip/source/pica_shadow_spans.c $(NS)/pica_shadow_ranges.c
shadow_spans_CFLAGS := -I$(N)/wip/include
shadow_spans_BASE := $(B6_C11)

TESTS += shadow_track
shadow_track_SRC := $(T)/shadow_track.c $(NS)/pica_shadow_track.c
shadow_track_BASE := $(B6_C11)

TESTS += shadow_track_raw
shadow_track_raw_SRC := $(T)/shadow_track_raw.c $(NS)/pica_shadow_track.c $(NS)/pica_shadow_track_raw.c
shadow_track_raw_BASE := $(B6_C11)

TESTS += shadow_track_routes
shadow_track_routes_SRC := $(T)/shadow_track_routes.c $(NS)/pica_shadow_track_routes.c
shadow_track_routes_BASE := $(B6_C11)

TESTS += shadow_world
shadow_world_SRC := $(T)/shadow_world.c
shadow_world_BASE := $(B6_C11)

TESTS += stack_watermark
stack_watermark_SRC := $(T)/stack_watermark.c
stack_watermark_CFLAGS := -DFH_HOST
stack_watermark_BASE := -std=gnu11 $(B6_O2W)

TESTS += stream_flush
stream_flush_SRC := $(T)/stream_flush.c
stream_flush_DEPS := $(G)/stream_flush/flush.inc
stream_flush_CFLAGS := -I$(G)/stream_flush
stream_flush_BASE := -O1 -g $(SAN) -fno-omit-frame-pointer

$(G)/stream_flush/flush.inc: $(NS)/platform_graphics.c
	mkdir -p $(@D)
	awk '/^static void flush_stream_buffers\(void\) \{/{exit} /^static void flush_stream\(/{p=1} p' $< > $@

TESTS += surface_detail
surface_detail_SRC := $(T)/surface_detail.c
surface_detail_BASE := -std=gnu11 $(B6_O2W)

TESTS += tev_compare
tev_compare_SRC := $(T)/tev_compare.c $(NS)/fragment.c $(NS)/material.c $(NS)/texture.c
tev_compare_BASE := -O1 -g $(SAN)

TESTS += tev_pipeline
tev_pipeline_SRC := $(T)/tev_pipeline.c $(NS)/tev_pipeline.c $(NS)/material_layers.c
tev_pipeline_BASE := -O1 -g $(SAN)

TESTS += texture_16
texture_16_SRC := $(T)/texture_16.c $(NS)/texture.c
texture_16_BASE := $(B6_O2)

TESTS += texture_8
texture_8_SRC := $(T)/texture_8.c $(NS)/texture.c
texture_8_BASE := $(B6_O2)

TESTS += texture_cmpr16
texture_cmpr16_SRC := $(T)/texture_cmpr16.c $(NS)/texture.c
texture_cmpr16_BASE := $(B6_O2)

TESTS += texture_rgba
texture_rgba_SRC := $(T)/texture_rgba.c $(NS)/texture.c
texture_rgba_BASE := $(B6_O2)

TESTS += texture_pack
texture_pack_SRC := $(T)/texture_pack.c $(NS)/platform_texture_pack.c
texture_pack_BASE := -O1 -g -Wall -Wextra -Werror $(SAN) -fno-omit-frame-pointer
texture_pack_LIBS := -lz
texture_pack_ARGS := fixture.bin

TESTS += texture_resource_invalidation
texture_resource_invalidation_SRC := $(T)/texture_resource_invalidation.c $(NS)/fragment.c $(NS)/texture.c
texture_resource_invalidation_DEPS := $(G)/texture_resource_invalidation/texture-cache-invalidate.inc
texture_resource_invalidation_CFLAGS := -I$(G)/texture_resource_invalidation '-Dfh_native_scene_texture_written(a,b)=((void)(a),(void)(b))'
texture_resource_invalidation_BASE := $(B6_O2W) -Wno-unused-function -ffunction-sections -fdata-sections -Wl,--gc-sections

$(G)/texture_resource_invalidation/texture-cache-invalidate.inc: $(NS)/platform_graphics.c
	mkdir -p $(@D)
	awk '/^void fh_renderer_invalidate\(/{exit} /^static void texture_cache_invalidate\(/{p=1} p' $< > $@

TESTS += texture_write_index
texture_write_index_SRC := $(T)/texture_write_index.c
texture_write_index_BASE := -std=c11 $(B6_O2W)

TESTS += uniform_rows
uniform_rows_SRC := $(T)/uniform_rows.c
uniform_rows_BASE := -std=c11 $(B6_O2)

TESTS += uvgen_matrix
uvgen_matrix_SRC := $(T)/uvgen_matrix.c
uvgen_matrix_BASE := $(B6_O2) -ffp-contract=off

TESTS += vertex_bounds
vertex_bounds_SRC := $(T)/vertex_bounds.c $(NS)/vertex_position.c
vertex_bounds_BASE := $(B6_O2W) -ffp-contract=off


define host_test
$(B)/bin/$(1): $$($(1)_SRC) $$($(1)_DEPS) | $(B)/bin
	$$(or $$($(1)_CC),$$(CC)) $$(or $$($(1)_BASE),$$(CFLAGS_BASE)) -I$(NI) $$($(1)_CFLAGS) $$($(1)_SRC) -o $$@ $$($(1)_LIBS) -lm
run-$(1): $(B)/bin/$(1) | $(B)/log $(G)
	@rm -rf $(B)/fixture/$(1) && mkdir -p $(B)/fixture/$(1)
	@cd $(B)/fixture/$(1) && $$(if $$($(1)_SETUP),$$($(1)_SETUP) &&) $(B)/bin/$(1) $$($(1)_ARGS) > $(B)/log/$(1).log 2>&1 \
	  && echo "PASS $(1)" || (cat $(B)/log/$(1).log; echo "FAIL $(1)"; false)
.PHONY: run-$(1)
endef

$(foreach t,$(TESTS),$(eval $(call host_test,$(t))))

$(B)/bin $(B)/log $(B)/fixture $(G):
	mkdir -p $@

build: $(addprefix $(B)/bin/,$(TESTS))
check: | $(B)/log $(B)/fixture $(G)
	@$(MAKE) --no-print-directory -k $(addprefix run-,$(TESTS))
list:
	@echo $(TESTS)
clean:
	rm -rf $(B)

.PHONY: build check list clean
.DEFAULT_GOAL := check

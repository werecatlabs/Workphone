"""Tag the existing six native binding intervals without moving any timer."""
import re
from MaterialSceneCpuAttributionOverlay import overlay,native_overlay
PHASE_NAMES=('eligibility','fixed_state','transfer_metadata','terrain_shadow_bindings','material_shader_bindings','texture_sampler_bindings')
BEGIN=b'wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN, 1);'
def native_phase_overlay(raw,bridge):
    generated=native_overlay(raw,bridge)
    if generated.count(BEGIN)!=len(PHASE_NAMES):raise ValueError('Native phase boundary count differs')
    index=0
    def tag(match):
        nonlocal index
        index+=1
        return b'wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN, '+str(index).encode()+b');'
    result=re.sub(re.escape(BEGIN),tag,generated)
    if canonical_phase_overlay(result)!=generated:raise ValueError('Phase tagging changed native code or timer boundaries')
    return result

def canonical_phase_overlay(raw):
    events=re.findall(rb'wp_material_scene_native_emit\(WP_SCENE_NATIVE_BIND_BEGIN, ([1-6])\);',raw)
    if events!=[str(i).encode() for i in range(1,7)]:raise ValueError('Missing, reordered or duplicated native phase tags')
    return re.sub(rb'wp_material_scene_native_emit\(WP_SCENE_NATIVE_BIND_BEGIN, [1-6]\);',BEGIN,raw)

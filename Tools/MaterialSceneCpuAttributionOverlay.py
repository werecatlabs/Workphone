"""Test-only scene overlays, including material binding callbacks."""
from pathlib import Path
from MaterialCpuAttributionOverlay import instrument as material_overlay

def include(raw,header,export=False):
    text=raw.decode().replace('\r\n','\n');line=text.index('\n')+1
    prefix='#define WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION\n'
    if not export:prefix+='#define WORKPHONE_MATERIAL_SCENE_NO_EXPORT\n'
    text=text[:line]+prefix+'#include "'+header.as_posix()+'"\n'+text[line:]
    return text.encode()

def overlay(name,raw,header):
    if name=='ClawRendererDX11':
        return material_overlay(raw,header,False).replace(b'WORKPHONE_MATERIAL_CPU_PROBE_IMPLEMENTATION',b'WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION').replace(b'material_cpu_attribution',b'material_scene_cpu')
    raw=include(raw,header);text=raw.decode()
    if name=='MaterialProgramDX11':
        starts=[('bool MaterialProgramDX11::Publication::Bind(',False),('void MaterialProgramDX11::Publication::Unbind(',True)]
        for signature,unbind in starts:
            assert text.count(signature)==1
            start=text.index('{',text.index(signature))+1
            text=text[:start]+'\n        ::material_scene_cpu::BindingScope wpBindingCpu('+str(unbind).lower()+');'+text[start:]
        marker='        return true;\n    }\n    void MaterialProgramDX11::Publication::Unbind'
        assert text.count(marker)==1
        text=text.replace(marker,'        ::material_scene_cpu::successfulBind();\n'+marker)
    elif name=='ClawMaterialDrawRecords':
        for method,stage in [('capture','Capture'),('seal','Capture'),('clear','Retirement')]:
            signature='void ClawMaterialDrawRecords::'+method+'('
            assert text.count(signature)==1
            start=text.index('{',text.index(signature))+1
            text=text[:start]+'\n        ::material_scene_cpu::StageScope wpStageCpu(::material_scene_cpu::'+stage+');'+text[start:]
    elif name=='ClawHammerSystem':
        marker='materialUpdates.commit(); // Publish before immutable scene-job capture.'
        assert text.count(marker)==1
        text=text.replace(marker,'{ ::material_scene_cpu::StageScope wpPublicationCpu(::material_scene_cpu::Publication); materialUpdates.commit(); }')
    else:raise ValueError('Unsupported scene overlay')
    return text.encode()

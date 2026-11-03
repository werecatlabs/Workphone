"""Test-only scene overlays, including material binding callbacks."""
from pathlib import Path
from MaterialCpuAttributionOverlay import instrument as material_overlay
from MaterialCpuAttributionOverlay import closing

def include(raw,header,export=False):
    text=raw.decode().replace('\r\n','\n');line=text.index('\n')+1
    prefix='#define WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION\n'
    if not export:prefix+='#define WORKPHONE_MATERIAL_SCENE_NO_EXPORT\n'
    text=text[:line]+prefix+'#include "'+header.as_posix()+'"\n'+text[line:]
    return text.encode()

def overlay(name,raw,header):
    if name=='ClawRendererDX11':
        return material_overlay(raw,header,False).replace(b'WORKPHONE_MATERIAL_CPU_PROBE_IMPLEMENTATION',b'WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION\n#define WORKPHONE_MATERIAL_SCENE_NATIVE_BRIDGE').replace(b'material_cpu_attribution',b'material_scene_cpu')
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


def native_overlay(raw,bridge):
    """Preserve C compilation and original call order; never time Draw* or MVP/IA work."""
    text=raw.decode().replace('\r\n','\n')
    if 'WORKPHONE_MATERIAL_SCENE_NATIVE_IMPLEMENTATION' in text:raise ValueError('Already instrumented native renderer')
    signature='static wp_s32 wp_renderer_dx11_bind_geometry_pntc('
    if text.count(signature)!=1:raise ValueError('Unknown native PNTC binder')
    start=text.index(signature);end=closing(text,text.index('{',start))+1;body=text[start:end]
    # Eligibility checks contain early returns. Keep their exact code in a
    # helper so the timing pair closes before propagating any rejection.
    eligibility_start=body.index('    wp_s32 rtt_index;')
    eligibility_end=body.index('    if( !r || !geometry')
    eligibility=body[eligibility_start:eligibility_end]
    helper='static wp_s32 wp_scene_material_views_eligible(wp_renderer_dx11 *r)\n{\n'+eligibility+'    return 1;\n}\n'
    body=body[:eligibility_start]+'''    {
        wp_s32 allowed;
        wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN, 1);
        allowed=wp_scene_material_views_eligible(r);
        wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_END, allowed);
        if(!allowed) return 0;
    }
'''+body[eligibility_end:]
    emit=lambda event:'wp_material_scene_native_emit(WP_SCENE_NATIVE_'+event+', 1);'
    regions=[('    wp_renderer_dx11_apply_state( r );','    if( !wp_renderer_dx11_upload_mvp( r ) )'),
             ('    // Derive transfer ownership','    if( r->material_dirty )'),
             ('        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 3,','        ID3D11DeviceContext_IASetInputLayout('),
             ('        ID3D11DeviceContext_PSSetShader( r->context,','        ID3D11DeviceContext_IASetPrimitiveTopology('),
             ('    if( !r->pntc_bindings_valid || r->bound_textures[0]',
              '    if( !r->pntc_bindings_valid || r->bound_vertex_buffer')]
    insertions=[]
    for begin,finish in regions:
        if body.count(begin)!=1 or body.count(finish)!=1:raise ValueError('Native binding region changed')
        a,b=body.index(begin),body.index(finish)
        if a>=b or 'return ' in body[a:b]:raise ValueError('Binding region has an unbalanced exit')
        indent=begin[:len(begin)-len(begin.lstrip())]
        insertions.extend([(a,indent+emit('BIND_BEGIN')+'\n'),(b,indent+emit('BIND_END')+'\n')])
    for offset,value in sorted(insertions,reverse=True):body=body[:offset]+value+body[offset:]
    text=text[:start]+helper+body+text[end:]
    def wrap(name,args,call,before,after,declarations=''):
        nonlocal text
        signature='static wp_s32 '+name+'('
        if text.count(signature)!=1:raise ValueError('Native function shape changed: '+name)
        a=text.index(signature);b=closing(text,text.index('{',a))+1
        original=text[a:b].replace(name+'(',name+'_cpu_body(',1)
        wrapper='\nstatic wp_s32 '+name+'('+args+')\n{\n    wp_s32 result;\n'+declarations+before+'\n    result='+name+'_cpu_body('+call+');\n'+after+'\n    return result;\n}\n'
        text=text[:a]+original+wrapper+text[b:]
    wrap('wp_renderer_dx11_upload_constants',
         'wp_renderer_dx11 *r, ID3D11Buffer *buffer, const void *data, size_t size','r,buffer,data,size',
         '    if(watched) wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_BEGIN,1);',
         '    if(watched) wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_END,result);',
         '    wp_s32 watched=r && (buffer==r->cb_shadow || buffer==r->cb_terrain);\n')
    wrap('wp_renderer_dx11_upload_material','wp_renderer_dx11 *r','r',
         '    wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_BEGIN,0);',
         '    wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_END,result);')
    wrap('wp_renderer_dx11_bind_geometry_pntc',
         'wp_renderer_dx11 *r, const wp_geometry_dx11 *geometry, wp_s32 index_start, wp_s32 index_count, wp_s32 base_vertex',
         'r,geometry,index_start,index_count,base_vertex',
         '    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_BEGIN,r?r->material_upload_mode:-1);',
         '    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_END,result);')
    return ('#define WORKPHONE_MATERIAL_SCENE_NATIVE_IMPLEMENTATION\n#include "'+bridge.as_posix()+'"\n'+text).encode()

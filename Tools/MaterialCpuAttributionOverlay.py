"""Mechanical test-only renderer overlay; the material algorithm stays intact."""
import re


def masked(source):
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]), source, flags=re.S)


def closing(source, opening):
    clean=masked(source); depth=0
    for index in range(opening,len(clean)):
        if clean[index]=='{': depth+=1
        elif clean[index]=='}':
            depth-=1
            if depth==0:return index
    raise ValueError('unbalanced renderer scope')


def instrument(raw, header, baseline):
    crlf=b'\r\n' in raw
    source=raw.decode('utf-8').replace('\r\n','\n')
    if 'WORKPHONE_MATERIAL_CPU_PROBE_IMPLEMENTATION' in source:
        raise ValueError('renderer is already instrumented')
    signature='u64 ClawRendererDX11::renderMeshBatch('
    if source.count(signature)!=1:raise ValueError('ambiguous renderer entry')
    begin=source.index(signature); opening=source.index('{',begin); end=closing(source,opening)
    body=source[begin:end+1]
    loop='for( s32 submeshIndex = 0; submeshIndex < drawCount; ++submeshIndex )'
    if body.count(loop)!=1:raise ValueError('material section loop changed')
    loop_open=body.index('{',body.index(loop)); loop_end=closing(body,loop_open)
    start=('            auto pMaterial = resolveMeshMaterial( mesh, submeshIndex );' if baseline else
           '            const auto record = records->get(*binding, submeshIndex);')
    draw='            if( instances )\n            {\n                if( geometry && count > 0 && count % 3u == 0 )'
    if body.count(start)!=1 or body.count(draw)!=1:raise ValueError('material/draw boundary changed')
    # Material owners are declared after the probe, so their destruction occurs
    # before its final stop. Native draw work is between stop and resume.
    insertions=[(body.index(start),'            ::material_cpu_attribution::MaterialScope wpMaterialCpuScope;\n'),
                (body.index(draw),'            wpMaterialCpuScope.stop();\n'),
                (body.rfind('\n',0,loop_end)+1,'            wpMaterialCpuScope.resume();\n')]
    for offset,text in sorted(insertions,reverse=True):body=body[:offset]+text+body[offset:]
    # Count one logical texture resolution on both adapter and explicit-renderer
    # paths. A generated untextured fixture must never enter either path.
    resolver='const auto resolveTexture = [&]( ITexture *texture ) -> void * {'
    if resolver in body:
        if body.count(resolver)!=1:raise ValueError('Ambiguous texture resolver')
        body=body.replace(resolver,resolver+'\n                ::material_cpu_attribution::texturePath();')
    else:
        body=re.sub(r'(?m)^(\s*)(texture|reflection)->getTextureFinal\(',
                    r'\1::material_cpu_attribution::texturePath();\n\1\2->getTextureFinal(',body)
    source=source[:begin]+body+source[end+1:]
    line=source.index('\n')+1
    include='#define WORKPHONE_MATERIAL_CPU_PROBE_IMPLEMENTATION\n#include "'+header.as_posix()+'"\n'
    source=source[:line]+include+source[line:]
    return source.replace('\n','\r\n').encode() if crlf else source.encode()

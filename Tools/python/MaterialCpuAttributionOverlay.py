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
    start_pattern = r'^            const auto record = records->get\( *\*binding, submeshIndex *\);'
    if baseline:
        start_pattern = r'^            auto pMaterial = resolveMeshMaterial\( mesh, submeshIndex \);'
    starts = list(re.finditer(start_pattern, body, re.M))
    if len(starts) != 1:
        raise ValueError('material/draw boundary changed')
    start = starts[0].start()
    insertions = [(start, '            ::material_cpu_attribution::MaterialScope wpMaterialCpuScope;\n')]
    native_batch = 'wp_renderer_dx11_submit_mesh_batch( dx11, &batch, &result )'
    if native_batch in body:
        resolver = 'auto resolve = [&]( s32 submeshIndex, wp_mesh_section_dx11 &section ) -> wp_s32 {'
        if body.count(resolver) != 1 or body.count(native_batch) != 1:
            raise ValueError('material callback changed')
        resolver_open = body.index('{', body.index(resolver))
        if not resolver_open < start < closing(body, resolver_open) < body.index(native_batch):
            raise ValueError('material scope must end before native submission')
    else:
        loop = 'for( s32 submeshIndex = 0; submeshIndex < drawCount; ++submeshIndex )'
        if body.count(loop) != 1:
            raise ValueError('material section loop changed')
        loop_open = body.index('{', body.index(loop))
        loop_end = closing(body, loop_open)
        draw = '            if( instances )\n            {\n                if( geometry && count > 0 && count % 3u == 0 )'
        if 'wp_renderer_dx11_submit_mesh_section( dx11, &section )' in body:
            draw = '            constexpr auto maxIndexCount = static_cast<u32>( std::numeric_limits<wp_s32>::max() );'
        if body.count(draw) != 1:
            raise ValueError('material/draw boundary changed')
        insertions.extend([(body.index(draw), '            wpMaterialCpuScope.stop();\n'),
                           (body.rfind('\n', 0, loop_end)+1, '            wpMaterialCpuScope.resume();\n')])
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

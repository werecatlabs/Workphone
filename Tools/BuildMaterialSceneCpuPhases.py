"""Capture normal and phase-instrumented production-scene runtimes, then restore normal DLLs."""
import argparse,json,subprocess,xml.etree.ElementTree as ET
from pathlib import Path
import ProfileMaterialCpu as profile
from MaterialSceneCpuPhaseOverlay import overlay,native_phase_overlay as native_overlay


def require_idle_build():
    # Instrumentation temporarily redirects generated compile items. A second
    # build could consume them even if it does not change any source bytes.
    if subprocess.check_output(['powershell','-NoProfile','-Command',
            "@(Get-Process MSBuild -ErrorAction SilentlyContinue).Count"],text=True).strip()!='0':
        raise ValueError('Another MSBuild is active; renderer instrumentation requires an idle build root')


def build(root,build,output):
    require_idle_build()
    if output.exists():raise ValueError('Preserve previous output; choose a new directory')
    output.mkdir(parents=True)
    header=root/'Samples/cpp/VehicleAdvanced/MaterialSceneCpuPhaseProbe.hpp'
    inputs=lambda:{**profile.all_engine_sources(root),**{str(p.relative_to(root)):profile.digest(p)
                   for p in (root/'Samples/cpp/VehicleAdvanced').glob('*') if p.is_file()}}
    receipt={'status':'BUILDING','engine_and_sample_before':inputs()}
    def save(): (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    def run(command,label):
        with (output/(label+'.log')).open('w') as log:
            result=subprocess.run(command,cwd=root,stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise ValueError(label+' failed')
    save()
    run(['cmake','--build',str(build),'--config','RelWithDebInfo','--target','SampleVehicleAdvanced',
         '--parallel','4','--','/nr:false'],'normal_build')
    if inputs()!=receipt['engine_and_sample_before']:
        raise ValueError('Engine/sample source drift during normal build; refusing generated-project overrides')
    runtime=root/'bin/windows/v145/x64/MD/RelWithDebInfo';exe=runtime/'SampleVehicleAdvanced.exe'
    _,_,receipt['normal_runtime']=profile.freeze(exe,output/'runtime_normal')
    receipt['build_provenance']=profile.build_provenance(root,build)
    compiler=Path(receipt['build_provenance']['compiler']['CXX']['CMAKE_CXX_COMPILER'])
    msbuild=Path(compiler.as_posix().split('/VC/')[0])/'MSBuild/Current/Bin/amd64/MSBuild.exe'
    project=next(build.rglob('WPGraphics.vcxproj'));original=project.read_bytes()
    native_project=next(build.rglob('WorkphonePlatformWin32.vcxproj'));native_original=native_project.read_bytes()
    ns='http://schemas.microsoft.com/developer/msbuild/2003';ET.register_namespace('',ns)
    tree=ET.fromstring(original);sources={};objects=[]
    names=('ClawRendererDX11','ClawMaterialDrawRecords','MaterialProgramDX11','ClawHammerSystem')
    for name in names:
        source=root/'Engine/cpp/Source/WPGraphics'/(name+'.cpp');raw=source.read_bytes()
        target=output/(name+'.cpp');target.write_bytes(overlay(name,raw,header))
        (output/(name+'.original.cpp')).write_bytes(raw)
        sources[str(source)]={'source_sha256':profile.digest(source),'overlay_sha256':profile.digest(target)}
        items=[i for i in tree.findall('.//{'+ns+'}ClCompile') if i.get('Include','').replace('\\','/').lower()==source.as_posix().lower()]
        if len(items)!=1:raise ValueError('Ambiguous renderer compile input')
        items[0].set('Include',str(target));node=ET.SubElement(items[0],'{'+ns+'}AdditionalIncludeDirectories')
        node.text=str(source.parent)+';%(AdditionalIncludeDirectories)'
        found=[p for p in build.rglob(name+'.obj') if 'WPGraphics.dir' in str(p) and 'RelWithDebInfo' in p.parts]
        if len(found)!=1 or not found[0].resolve().is_relative_to(build.resolve()):raise ValueError('Ambiguous renderer object')
        objects+=found
    edited=ET.tostring(tree,encoding='utf-8',xml_declaration=True)
    native_source=root/'Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c'
    bridge=header.with_name('MaterialSceneCpuNativeBridge.h')
    native_target=output/native_source.name
    native_target.write_bytes(native_overlay(native_source.read_bytes(),bridge))
    (output/'workphone_graphics_renderer_dx11.original.c').write_bytes(native_source.read_bytes())
    native_tree=ET.fromstring(native_original)
    entries=[i for i in native_tree.findall('.//{'+ns+'}ClCompile') if i.get('Include','').replace('\\','/').lower()==native_source.as_posix().lower()]
    if len(entries)!=1:raise ValueError('Ambiguous native C compile input')
    entries[0].set('Include',str(native_target))
    ET.SubElement(entries[0],'{'+ns+'}CompileAs').text='CompileAsC'
    ET.SubElement(entries[0],'{'+ns+'}AdditionalIncludeDirectories').text=str(native_source.parent)+';%(AdditionalIncludeDirectories)'
    native_edited=ET.tostring(native_tree,encoding='utf-8',xml_declaration=True)
    native_objects=[p for p in build.rglob('workphone_graphics_renderer_dx11.obj') if 'WorkphonePlatformWin32.dir' in str(p) and 'RelWithDebInfo' in p.parts]
    if len(native_objects)!=1 or not native_objects[0].resolve().is_relative_to(build.resolve()):raise ValueError('Ambiguous native C object')
    command=[str(msbuild),str(project),'/p:Configuration=RelWithDebInfo','/p:Platform=x64',
             '/p:BuildProjectReferences=false','/m:4','/nr:false','/v:minimal']
    native_command=[str(msbuild),str(native_project),*command[2:]]
    library=root/'libs/windows/v145/x64/MD/RelWithDebInfo/WorkphonePlatformWin32.lib'
    receipt.update(sources=sources,probe_sha256=profile.digest(header),protocol='material_scene_cpu_v4',
        native_source=str(native_source),native_source_sha256=profile.digest(native_source),
        native_overlay_sha256=profile.digest(native_target),native_bridge_sha256=profile.digest(bridge),
        phase_protocol='material_native_phases_v1',
        aggregate_probe_sha256=profile.digest(header.with_name('MaterialSceneCpuAttributionProbe.hpp')),
        phase_contract_sha256=profile.digest(header.with_name('MaterialSceneCpuNativePhases.hpp')),
        overlay_tool_sha256=profile.digest(Path(__file__).with_name('MaterialSceneCpuPhaseOverlay.py')))
    save()
    require_idle_build()
    try:
        native_project.write_bytes(native_edited);project.write_bytes(edited)
        run(native_command,'instrumented_native_c_build')
        run(command,'instrumented_build')
        exports=profile.command_output([str(compiler.with_name('dumpbin.exe')),'/exports',str(runtime/'WPGraphics.dll')],root)
        if 'wp_material_scene_native_phases_v1' not in exports or 'wp_material_scene_cpu_v4' not in exports:raise ValueError('Native scene probe export missing')
        _,_,receipt['profiled_runtime']=profile.freeze(exe,output/'runtime_profiled')
    finally:
        for target,before,after in ((project,original,edited),(native_project,native_original,native_edited)):
            if target.read_bytes() not in (before,after):raise ValueError('Generated project changed concurrently; refusing blind restoration')
            target.write_bytes(before)
        for obj in native_objects:
            if obj.exists():obj.unlink()
        run(native_command,'restore_normal_native_c_build')
        for obj in objects:
            if obj.exists():obj.unlink()
        run(command,'restore_normal_build')
    exports=profile.command_output([str(compiler.with_name('dumpbin.exe')),'/exports',str(runtime/'WPGraphics.dll')],root)
    if 'wp_material_scene_native_phases_v1' in exports or 'wp_material_scene_cpu_v4' in exports:raise ValueError('Normal renderer still contains scene probe')
    symbols=profile.command_output([str(compiler.with_name('dumpbin.exe')),'/symbols',str(library)],root)
    if 'wp_material_scene_native_set_callback' in symbols:raise ValueError('Normal C library still contains scene probe')
    receipt['engine_and_sample_after']=inputs()
    if receipt['engine_and_sample_before']!=receipt['engine_and_sample_after']:raise ValueError('Source drift during capture')
    if project.read_bytes()!=original or native_project.read_bytes()!=native_original:raise ValueError('Generated project restoration failed')
    receipt.update(status='COMPLETE',normal_renderer_restored=True,normal_native_library_restored=True,source_unchanged=True);save()
    return receipt


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for key in ('root','build','output'):ap.add_argument('--'+key,type=Path,required=True)
    a=ap.parse_args()
    if a.output.exists():ap.error('Preserve previous output; choose a new directory')
    try:build(a.root.resolve(),a.build.resolve(),a.output.resolve())
    except Exception as error:
        path=a.output/'receipt.json'
        if path.exists():
            receipt=json.loads(path.read_text())
            if receipt.get('status')!='COMPLETE':
                receipt.update(status='REJECTED',error=str(error));path.write_text(json.dumps(receipt,indent=2)+'\n')
        raise
    print('Scene CPU overlay capture complete; normal renderer restored')

if __name__=='__main__':main()

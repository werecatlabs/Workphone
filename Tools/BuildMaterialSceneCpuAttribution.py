"""Capture normal and instrumented production-scene runtimes, then restore normal DLLs."""
import argparse,json,subprocess,xml.etree.ElementTree as ET
from pathlib import Path
import ProfileMaterialCpu as profile
from MaterialSceneCpuAttributionOverlay import overlay


def build(root,build,output):
    if output.exists():raise ValueError('Preserve previous output; choose a new directory')
    output.mkdir(parents=True)
    header=root/'Samples/cpp/VehicleAdvanced/MaterialSceneCpuAttributionProbe.hpp'
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
    runtime=root/'bin/windows/v145/x64/MD/RelWithDebInfo';exe=runtime/'SampleVehicleAdvanced.exe'
    _,_,receipt['normal_runtime']=profile.freeze(exe,output/'runtime_normal')
    receipt['build_provenance']=profile.build_provenance(root,build)
    compiler=Path(receipt['build_provenance']['compiler']['CXX']['CMAKE_CXX_COMPILER'])
    msbuild=Path(compiler.as_posix().split('/VC/')[0])/'MSBuild/Current/Bin/amd64/MSBuild.exe'
    project=next(build.rglob('WPGraphics.vcxproj'));original=project.read_bytes()
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
    command=[str(msbuild),str(project),'/p:Configuration=RelWithDebInfo','/p:Platform=x64',
             '/p:BuildProjectReferences=false','/m:4','/nr:false','/v:minimal']
    receipt.update(sources=sources,probe_sha256=profile.digest(header));save();project.write_bytes(edited)
    try:
        run(command,'instrumented_build')
        exports=profile.command_output([str(compiler.with_name('dumpbin.exe')),'/exports',str(runtime/'WPGraphics.dll')],root)
        if 'wp_material_scene_cpu_v1' not in exports:raise ValueError('Scene probe export missing')
        _,_,receipt['profiled_runtime']=profile.freeze(exe,output/'runtime_profiled')
    finally:
        if project.read_bytes()!=edited:raise ValueError('Generated project changed concurrently; refusing blind restoration')
        project.write_bytes(original)
        for obj in objects:obj.unlink()
        run(command,'restore_normal_build')
    exports=profile.command_output([str(compiler.with_name('dumpbin.exe')),'/exports',str(runtime/'WPGraphics.dll')],root)
    if 'wp_material_scene_cpu_v1' in exports:raise ValueError('Normal renderer still contains scene probe')
    receipt['engine_and_sample_after']=inputs()
    if receipt['engine_and_sample_before']!=receipt['engine_and_sample_after']:raise ValueError('Source drift during capture')
    if project.read_bytes()!=original:raise ValueError('Generated project restoration failed')
    receipt.update(status='COMPLETE',normal_renderer_restored=True,source_unchanged=True);save()
    return receipt


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for key in ('root','build','output'):ap.add_argument('--'+key,type=Path,required=True)
    a=ap.parse_args()
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

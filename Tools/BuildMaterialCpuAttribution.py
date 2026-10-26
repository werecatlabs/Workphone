"""Build a test-only renderer overlay without changing engine source files."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET
import ProfileMaterialCpu as profile
from MaterialCpuAttributionOverlay import instrument


def build_overlay(root,build,output,fixture_root,baseline):
    if output.exists():raise ValueError('output exists; preserve previous evidence')
    output.mkdir(parents=True)
    receipt={'status':'BUILDING','root':str(root),'build':str(build),'baseline':baseline,
             'engine_sources_before':profile.all_engine_sources(root)}
    def save(): (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    def run(command,label):
        with (output/(label+'.log')).open('w') as log:
            result=subprocess.run(command,cwd=root,stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise ValueError(label+' failed; inspect its log')
    if baseline:
        if profile.command_output(['git','rev-parse','HEAD'],root)!=profile.BASELINE_REF or \
                profile.command_output(['git','diff','--name-only','HEAD','--','Engine'],root):
            raise ValueError('baseline engine must remain the unmodified original revision')
        for name in ('MaterialCpuProfile.hpp','MaterialCpuAttributionProbe.hpp','MaterialCpuAttributionClient.hpp'):
            shutil.copy2(fixture_root/'Tests/cpp'/name,root/'Tests/cpp'/name)
    save()
    run(['cmake','--build',str(build),'--config','RelWithDebInfo','--target',
         'WPMaterialLoadSequenceTests','--parallel','4','--','/nr:false'],'normal_build')
    runtime=root/'bin/windows/v145/x64/MD/RelWithDebInfo'
    exe=runtime/'WPMaterialLoadSequenceTests.exe'
    _,_,normal=profile.freeze(exe,output/'runtime_unprobed')
    receipt['unprobed_runtime']=normal
    receipt['build_provenance']=profile.build_provenance(root,build)
    compiler=Path(receipt['build_provenance']['compiler']['CXX']['CMAKE_CXX_COMPILER'])
    install=Path(compiler.as_posix().split('/VC/')[0])
    msbuild=install/'MSBuild/Current/Bin/amd64/MSBuild.exe'
    dumpbin=compiler.with_name('dumpbin.exe')
    projects=list(build.rglob('WPGraphics.vcxproj'))
    if len(projects)!=1:raise ValueError('ambiguous WPGraphics project')
    project=projects[0]; original_project=project.read_bytes()
    source=root/'Engine/cpp/Source/WPGraphics/ClawRendererDX11.cpp'
    original_source=source.read_bytes(); probe=root/'Tests/cpp/MaterialCpuAttributionProbe.hpp'
    overlay=output/'ClawRendererDX11.cpp'; overlay.write_bytes(instrument(original_source,probe,baseline))
    (output/'ClawRendererDX11.original.cpp').write_bytes(original_source)
    receipt.update(renderer_source_sha256=profile.digest(source),overlay_sha256=profile.digest(overlay),
                   probe_sha256=profile.digest(probe),fixture_sha256=profile.digest(root/'Tests/cpp/MaterialCpuProfile.hpp'))
    namespace={'ms':'http://schemas.microsoft.com/developer/msbuild/2003'}
    ET.register_namespace('',namespace['ms'])
    document=ET.fromstring(original_project)
    items=[item for item in document.findall('.//ms:ClCompile',namespace)
           if item.get('Include','').replace('\\','/').lower()==source.as_posix().lower()]
    if len(items)!=1:raise ValueError('renderer compile item changed')
    item=items[0];item.set('Include',str(overlay))
    includes=ET.SubElement(item,'{'+namespace['ms']+'}AdditionalIncludeDirectories')
    includes.text=str(source.parent)+';%(AdditionalIncludeDirectories)'
    edited=ET.tostring(document,encoding='utf-8',xml_declaration=True)
    command=[str(msbuild),str(project),'/p:Configuration=RelWithDebInfo','/p:Platform=x64',
             '/p:BuildProjectReferences=false','/m:4','/nr:false','/v:minimal']
    project.write_bytes(edited)
    try:
        run(command,'instrumented_build')
        exports=profile.command_output([str(dumpbin),'/exports',str(runtime/'WPGraphics.dll')],root)
        if 'wp_material_cpu_attribution_v1' not in exports:raise ValueError('probe export missing')
        _,_,frozen=profile.freeze(exe,output/'runtime_profiled')
        receipt['profiled_runtime']=frozen
    finally:
        if project.read_bytes()!=edited:raise ValueError('generated project changed concurrently; refusing blind restoration')
        project.write_bytes(original_project)
        # Force only this translation unit back to its original compile input.
        objects=[path for path in build.rglob('ClawRendererDX11.obj')
                 if 'WPGraphics.dir' in str(path) and 'RelWithDebInfo' in path.parts]
        if len(objects)!=1:raise ValueError('ambiguous renderer object during restoration')
        if not objects[0].resolve().is_relative_to(build.resolve()):raise ValueError('object escapes build root')
        objects[0].unlink()
        run(command,'restore_normal_build')
    exports=profile.command_output([str(dumpbin),'/exports',str(runtime/'WPGraphics.dll')],root)
    if 'wp_material_cpu_attribution_v1' in exports:raise ValueError('shipping renderer still contains probe')
    if source.read_bytes()!=original_source or project.read_bytes()!=original_project:
        raise ValueError('renderer source/project restoration failed')
    receipt['engine_sources_after']=profile.all_engine_sources(root)
    if receipt['engine_sources_before']!=receipt['engine_sources_after']:
        raise ValueError('engine sources changed during overlay build')
    receipt.update(status='COMPLETE',engine_source_unchanged=True,normal_renderer_restored=True,
                   normal_runtime_after=profile.modules(exe))
    save();return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for field in ('root','build','output','fixture-root'):parser.add_argument('--'+field,type=Path,required=True)
    parser.add_argument('--baseline',action='store_true');args=parser.parse_args()
    try:
        result=build_overlay(args.root.resolve(),args.build.resolve(),args.output.resolve(),args.fixture_root.resolve(),args.baseline)
    except Exception as error:
        receipt=args.output/'receipt.json'
        if receipt.is_file():
            data=json.loads(receipt.read_text())
            # Preserve completed evidence when a caller accidentally reuses it.
            if data.get('status')!='COMPLETE':
                data.update(status='REJECTED',error=str(error))
                receipt.write_text(json.dumps(data,indent=2)+'\n')
        raise
    print(json.dumps({'status':result['status'],'normal_renderer_restored':True}))

if __name__=='__main__':main()

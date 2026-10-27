"""Revalidate immutable phase overlays, scope, output hashes and exact phase sums."""
import argparse,json,subprocess
from argparse import Namespace
from pathlib import Path
import ProfileMaterialCpu as cpu
import ProfileProductionScene as scene
import ProfileMaterialSceneCpu as aggregate
import ProfileMaterialNativePhases as phases
from MaterialSceneCpuPhaseOverlay import overlay,native_phase_overlay,canonical_phase_overlay
from MaterialSceneCpuAttributionOverlay import native_overlay

def verify_build(path,root):
    receipt=json.loads(path.read_text())
    if receipt.get('status')!='COMPLETE' or receipt.get('phase_protocol')!='material_native_phases_v1' or not all(receipt.get(k) for k in ('source_unchanged','normal_renderer_restored','normal_native_library_restored')):raise ValueError('Incomplete phase capture')
    if receipt['engine_and_sample_before']!=receipt['engine_and_sample_after'] or phases.current_inputs(root)!=receipt['engine_and_sample_after']:raise ValueError('Phase build source drift')
    header=root/'Samples/cpp/VehicleAdvanced/MaterialSceneCpuPhaseProbe.hpp';bridge=header.with_name('MaterialSceneCpuNativeBridge.h')
    for file,key in ((header,'probe_sha256'),(bridge,'native_bridge_sha256'),(header.with_name('MaterialSceneCpuAttributionProbe.hpp'),'aggregate_probe_sha256'),(header.with_name('MaterialSceneCpuNativePhases.hpp'),'phase_contract_sha256')):
        if cpu.digest(file)!=receipt[key]:raise ValueError('Phase header drift: '+str(file))
    for source,fields in receipt['sources'].items():
        original=path.parent/(Path(source).stem+'.original.cpp');generated=path.parent/(Path(source).stem+'.cpp')
        if cpu.digest(original)!=fields['source_sha256'] or cpu.digest(generated)!=fields['overlay_sha256'] or generated.read_bytes()!=overlay(Path(source).stem,original.read_bytes(),header):raise ValueError('C++ overlay reconstruction differs')
    original=path.parent/'workphone_graphics_renderer_dx11.original.c';generated=path.parent/'workphone_graphics_renderer_dx11.c'
    if cpu.digest(original)!=receipt['native_source_sha256'] or cpu.digest(generated)!=receipt['native_overlay_sha256'] or generated.read_bytes()!=native_phase_overlay(original.read_bytes(),bridge):raise ValueError('Native phase overlay reconstruction differs')
    if original.read_bytes()!=Path(receipt['native_source']).read_bytes() or canonical_phase_overlay(generated.read_bytes())!=native_overlay(original.read_bytes(),bridge):raise ValueError('Native algorithm or timer boundaries changed')
    for mode,field in (('normal','normal_runtime'),('profiled','profiled_runtime')):
        if cpu.modules(path.parent/('runtime_'+mode)/'SampleVehicleAdvanced.exe')!=receipt[field]:raise ValueError('Captured phase runtime changed')
    compiler=Path(receipt['build_provenance']['compiler']['CXX']['CMAKE_CXX_COMPILER'])
    exports=subprocess.check_output([str(compiler.with_name('dumpbin.exe')),'/exports',str(root/'bin/windows/v145/x64/MD/RelWithDebInfo/WPGraphics.dll')],text=True)
    symbols=subprocess.check_output([str(compiler.with_name('dumpbin.exe')),'/symbols',str(root/'libs/windows/v145/x64/MD/RelWithDebInfo/WorkphonePlatformWin32.lib')],text=True)
    if 'wp_material_scene_cpu_v4' in exports or 'wp_material_scene_native_phases_v1' in exports or 'wp_material_scene_native_set_callback' in symbols:raise ValueError('Shipping artifacts still contain test probes')
    return receipt

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--receipt',type=Path,required=True);ap.add_argument('--scene',type=Path);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);a=ap.parse_args()
    a.root=a.root.resolve();a.receipt=a.receipt.resolve()
    if a.output.exists():ap.error('Preserve earlier verification; use fresh output')
    receipt=verify_build(a.receipt,a.root);evidence=[a.receipt,Path(__file__)];result={'status':'BUILD_AND_PROBE_SCOPE_PASS','m6_complete':False,'production_cpu_accepted':False,'checks':{'exact_cpp_and_c_overlay_reconstruction':True,'native_algorithm_and_timer_boundaries_unchanged':True,'aggregate_v4_abi_retained':True,'captured_runtimes_unchanged':True,'normal_native_and_renderer_probe_free':True}}
    if a.scene:
        manifest=a.scene.resolve()/'manifest.json';m=json.loads(manifest.read_text());evidence.append(manifest)
        if m['status']!='COMPLETE' or m['protocol']!='material_native_phases_v1' or m['repetitions']!=3 or len(m['runs'])!=24:raise ValueError('Incomplete production phase collection')
        if m['source_inputs']!=receipt['engine_and_sample_after'] or m['asset_inventory']!=scene.asset_inventory():raise ValueError('Scene source or assets changed')
        for path,h in m['tool_and_receipt_sha256'].items():
            if cpu.digest(Path(path))!=h:raise ValueError('Scene tool or receipt changed')
        for key,files in m['frozen_runtime'].items():
            if scene.frozen_identities(manifest.parent/key/'runtime')!=files:raise ValueError('Scene runtime changed')
        args=Namespace(seed=42,quality='high',width=1280,height=720,smoke=False)
        for run in m['runs']:
            for path,h in run['outputs'].items():
                if cpu.digest(Path(path))!=h:raise ValueError('Raw phase output changed')
            profile_path=Path(run['command'][run['command'].index('--scene-profile')+1]);p=json.loads(profile_path.read_text());review=json.loads(profile_path.with_name('render.json').read_text());plugins=Path(run['command'][run['command'].index('--plugins')+1])
            if p!=run['profile'] or review['device']!=run['device']:raise ValueError('Stored phase profile/device differs')
            scene.verify_profile_scope(p,review,plugins);scene.validate(p,review,'cpu','follow',p['processId'],args,m['requested_seconds'])
            active=run['mode'] in ('aggregate','phases');aggregate.edits(p,run['edit_case'],run['variant']=='custom',active)
            if active:
                if aggregate.material(p,run['variant']=='custom','combined')!=run['material']:raise ValueError('Aggregate reparse differs')
                c=p['materialCpu'];raw=32*sum(c[k] for k in ('preparationSampleCycles','bindingSampleCycles','nativeUploadSampleCycles','nativeBindingSampleCycles'))+sum(c[k] for k in ('publicationCommitCycles','captureSealCycles','recordClearCycles','authoringCycles','editPublicationCycles'))
                if raw/p['renderOwnerUpdatesMeasured']!=run['material']['raw_combined_cycles_per_frame']:raise ValueError('Material sum includes incorrect intervals')
            elif p.get('materialCpu') is not None:raise ValueError('Inactive control recorded aggregate CPU')
            if run['mode']=='phases':
                if phases.phases(p)!=run['phases']:raise ValueError('Phase reparse differs')
                rows=p['nativeMaterialPhases']['phases'];c=p['materialCpu']
                if sum(r['cycles'] for r in rows)!=c['nativeBindingSampleCycles'] or sum(r['segments'] for r in rows)!=c['nativeBindingSampleSegments'] or sum(r['regions'] for r in rows)!=c['nativeBindingRegions']:raise ValueError('Independent phase partition mismatch')
            elif p.get('nativeMaterialPhases') is not None:raise ValueError('Phase-disabled control recorded phases')
        summary=phases.summarize(m['runs'],3)
        if summary!=m['summary']:raise ValueError('Phase/control medians differ')
        if not all(r['profile']['workload']==m['runs'][0]['profile']['workload'] and r['device']==m['runs'][0]['device'] for r in m['runs']):raise ValueError('Geometry/inventory/device/target mismatch')
        result.update(status='PRODUCTION_NATIVE_PHASE_ATTRIBUTION_PASS',production_cpu_accepted=True,processes=24,phase_processes=9,aggregate_controls=9,normal_inactive_controls=6,summary=summary,device=m['runs'][0]['device'])
        result['checks'].update(raw_outputs_reparsed=True,exact_phase_partition=True,material_sum_not_double_counted=True,complete_alternating_mode_and_control_coverage=True,same_geometry_inventory_device_target=True)
    result['limitations']=['Raw owner cycles include clock/observer cost; phase-mode bookkeeping has aggregate-only controls','Phase counters reuse aggregate clock pairs; shares describe sampled intervals and are not uninstrumented driver CPU percentages','Three processes per active mode/case; normal/inactive controls are single processes','Built-in none/all and custom all; follow view, combined uploads only','WARP/software rendering; no physical cache/PMU/GPU or whole-frame p95 certification','Background workloads not fully controlled; no renderer optimization or speedup claim']
    result['evidence_sha256']={str(p.resolve()):cpu.digest(p) for p in evidence};a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k in ('status','production_cpu_accepted','processes')}))
if __name__=='__main__':main()

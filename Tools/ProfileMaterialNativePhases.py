"""Validate additive native phase attribution and aggregate-only observer controls."""
import argparse,json,math,os,shutil,statistics,subprocess
from pathlib import Path
import ProfileMaterialCpu as cpu
import ProfileProductionScene as scene
import ProfileMaterialSceneCpu as attribution
from MaterialSceneCpuPhaseOverlay import PHASE_NAMES
CASES=(('textured','none'),('textured','all'),('custom','all'))

def phases(profile):
    p=profile.get('nativeMaterialPhases');m=profile.get('materialCpu');frames=profile['renderOwnerUpdatesMeasured']
    if not isinstance(p,dict) or p.get('protocol')!='material_native_phases_v1' or p.get('clock')!='QueryThreadCycleTime':raise ValueError('Missing native phase protocol')
    for key in ('thread','stride','draws','sampledDraws','errors'):
        if type(p.get(key)) is not int or p[key]<0:raise ValueError('Invalid phase identity counter')
    if type(frames) is not int or frames<=0:raise ValueError('Invalid phase frame coverage')
    if not isinstance(m,dict) or m.get('protocol')!='material_scene_cpu_v4' or any(type(m.get(k)) is not int or m[k]<0 for k in ('thread','nativeDrawCalls','nativeSampledDraws','nativeBindingSampleCycles','nativeBindingSampleSegments','nativeBindingRegions')):raise ValueError('Invalid aggregate phase counters')
    if not 0<p['sampledDraws']<=p['draws']:raise ValueError('Empty phase draw coverage')
    if p['errors'] or p['thread']!=profile['measurementThreadId'] or p['thread']!=m['thread'] or p['stride']!=32 or p['draws']!=m['nativeDrawCalls'] or p['sampledDraws']!=m['nativeSampledDraws']:raise ValueError('Native phase owner/sampling coverage differs')
    rows=p.get('phases');n=p['draws'];selected=p['sampledDraws']
    if not isinstance(rows,list) or len(rows)!=6:raise ValueError('Incomplete native phases')
    for i,row in enumerate(rows):
        if not isinstance(row,dict):raise ValueError('Invalid phase record')
        if row.get('id')!=i+1 or type(row.get('id')) is not int or row.get('name')!=PHASE_NAMES[i]:raise ValueError('Reordered/unknown native phase')
        if any(type(row.get(k)) is not int or row[k]<0 for k in ('cycles','segments','regions')):raise ValueError('Invalid native phase counter')
        if row['segments']>row['regions'] or (not row['segments'] and row['cycles']):raise ValueError('Unsampled native phase has CPU')
        if i in (0,1,2,5) and (row['regions']!=n or row['segments']!=selected):raise ValueError('Unconditional native phase missed draws')
        if i in (3,4) and (row['regions']>n or row['segments']>selected):raise ValueError('Conditional native phase exceeds draws')
    if any(rows[3][k]!=rows[4][k] for k in ('regions','segments')):raise ValueError('Conditional native binding phases differ')
    if sum(r['cycles'] for r in rows)!=m['nativeBindingSampleCycles'] or sum(r['segments'] for r in rows)!=m['nativeBindingSampleSegments'] or sum(r['regions'] for r in rows)!=m['nativeBindingRegions']:raise ValueError('Native phase partition does not equal aggregate')
    total=m['nativeBindingSampleCycles']
    return [{'name':r['name'],'raw_cycles_per_frame':r['cycles']*32/frames,'raw_binding_share':r['cycles']/total if total else 0,
             'sample_cycles_per_region':r['cycles']/r['segments'] if r['segments'] else 0,
             'sampled_segments':r['segments'],'regions_per_frame':r['regions']/frames,
             'median_pair_tax_adjusted_cycles_per_frame':max(0,r['cycles']-r['segments']*m['clockOverheadMedian'])*32/frames,
             'maximum_pair_tax_subtracted_cycles_per_frame':max(0,r['cycles']-r['segments']*m['clockOverheadMax'])*32/frames} for r in rows]

def tasks(repetitions):
    result=[(variant,case,mode,0) for variant,case in CASES for mode in ('normal','inactive')]
    for rep in range(1,repetitions+1):
        for index,(variant,case) in enumerate(CASES if rep%2 else reversed(CASES)):
            modes=('aggregate','phases') if (rep+index)%2 else ('phases','aggregate')
            result.extend((variant,case,mode,rep) for mode in modes)
    return result

def summarize(runs,repetitions):
    expected=set(tasks(repetitions));actual={(r['variant'],r['edit_case'],r['mode'],r['repetition']) for r in runs}
    if actual!=expected or len(runs)!=len(expected):raise ValueError('Incomplete native phase/control coverage')
    results=[]
    for variant,case in CASES:
        group=[r for r in runs if r['variant']==variant and r['edit_case']==case]
        modes={mode:sorted((r for r in group if r['mode']==mode),key=lambda r:r['repetition']) for mode in ('aggregate','phases')}
        medians={mode:{key:statistics.median(r['material'][key] for r in values) for key in values[0]['material'] if key!='raw_counters'} for mode,values in modes.items()}
        phaseRows=[{**{'name':name},**{key:statistics.median(r['phases'][i][key] for r in modes['phases']) for key in modes['phases'][0]['phases'][i] if key!='name'}} for i,name in enumerate(PHASE_NAMES)]
        results.append({'variant':variant,'edit_case':case,'repetitions':repetitions,'phase_medians':phaseRows,'material_medians':medians,
            'owner_mean_ms':{mode:statistics.median(r['profile']['renderOwnerUpdateMeanMs'] for r in group if r['mode']==mode) for mode in ('normal','inactive','aggregate','phases')},
            'observer_pairs':[{'repetition':a['repetition'],'phase_vs_aggregate_native_binding_ratio':b['material']['raw_native_binding_cycles_per_frame']/a['material']['raw_native_binding_cycles_per_frame'],
                'phase_vs_aggregate_total_ratio':b['material']['raw_combined_cycles_per_frame']/a['material']['raw_combined_cycles_per_frame']} for a,b in zip(modes['aggregate'],modes['phases'])]})
    return results

def current_inputs(root):
    return {**cpu.all_engine_sources(root),**{str(p.relative_to(root)):cpu.digest(p) for p in (root/'Samples/cpp/VehicleAdvanced').glob('*') if p.is_file()}}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('receipt','plugins','output'):ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);ap.add_argument('--repetitions',type=int,default=3);ap.add_argument('--seconds',type=float,default=30)
    a=ap.parse_args()
    for key in ('root','receipt','plugins','output'):setattr(a,key,getattr(a,key).resolve())
    if a.output.exists() or not 1<=a.repetitions<=3 or not math.isfinite(a.seconds) or a.seconds<30:ap.error('Use fresh output, 1-3 repetitions and at least 30 seconds')
    a.seed=42;a.quality='high';a.width=1280;a.height=720;a.resolution='1280x720';a.smoke=False
    receipt=json.loads(a.receipt.read_text())
    if receipt.get('status')!='COMPLETE' or receipt.get('phase_protocol')!='material_native_phases_v1' or not all(receipt.get(k) for k in ('source_unchanged','normal_renderer_restored','normal_native_library_restored')):raise ValueError('Incomplete phase runtime provenance')
    if receipt['engine_and_sample_before']!=receipt['engine_and_sample_after'] or current_inputs(a.root)!=receipt['engine_and_sample_after']:raise ValueError('Engine/sample source drift')
    a.output.mkdir(parents=True);executables={};plugins={};frozen={}
    for mode,field in (('normal','normal_runtime'),('profiled','profiled_runtime')):
        original=a.receipt.parent/('runtime_'+mode)/'SampleVehicleAdvanced.exe'
        if cpu.modules(original)!=receipt[field]:raise ValueError('Phase build runtime drift')
        folder=a.output/mode;folder.mkdir();executables[mode],plugins[mode],_,frozen[mode]=scene.snapshot_runtime(original,a.plugins,folder,False)
    identities={str(p.resolve()):cpu.digest(p) for p in (a.receipt,Path(__file__),Path(attribution.__file__),Path(cpu.__file__),Path(scene.__file__),Path(__file__).with_name('MaterialSceneCpuPhaseOverlay.py'),Path(__file__).with_name('MaterialSceneCpuAttributionOverlay.py'),Path(__file__).with_name('MaterialCpuAttributionOverlay.py'))}
    report={'status':'RUNNING','protocol':'material_native_phases_v1','m6_complete':False,'runtime_optimization':False,'hardware_cache_acceptance':False,'frame_p95_acceptance':False,'background_workloads_controlled':False,
        'scope':'Native phase attribution; same instrumented binary for aggregate/phases; built-in none/all and custom all, follow view, combined uploads',
        'repetitions':a.repetitions,'requested_seconds':a.seconds,'receipt':str(a.receipt),'source_inputs':receipt['engine_and_sample_after'],'frozen_runtime':frozen,'tool_and_receipt_sha256':identities,'asset_inventory':scene.asset_inventory(),'runs':[]}
    for path in identities:
        p=Path(path)
        if p.suffix=='.py':shutil.copy2(p,a.output/(p.stem+'.captured.py'))
    manifest=a.output/'manifest.json';save=lambda:manifest.write_text(json.dumps(report,indent=2)+'\n');save()
    try:
        for variant,case,mode,rep in tasks(a.repetitions):
            for key,files in frozen.items():
                if scene.frozen_identities(executables[key].parent)!=files:raise ValueError('Frozen phase runtime changed')
            key='normal' if mode=='normal' else 'profiled';folder=a.output/f'{variant}_{case}_{mode}_{rep}';folder.mkdir()
            command=scene.sample_command(executables[key],plugins[key],a,folder,{'view':'follow','seconds':a.seconds,'mode':'cpu'})
            env=dict(os.environ)
            for name in ('WP_MATERIAL_SCENE_CPU','WP_MATERIAL_SCENE_NATIVE_PHASES','WP_MATERIAL_SCENE_CUSTOM','WP_MATERIAL_SCENE_UPLOAD_MODE','WP_MATERIAL_SCENE_EDITS'):env.pop(name,None)
            env.update(WP_MATERIAL_SCENE_UPLOAD_MODE='combined',WP_MATERIAL_SCENE_EDITS=case)
            if variant=='custom':env['WP_MATERIAL_SCENE_CUSTOM']='1'
            active=mode in ('aggregate','phases')
            if active:env['WP_MATERIAL_SCENE_CPU']='1'
            if mode=='phases':env['WP_MATERIAL_SCENE_NATIVE_PHASES']='1'
            with (folder/'stdout.log').open('w') as out,(folder/'stderr.log').open('w') as err:
                process=subprocess.Popen(command,cwd=scene.ROOT,env=env,stdout=out,stderr=err)
                try:code=process.wait(timeout=600)
                except subprocess.TimeoutExpired:process.kill();process.wait();raise ValueError('Unchanged 600-second scene timeout exceeded')
            if code:raise ValueError(folder.name+' exited '+str(code))
            p=json.loads((folder/'profile.json').read_text());review=json.loads((folder/'render.json').read_text())
            scene.verify_profile_scope(p,review,plugins[key]);scene.validate(p,review,'cpu','follow',process.pid,a,a.seconds)
            attribution.edits(p,case,variant=='custom',active)
            material=attribution.material(p,variant=='custom','combined') if active else None
            if active and p['materialCpu']['protocol']!='material_scene_cpu_v4':raise ValueError('Aggregate ABI changed')
            if not active and p.get('materialCpu') is not None:raise ValueError('Inactive phase control recorded aggregate CPU')
            phaseRows=phases(p) if mode=='phases' else None
            if mode!='phases' and p.get('nativeMaterialPhases') is not None:raise ValueError('Phase-disabled control collected phases')
            if report['runs'] and (p['workload']!=report['runs'][0]['profile']['workload'] or review['device']!=report['runs'][0]['device']):raise ValueError('Scene geometry/inventory/device/target differs')
            report['runs'].append({'variant':variant,'edit_case':case,'mode':mode,'repetition':rep,'command':command,'profile':p,'device':review['device'],'material':material,'phases':phaseRows,'outputs':{str(path):cpu.digest(path) for path in folder.iterdir() if path.is_file()}})
            save();print('accepted '+folder.name,flush=True)
        if current_inputs(a.root)!=receipt['engine_and_sample_after'] or any(cpu.digest(Path(path))!=h for path,h in identities.items()) or scene.asset_inventory()!=report['asset_inventory']:raise ValueError('Phase source/tool/assets drift')
        for key,files in frozen.items():
            if scene.frozen_identities(executables[key].parent)!=files:raise ValueError('Final phase runtime drift')
        report.update(status='COMPLETE',summary=summarize(report['runs'],a.repetitions));save()
    except Exception as error:report.update(status='REJECTED',error=str(error));save();raise
if __name__=='__main__':main()

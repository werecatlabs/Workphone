"""Matched pre-change/current material CPU comparison in the actual racing scene.

Reuses the unchanged production validator and v4 material/edit guards. Owner
cycles include probe cost; this is not hardware/cache or frame certification.
"""
import argparse,itertools,json,math,os,shutil,statistics,subprocess
from pathlib import Path
import ProfileMaterialCpu as cpu
import ProfileProductionScene as scene
import ProfileMaterialSceneCpu as attribution

ALLOWED_CHANGES={
    'Engine/cpp/Source/Workphone/State/States/MaterialPassStateData.cpp',
    'Engine/cpp/Source/WPGraphics/MaterialProgramDX11.cpp',
    'Engine/cpp/Include/WPGraphics/MaterialProgramDX11.hpp',
    'Engine/cpp/Source/WPGraphics/ClawMaterialRenderCache.cpp'}


def source_changes(control,candidate):
    if set(control)!=set(candidate):raise ValueError('Source membership differs')
    changes={key:{'control':value,'candidate':candidate[key]} for key,value in control.items() if value!=candidate[key]}
    if not changes or set(changes)-ALLOWED_CHANGES:raise ValueError('Source changes exceed the reviewed publication/capture boundary')
    return changes


def tasks(repetitions):
    result=[]
    # Both cohorts execute identical normal/inactive unchanged/all-edit controls.
    for case,variant,mode in itertools.product(('none','all'),('custom','textured'),('normal','inactive')):
        result.extend((cohort,variant,mode,case,0) for cohort in ('control','candidate'))
    for rep in range(1,repetitions+1):
        cases=('none','single','all') if rep%2 else ('all','single','none')
        variants=('textured','custom') if rep%2 else ('custom','textured')
        for index,(case,variant) in enumerate(itertools.product(cases,variants)):
            cohorts=('control','candidate') if (rep+index)%2 else ('candidate','control')
            result.extend((cohort,variant,'active',case,rep) for cohort in cohorts)
    return result


def compare(runs,repetitions):
    expected=set(tasks(repetitions))
    actual={(r['cohort'],r['variant'],r['mode'],r['edit_case'],r['repetition']) for r in runs}
    if len(runs)!=len(expected) or actual!=expected:raise ValueError('Incomplete matched process coverage')
    results=[]
    for variant,case in itertools.product(('textured','custom'),('none','single','all')):
        pairs={kind:sorted((r for r in runs if r['cohort']==kind and r['variant']==variant and
               r['mode']=='active' and r['edit_case']==case),key=lambda r:r['repetition']) for kind in ('control','candidate')}
        medians={kind:{key:statistics.median(r['material'][key] for r in rows) for key in rows[0]['material'] if key!='raw_counters'}
                 for kind,rows in pairs.items()}
        reductions={key:1-medians['candidate'][key]/value if value else None for key,value in medians['control'].items() if 'cycles' in key}
        per_pair=[{'repetition':old['repetition'],'stage_reductions':{
            key:1-new['material'][key]/value if value else None for key,value in old['material'].items() if 'cycles' in key}}
            for old,new in zip(pairs['control'],pairs['candidate'])]
        results.append({'variant':variant,'edit_case':case,'samples_per_cohort':repetitions,
                        'medians':medians,'observed_reductions':reductions,'pairs':per_pair})
    return results


def current_inputs(root):
    return {**cpu.all_engine_sources(root),**{str(p.relative_to(root)):cpu.digest(p)
        for p in (root/'Samples/cpp/VehicleAdvanced').glob('*') if p.is_file()}}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('control-receipt','candidate-receipt','plugins','output'):ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--repetitions',type=int,default=3);ap.add_argument('--seconds',type=float,default=30)
    a=ap.parse_args()
    if a.output.exists() or not 1<=a.repetitions<=3 or not math.isfinite(a.seconds) or a.seconds<30:
        ap.error('Use fresh output, 1-3 repetitions and at least 30 seconds')
    a.root=a.root.resolve();a.seed=42;a.quality='high';a.resolution='1280x720';a.width=1280;a.height=720;a.smoke=False
    control=json.loads(a.control_receipt.read_text());candidate=json.loads(a.candidate_receipt.read_text())
    origin=Path(control['scene_receipt']);old=json.loads(origin.read_text())
    if cpu.digest(origin)!=control['scene_receipt_sha256'] or old['engine_and_sample_after']!=control['source_inputs']:
        raise ValueError('Frozen control provenance differs')
    for receipt in (old,candidate):
        if receipt.get('status')!='COMPLETE' or receipt.get('protocol')!='material_scene_cpu_v4' or not all(
                receipt.get(key) for key in ('source_unchanged','normal_renderer_restored','normal_native_library_restored')):
            raise ValueError('Incomplete v4 build/restoration provenance')
        if receipt['engine_and_sample_before']!=receipt['engine_and_sample_after']:raise ValueError('Build source drift')
    if current_inputs(a.root)!=candidate['engine_and_sample_after']:raise ValueError('Candidate source drift')
    changes=source_changes(control['source_inputs'],candidate['engine_and_sample_after'])
    if set(control['allowed_changes'])!=ALLOWED_CHANGES:raise ValueError('Control allowed-change provenance differs')
    for key in ('probe_sha256','native_bridge_sha256','native_source_sha256','native_overlay_sha256'):
        if old[key]!=candidate[key]:raise ValueError('Probe/native scope changed: '+key)
    cpu.matching_builds(old['build_provenance'],candidate['build_provenance'])
    a.output.mkdir(parents=True);executables={};plugins={};frozen={}
    for cohort,path,receipt in (('control',origin,old),('candidate',a.candidate_receipt,candidate)):
        for mode,field in (('normal','normal_runtime'),('profiled','profiled_runtime')):
            original=path.parent/('runtime_'+mode)/'SampleVehicleAdvanced.exe'
            if cpu.modules(original)!=receipt[field]:raise ValueError('Build runtime drift: '+cohort+'/'+mode)
            target=a.output/(cohort+'_'+mode);target.mkdir();key=cohort+'_'+mode
            executables[key],plugins[key],_,frozen[key]=scene.snapshot_runtime(original,a.plugins.resolve(),target,False)
    identities={str(p.resolve()):cpu.digest(p) for p in (a.control_receipt,a.candidate_receipt,origin,Path(__file__),
        Path(attribution.__file__),Path(scene.__file__),Path(cpu.__file__))}
    for module in (__file__,attribution.__file__,scene.__file__,cpu.__file__):shutil.copy2(module,a.output/(Path(module).stem+'.captured.py'))
    report={'status':'RUNNING','protocol':'material_scene_cpu_v4','scope':'Matched current-runtime owner-thread material CPU; follow view, combined uploads',
        'm6_complete':False,'hardware_cache_acceptance':False,'representative_frame_acceptance':False,'background_workloads_controlled':False,
        'repetitions':a.repetitions,'requested_seconds':a.seconds,'source_changes':changes,'candidate_source_inputs':candidate['engine_and_sample_after'],
        'build_and_tool_sha256':identities,'frozen_runtime':frozen,'asset_inventory':scene.asset_inventory(),'runs':[]}
    manifest=a.output/'manifest.json';save=lambda:manifest.write_text(json.dumps(report,indent=2)+'\n');save()
    try:
        for cohort,variant,mode,case,rep in tasks(a.repetitions):
            for key,files in frozen.items():
                if scene.frozen_identities(executables[key].parent)!=files:raise ValueError('Frozen runtime drift')
            key=cohort+('_normal' if mode=='normal' else '_profiled')
            name=f'{cohort}_{variant}_{mode}_{case}_{rep}';folder=a.output/name;folder.mkdir()
            command=scene.sample_command(executables[key],plugins[key],a,folder,{'view':'follow','seconds':a.seconds,'mode':'cpu'})
            env=dict(os.environ)
            for name in ('WP_MATERIAL_SCENE_CPU','WP_MATERIAL_SCENE_CUSTOM','WP_MATERIAL_SCENE_UPLOAD_MODE','WP_MATERIAL_SCENE_EDITS'):env.pop(name,None)
            env.update(WP_MATERIAL_SCENE_UPLOAD_MODE='combined',WP_MATERIAL_SCENE_EDITS=case)
            if variant=='custom':env['WP_MATERIAL_SCENE_CUSTOM']='1'
            if mode=='active':env['WP_MATERIAL_SCENE_CPU']='1'
            with (folder/'stdout.log').open('w') as out,(folder/'stderr.log').open('w') as err:
                process=subprocess.Popen(command,cwd=scene.ROOT,env=env,stdout=out,stderr=err)
                try:code=process.wait(timeout=600)
                except subprocess.TimeoutExpired:
                    process.kill();process.wait();raise ValueError(folder.name+' exceeded the unchanged 600-second timeout')
            if code:raise ValueError(folder.name+' exited '+str(code))
            p=json.loads((folder/'profile.json').read_text());review=json.loads((folder/'render.json').read_text())
            scene.verify_profile_scope(p,review,plugins[key]);scene.validate(p,review,'cpu','follow',process.pid,a,a.seconds)
            attribution.edits(p,case,variant=='custom',mode=='active')
            details=attribution.material(p,variant=='custom','combined') if mode=='active' else None
            if mode=='active' and p['materialCpu']['protocol']!='material_scene_cpu_v4':raise ValueError('CPU scope differs')
            if mode!='active' and p.get('materialCpu') is not None:raise ValueError('Inactive probe recorded CPU')
            if report['runs']:
                first=report['runs'][0]
                if p['workload']!=first['profile']['workload'] or review['device']!=first['device']:
                    raise ValueError('Matched scene geometry/inventory/device/target differs')
            report['runs'].append({'cohort':cohort,'variant':variant,'mode':mode,'edit_case':case,'repetition':rep,
                'command':command,'profile':p,'device':review['device'],'material':details,
                'outputs':{str(path):cpu.digest(path) for path in folder.iterdir() if path.is_file()}})
            save();print('accepted '+folder.name,flush=True)
        if current_inputs(a.root)!=candidate['engine_and_sample_after']:raise ValueError('Source drift during comparison')
        if any(cpu.digest(Path(path))!=h for path,h in identities.items()):raise ValueError('Build/tool provenance changed')
        if scene.asset_inventory()!=report['asset_inventory']:raise ValueError('Scene asset drift')
        for key,files in frozen.items():
            if scene.frozen_identities(executables[key].parent)!=files:raise ValueError('Final frozen runtime drift')
        report.update(comparisons=compare(report['runs'],a.repetitions),status='COMPLETE');save()
    except Exception as error:report.update(status='REJECTED',error=str(error));save();raise


if __name__=='__main__':main()

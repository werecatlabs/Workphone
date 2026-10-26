"""Current-runtime material CPU attribution in the real procedural racing scene.

Custom programs did not exist in the original baseline. No old/new percentage
is inferred here. The unchanged production-scene validator remains mandatory.
"""
import argparse,json,os,shutil,subprocess
from pathlib import Path
import ProfileMaterialCpu as cpu
import ProfileProductionScene as scene


def material(profile,custom):
    m=profile.get('materialCpu')
    if not isinstance(m,dict) or m.get('protocol')!='material_scene_cpu_v1' or m.get('clock')!='QueryThreadCycleTime':
        raise ValueError('Missing scene CPU attribution')
    if any(type(value) is not int or value<0 for key,value in m.items() if key not in ('protocol','clock')):
        raise ValueError('Invalid scene CPU counters')
    if m['clockErrors'] or m['stride']!=32 or m['thread']!=profile['measurementThreadId'] or \
            not 0<m['sampledSections']<=m['visitedSections'] or not m['texturePaths'] or \
            m['clockOverheadMedian']<=0 or m['clockOverheadMax']<m['clockOverheadMedian']:
        raise ValueError('Incomplete textured scene owner-thread attribution')
    if custom and (not m['successfulBinds'] or m['bindCalls']!=m['successfulBinds'] or
                   m['bindCalls']!=m['unbindCalls'] or not m['bindingSampleCycles']):
        raise ValueError('Custom material activation/binding did not complete')
    if not custom and (m['bindCalls'] or m['unbindCalls']):raise ValueError('Builtin scene unexpectedly used custom programs')
    frames=profile['renderOwnerUpdatesMeasured'];stride=m['stride']
    raw=(m['preparationSampleCycles']+m['bindingSampleCycles'])*stride + m['publicationCommitCycles']+m['captureSealCycles']+m['recordClearCycles']
    segments=(m['preparationSampleSegments']+m['bindingSampleSegments'])*stride + m['publicationCommitSegments']+m['captureSealSegments']+m['recordClearSegments']
    return {'raw_preparation_cycles_per_frame':m['preparationSampleCycles']*stride/frames,
            'raw_custom_binding_cycles_per_frame':m['bindingSampleCycles']*stride/frames,
            'publication_commit_cycles_per_frame':m['publicationCommitCycles']/frames,
            'capture_seal_cycles_per_frame':m['captureSealCycles']/frames,
            'record_clear_cycles_per_frame':m['recordClearCycles']/frames,
            'visited_sections_per_frame':m['visitedSections']/frames,
            'successful_binds_per_frame':m['successfulBinds']/frames,
            'texture_paths_per_frame':m['texturePaths']/frames,
            'raw_combined_cycles_per_frame':raw/frames,
            'median_calibrated_combined_cycles_per_frame':max(0,raw-segments*m['clockOverheadMedian'])/frames,
            'maximum_calibration_subtracted_cycles_per_frame':max(0,raw-segments*m['clockOverheadMax'])/frames,
            'raw_counters':m}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--receipt',type=Path,required=True);ap.add_argument('--plugins',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--views',nargs='+',choices=('car','track','follow'),default=['car','track','follow'])
    ap.add_argument('--repetitions',type=int,default=3);ap.add_argument('--seconds',type=float,default=30)
    a=ap.parse_args()
    if a.output.exists() or not 1<=a.repetitions<=3 or a.seconds<30 or len(set(a.views))!=len(a.views):
        ap.error('Use fresh output, unique views, 1-3 repetitions and at least 30 seconds')
    a.seed=42;a.quality='high';a.resolution='1280x720';a.width=1280;a.height=720;a.smoke=False
    receipt=json.loads(a.receipt.read_text())
    if receipt.get('status')!='COMPLETE' or not receipt.get('source_unchanged') or not receipt.get('normal_renderer_restored'):
        ap.error('Incomplete/restoration-failed scene build')
    a.output.mkdir(parents=True);exe={};plugins={};frozen={}
    for mode,field,folder in [('normal','normal_runtime','runtime_normal'),('profiled','profiled_runtime','runtime_profiled')]:
        original=a.receipt.parent/folder/'SampleVehicleAdvanced.exe'
        if cpu.modules(original.resolve())!=receipt[field]:raise ValueError('Scene receipt runtime drift')
        destination=a.output/mode;destination.mkdir()
        exe[mode],plugins[mode],_,frozen[mode]=scene.snapshot_runtime(original.resolve(),a.plugins.resolve(),destination,False)
    report={'status':'RUNNING','protocol':'material_scene_cpu_v1','original_comparison':False,'m6_complete':False,
            'hardware_cache_acceptance':False,'background_workloads_controlled':False,
            'receipt_sha256':cpu.digest(a.receipt),'runner_sha256':cpu.digest(Path(__file__)),
            'validator_sha256':cpu.digest(Path(scene.__file__)),'frozen_runtime':frozen,'runs':[],
            'asset_inventory':scene.asset_inventory()}
    shutil.copy2(Path(__file__),a.output/'ProfileMaterialSceneCpu.captured.py')
    shutil.copy2(Path(scene.__file__),a.output/'ProfileProductionScene.captured.py')
    manifest=a.output/'manifest.json';save=lambda:manifest.write_text(json.dumps(report,indent=2)+'\n')
    save();tasks=[(variant,mode,'follow',0) for variant in ('custom','textured') for mode in ('normal','inactive')]
    tasks += [(variant,'active',view,rep+1) for rep in range(a.repetitions) for view in a.views for variant in
              (('textured','custom') if rep%2==0 else ('custom','textured'))]
    try:
        for variant,mode,view,rep in tasks:
            for key,files in frozen.items():
                if scene.frozen_identities(exe[key].parent)!=files:raise ValueError('Frozen scene runtime changed')
            key='normal' if mode=='normal' else 'profiled';folder=a.output/f'{variant}_{mode}_{view}_{rep}';folder.mkdir()
            command=scene.sample_command(exe[key],plugins[key],a,folder,{'view':view,'seconds':a.seconds,'mode':'cpu'})
            env=dict(os.environ)
            for name in ('WP_MATERIAL_SCENE_CPU','WP_MATERIAL_SCENE_CUSTOM'):env.pop(name,None)
            if variant=='custom':env['WP_MATERIAL_SCENE_CUSTOM']='1'
            if mode=='active':env['WP_MATERIAL_SCENE_CPU']='1'
            with (folder/'stdout.log').open('w') as out,(folder/'stderr.log').open('w') as err:
                process=subprocess.Popen(command,cwd=scene.ROOT,env=env,stdout=out,stderr=err)
                try:result=process.wait(timeout=600)
                except subprocess.TimeoutExpired:
                    process.kill();process.wait()
                    raise ValueError(folder.name+' exceeded the unchanged 600-second scene timeout')
            if result:raise ValueError(folder.name+' exited '+str(result))
            profile=json.loads((folder/'profile.json').read_text());review=json.loads((folder/'render.json').read_text())
            scene.verify_profile_scope(profile,review,plugins[key]);scene.validate(profile,review,'cpu',view,process.pid,a,a.seconds)
            details=material(profile,variant=='custom') if mode=='active' else None
            if mode!='active' and profile.get('materialCpu') is not None:raise ValueError('Inactive control recorded probe work')
            report['runs'].append({'variant':variant,'mode':mode,'view':view,'repetition':rep,'command':command,
                'profile':profile,'material':details,'outputs':{str(p):cpu.digest(p) for p in folder.iterdir() if p.is_file()}})
            save();print('accepted '+folder.name,flush=True)
        for key,files in frozen.items():
            if scene.frozen_identities(exe[key].parent)!=files:raise ValueError('Final scene runtime drift')
        if cpu.digest(a.receipt)!=report['receipt_sha256'] or cpu.digest(Path(__file__))!=report['runner_sha256'] or cpu.digest(Path(scene.__file__))!=report['validator_sha256']:
            raise ValueError('Scene runner/receipt/validator changed')
        if scene.asset_inventory()!=report['asset_inventory']:raise ValueError('Scene media assets changed')
        report['status']='COMPLETE';save()
    except Exception as error:
        report.update(status='REJECTED',error=str(error));save();raise

if __name__=='__main__':main()

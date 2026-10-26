"""Matched owner-thread CPU attribution in the actual instrumented adapters.

Warm, generated, untextured fixture only. Material selection/constant preparation
and owner release are sampled; publication, capture and retirement are complete.
Native draw/geometry/driver work is excluded, not subtracted from wall time.
"""
import argparse,json,os,shutil,statistics,subprocess
from pathlib import Path
import ProfileMaterialCpu as profile

FIELDS=('publication_cycles','capture_cycles','retirement_cycles','submit_owner_cycles',
        'material_sample_cycles','material_sample_segments','material_visited_sections',
        'material_sampled_sections','material_sampling_stride','clock_overhead_median','clock_overhead_max',
        'clock_errors','native_texture_paths')
MATCH_FIELDS=('materials','parameters','edits_percent','frames','sections','views','sample','samples',
              'checksum','draws','adapter_vendor','adapter_device','driver_version','feature_level',
              'driver_version_available','dedicated_video_memory')


def parse(path,kind,count,width,frames,samples,stride):
    rows=profile.parse(path,'baseline957' if kind=='baseline' else 'optimized',count,width,frames,samples)
    for row in rows:
        if row.get('attribution_protocol')!='material_cpu_scopes_v1' or row.get('thread_cycle_clock')!='QueryThreadCycleTime':
            raise ValueError('missing or incompatible material CPU scope evidence')
        if any(type(row.get(field)) is not int or row[field]<0 for field in FIELDS):
            raise ValueError('invalid material CPU counters')
        if row['clock_errors'] or row['native_texture_paths'] or row['material_sampling_stride']!=stride or \
                row['material_visited_sections']!=count*4*frames or \
                not 0<row['material_sampled_sections']<=row['material_visited_sections'] or \
                row['material_sample_segments']!=2*row['material_sampled_sections'] or \
                not 0<row['clock_overhead_median']<=row['clock_overhead_max']:
            raise ValueError('material CPU coverage, warm textureless path or clock guards failed')
    return rows


def attributed(row):
    stride=row['material_sampling_stride'];frames=row['frames']
    raw=row['publication_cycles']+row['capture_cycles']+row['retirement_cycles']+row['material_sample_cycles']*stride
    segments=3*frames+row['material_sample_segments']*stride
    return {'raw_cycles':raw/frames,
            'calibrated_cycles':max(0,raw-segments*row['clock_overhead_median'])/frames,
            'empirical_lower_cycles':max(0,raw-segments*row['clock_overhead_max'])/frames,
            'sampled_preparation_raw_cycles':row['material_sample_cycles']*stride/frames,
            'publication_raw_cycles':row['publication_cycles']/frames,
            'capture_raw_cycles':row['capture_cycles']/frames,
            'retirement_raw_cycles':row['retirement_cycles']/frames}


def comparisons(runs):
    groups={}
    for run in runs:
        if run['mode']!='active':continue
        for row in run['rows']:
            groups.setdefault((row['materials'],row['parameters'],row['edits_percent']),{'baseline':[],'optimized':[]})[run['kind']].append(row)
    result=[]
    for (count,width,edits),pair in sorted(groups.items()):
        old,new=pair['baseline'],pair['optimized']
        if not old or len(old)!=len(new):raise ValueError('unpaired material CPU attribution')
        for a,b in zip(old,new):
            for field in (*MATCH_FIELDS,'material_sampled_sections','material_sampling_stride'):
                if a[field]!=b[field]:raise ValueError('material CPU workload/device/sampling mismatch: '+field)
        before=[attributed(row) for row in old];after=[attributed(row) for row in new]
        med=lambda rows:{field:statistics.median(row[field] for row in rows) for field in rows[0]}
        b,a=med(before),med(after)
        conservative=1-a['raw_cycles']/b['empirical_lower_cycles'] if b['empirical_lower_cycles'] else None
        result.append({'materials':count,'parameters':width,'edits_percent':edits,'samples_per_implementation':len(old),
                       'baseline_cycles_per_frame':b,'optimized_cycles_per_frame':a,
                       'raw_material_cpu_reduction':1-a['raw_cycles']/b['raw_cycles'],
                       'calibrated_material_cpu_reduction':1-a['calibrated_cycles']/b['calibrated_cycles'] if b['calibrated_cycles'] else None,
                       'conservative_material_cpu_reduction':conservative,
                       'scoped_50_percent_observed':conservative is not None and conservative>=.5})
    return result


def load_receipt(path):
    receipt=json.loads(path.read_text())
    if receipt.get('status')!='COMPLETE' or not receipt.get('engine_source_unchanged') or not receipt.get('normal_renderer_restored'):
        raise ValueError('attribution build is incomplete or its normal renderer was not restored')
    for field in ('profiled_runtime','unprobed_runtime'):
        directory=path.parent/('runtime_profiled' if field=='profiled_runtime' else 'runtime_unprobed')
        exe=directory/'WPMaterialLoadSequenceTests.exe'
        if profile.modules(exe)!=receipt[field]:raise ValueError('attribution build runtime drift')
    return receipt


def controls(runs):
    result=[]
    for kind in ('baseline','optimized'):
        selected=[run for run in runs if run['kind']==kind and run['control']]
        if not selected:continue
        for edits in (0,1,100):
            values={};reference=None
            for mode in ('normal','inactive','active'):
                rows=[row for run in selected if run['mode']==mode for row in run['rows'] if row['edits_percent']==edits]
                if not rows:raise ValueError('incomplete probe overhead control')
                if reference is not None:
                    if len(rows)!=len(reference) or any(a[field]!=b[field] for a,b in zip(reference,rows) for field in MATCH_FIELDS):
                        raise ValueError('probe overhead control workload/device mismatch')
                else:reference=rows
                values[mode]=statistics.median(row['total_ns']/row['frames'] for row in rows)
            result.append({'kind':kind,'edits_percent':edits,'wall_medians_ns':values,
                           'active_over_normal_fraction':values['active']/values['normal']-1,
                           'active_over_inactive_fraction':values['active']/values['inactive']-1})
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--baseline-receipt',type=Path,required=True);ap.add_argument('--optimized-receipt',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--counts',nargs='+',type=int,default=[1000,10000])
    ap.add_argument('--width',type=int,default=32);ap.add_argument('--frames',type=int,default=1)
    ap.add_argument('--samples',type=int,default=3);ap.add_argument('--repetitions',type=int,default=3)
    ap.add_argument('--stride',type=int,default=32);ap.add_argument('--controls',action='store_true')
    a=ap.parse_args()
    if a.output.exists():ap.error('output exists; preserve prior evidence')
    if not a.counts or len(set(a.counts))!=len(a.counts) or any(n<100 or n>10000 for n in a.counts) or \
            a.width not in (8,32,128) or not 1<=a.frames<=300 or not 1<=a.samples<=31 or \
            not 1<=a.repetitions<=31 or not 1<=a.stride<=4096:ap.error('invalid attribution workload')
    receipts={kind:load_receipt(path.resolve()) for kind,path in
              (('baseline',a.baseline_receipt),('optimized',a.optimized_receipt))}
    profile.matching_builds(receipts['baseline']['build_provenance'],receipts['optimized']['build_provenance'])
    if receipts['baseline']['fixture_sha256']!=receipts['optimized']['fixture_sha256'] or \
            receipts['baseline']['probe_sha256']!=receipts['optimized']['probe_sha256']:
        ap.error('portable attribution fixture/probe differs')
    a.output.mkdir(parents=True);executables={};frozen={}
    report={'status':'RUNNING','protocol':'material_cpu_scopes_v1','scope':'warm generated textureless fixture; sampled material preparation and ownership cleanup plus complete owner-thread publication/capture/retirement',
            'timer':'QueryThreadCycleTime; cycle counts are not elapsed nanoseconds or PMU/cache counters',
            'material_sampling_estimator':'deterministic seeded Bernoulli selection, inverse-probability weight=stride',
            'counter_overhead':'empty query-pair median/max; conservative comparison uses raw current versus max-calibration-subtracted baseline',
            'build_receipts':{kind:str(path.resolve()) for kind,path in (('baseline',a.baseline_receipt),('optimized',a.optimized_receipt))},
            'build_receipt_sha256':{kind:profile.digest(path) for kind,path in (('baseline',a.baseline_receipt),('optimized',a.optimized_receipt))},
            'fixture_sha256':receipts['baseline']['fixture_sha256'],'probe_sha256':receipts['baseline']['probe_sha256'],
            'runner_sha256':profile.digest(Path(__file__)),'parser_sha256':profile.digest(Path(profile.__file__)),
            'hardware_cache_acceptance':False,'representative_frame_acceptance':False,'m6_complete':False,
            'runs':[],'background_workloads_controlled':False}
    # Preserve the exact runner/parser even if the checkout advances later.
    shutil.copy2(Path(__file__),a.output/'ProfileMaterialCpuAttribution.captured.py')
    shutil.copy2(Path(profile.__file__),a.output/'ProfileMaterialCpu.captured.py')
    manifest=a.output/'manifest.json'
    save=lambda:manifest.write_text(json.dumps(report,indent=2)+'\n')
    try:
        for kind,receipt in receipts.items():
            for mode,field in (('normal','unprobed_runtime'),('profiled','profiled_runtime')):
                source=Path(next(name for name in receipt[field] if Path(name).name=='WPMaterialLoadSequenceTests.exe'))
                exe,_,hashes=profile.freeze(source,a.output/('runtime_'+kind+'_'+mode))
                executables[kind,mode]=exe;frozen[kind,mode]=hashes
        report['frozen_runtime']={kind+'_'+mode:value for (kind,mode),value in frozen.items()};save()
        tasks=[]
        if a.controls:
            tasks.extend((kind,mode,1000,0,True) for kind in ('baseline','optimized') for mode in ('normal','inactive','active'))
        for repetition in range(a.repetitions):
            for count in a.counts:
                tasks.extend((kind,'active',count,repetition+1,False) for kind in
                             (('baseline','optimized') if repetition%2==0 else ('optimized','baseline')))
        for kind,mode,count,repetition,control in tasks:
            exe=executables[kind,'normal' if mode=='normal' else 'profiled']
            for key,value in frozen.items():
                if profile.modules(executables[key])!=value:raise ValueError('frozen attribution runtime changed')
            label=f'{kind}_{mode}_{count}x{a.width}_{repetition}'+('_control' if control else '')
            out=a.output/(label+'.stdout.log');err=a.output/(label+'.stderr.log')
            environment=dict(os.environ);environment['PATH']=str(exe.parent)+os.pathsep+environment.get('PATH','')
            environment.pop('WP_MATERIAL_CPU_ATTRIBUTION',None);environment.pop('WP_MATERIAL_CPU_SAMPLE_STRIDE',None)
            if mode=='active':environment.update(WP_MATERIAL_CPU_ATTRIBUTION='1',WP_MATERIAL_CPU_SAMPLE_STRIDE=str(a.stride))
            command=[str(exe),'--profile-material-cpu',str(count),str(a.width),str(a.frames),str(a.samples)]
            with out.open('w') as stdout,err.open('w') as stderr:
                result=subprocess.run(command,cwd=exe.parent,env=environment,stdout=stdout,stderr=stderr,timeout=1200)
            if result.returncode:raise ValueError(label+' failed: '+str(result.returncode))
            rows=parse(out,kind,count,a.width,a.frames,a.samples,a.stride) if mode=='active' else \
                 profile.parse(out,'baseline957' if kind=='baseline' else 'optimized',count,a.width,a.frames,a.samples)
            report['runs'].append({'kind':kind,'mode':mode,'control':control,'repetition':repetition,
                'rows':rows,'command':command,'stdout_sha256':profile.digest(out),'stderr_sha256':profile.digest(err)})
            save();print('accepted '+label,flush=True)
        # Controls validate instrumentation separately, not as extra treatment samples.
        report['comparisons']=comparisons([run for run in report['runs'] if not run['control']])
        report['overhead_controls']=controls(report['runs'])
        for key,value in frozen.items():
            if profile.modules(executables[key])!=value:raise ValueError('final frozen attribution runtime drift')
        if profile.digest(Path(__file__))!=report['runner_sha256'] or profile.digest(Path(profile.__file__))!=report['parser_sha256']:
            raise ValueError('attribution runner dependencies changed')
        for kind,path in (('baseline',a.baseline_receipt),('optimized',a.optimized_receipt)):
            if profile.digest(path)!=report['build_receipt_sha256'][kind]:raise ValueError('attribution build receipt changed')
        report['scoped_material_cpu_50_percent_observed']=all(row['scoped_50_percent_observed'] for row in report['comparisons'])
        report['status']='COMPLETE';save()
    except Exception as error:
        report.update(status='REJECTED',error=str(error));save();raise

if __name__=='__main__':main()

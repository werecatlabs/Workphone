"""Paired current-code control/candidate publication benchmark.

Uses the existing real-renderer CPU fixture and its draw/value/device guards.
The control receipt must precede the narrowly permitted implementation edits.
This is inclusive owner wall time, not hardware/cache or frame certification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import ProfileMaterialCpu as profile

ALLOWED_CHANGES = {'Engine/cpp/Source/Workphone/Graphics/Material.cpp',
                   'Tests/cpp/MaterialLoadSequenceTests.cpp'}

def compare(runs):
    groups = {}
    for run in runs:
        for row in run['rows']:
            groups.setdefault((row['materials'],row['parameters'],row['edits_percent']),
                              {'control':[], 'candidate':[]})[run['kind']].append(row)
    result=[]
    for (count,width,edits),pair in sorted(groups.items()):
        before,after=pair['control'],pair['candidate']
        if not before or len(before)!=len(after): raise ValueError('incomplete paired workload')
        for old,new in zip(before,after):
            for field in ('draws','checksum','adapter_vendor','adapter_device','feature_level',
                          'driver_version','driver_version_available','dedicated_video_memory'):
                if old[field]!=new[field]: raise ValueError('control/candidate mismatch: '+field)
        stages=('update_publication_ns','capture_prepare_ns','submit_ns','retirement_ns','total_ns')
        b={s:statistics.median(r[s]/r['frames'] for r in before) for s in stages}
        a={s:statistics.median(r[s]/r['frames'] for r in after) for s in stages}
        result.append({'materials':count,'parameters':width,'edits_percent':edits,
            'samples_per_variant':len(before),'control_median_ns_per_frame':b,
            'candidate_median_ns_per_frame':a,
            'publication_reduction':1-a['update_publication_ns']/b['update_publication_ns'] if b['update_publication_ns'] else None,
            'inclusive_reduction':1-a['total_ns']/b['total_ns']})
    return result

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for field in ('control-exe','candidate-exe','control-receipt','output'):
        ap.add_argument('--'+field,type=Path,required=True)
    ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--counts',nargs='+',type=int,default=[1000])
    ap.add_argument('--width',type=int,default=32)
    ap.add_argument('--frames',type=int,default=5)
    ap.add_argument('--samples',type=int,default=7)
    ap.add_argument('--repetitions',type=int,default=3)
    a=ap.parse_args()
    if a.output.exists(): ap.error('output exists; preserve previous evidence')
    if (not a.counts or len(set(a.counts))!=len(a.counts) or any(n<100 or n>10000 for n in a.counts)
        or a.width not in (8,32,128) or not 1<=a.frames<=300 or not 1<=a.samples<=31
        or not 1<=a.repetitions<=31): ap.error('invalid workload dimensions')
    a.root=a.root.resolve(); receipt=json.loads(a.control_receipt.read_text())
    current=profile.sources(a.root)
    extra_inputs={p:profile.digest(a.root/p) for p in
        ('Tests/cpp/MaterialSinglePassEditContracts.hpp','Tools/ProfileMaterialCpu.py')}
    changes={p:{'control':h,'candidate':current.get(p)} for p,h in receipt['sources'].items() if current.get(p)!=h}
    if not changes or set(changes)-ALLOWED_CHANGES: ap.error('control differs outside permitted implementation/test files: '+str(list(changes)))
    profile.matching_builds(receipt['build'],profile.build_provenance(a.root,a.root/'project_x64'))
    a.output.mkdir(parents=True)
    report={'status':'RUNNING','scope':'paired inclusive owner wall intervals; real DX11 submission',
        'background_workloads_controlled':False,'hardware_cache_acceptance':False,
        'material_only_attribution':False,'representative_frame_acceptance':False,
        'source_changes':changes,'source_inputs':current,'extra_source_inputs':extra_inputs,
        'control_receipt_sha256':profile.digest(a.control_receipt),
        'runner_sha256':profile.digest(Path(__file__)),'fixture_sha256':profile.digest(a.root/'Tests/cpp/MaterialCpuProfile.hpp'),'runs':[]}
    manifest=a.output/'manifest.json'
    def save(): manifest.write_text(json.dumps(report,indent=2)+'\n')
    try:
        executables={}; hashes={}
        for kind,exe in (('control',a.control_exe),('candidate',a.candidate_exe)):
            executables[kind],original,hashes[kind]=profile.freeze(exe.resolve(),a.output/('runtime_'+kind))
            report[kind+'_source_runtime']=original;report[kind+'_frozen_runtime']=hashes[kind]
        save()
        for repetition in range(a.repetitions):
            for count in a.counts:
                for kind in (('control','candidate') if repetition%2==0 else ('candidate','control')):
                    exe=executables[kind]
                    if profile.modules(exe)!=hashes[kind]: raise ValueError('frozen runtime drift')
                    label=f'{kind}_{count}x{a.width}_{repetition+1}'
                    stdout=a.output/(label+'.stdout.log');stderr=a.output/(label+'.stderr.log')
                    command=[str(exe),'--profile-material-cpu',str(count),str(a.width),str(a.frames),str(a.samples)]
                    environment=dict(os.environ);environment['PATH']=str(exe.parent)+os.pathsep+environment.get('PATH','')
                    with stdout.open('w') as out,stderr.open('w') as err:
                        run=subprocess.run(command,cwd=exe.parent,env=environment,stdout=out,stderr=err,timeout=1200)
                    if run.returncode: raise ValueError(label+' failed: '+str(run.returncode))
                    rows=profile.parse(stdout,'optimized',count,a.width,a.frames,a.samples)
                    report['runs'].append({'kind':kind,'name':label,'rows':rows,'command':command,
                        'stdout_sha256':profile.digest(stdout),'stderr_sha256':profile.digest(stderr)})
                    save();print('accepted '+label,flush=True)
        if current!=profile.sources(a.root): raise ValueError('required sources changed while measuring')
        if any(profile.digest(a.root/p)!=h for p,h in extra_inputs.items()): raise ValueError('test/runner dependencies changed')
        if any(profile.modules(executables[k])!=hashes[k] for k in executables): raise ValueError('frozen runtime drift')
        report['comparisons']=compare(report['runs']);report['status']='COMPLETE';save()
    except Exception as error:
        report['status']='REJECTED';report['error']=str(error);save();raise

if __name__=='__main__': main()

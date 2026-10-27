import copy,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'))
import ProfileMaterialNativePhases as phases
from MaterialSceneCpuPhaseOverlay import native_phase_overlay,canonical_phase_overlay
from MaterialSceneCpuAttributionOverlay import native_overlay
import test_material_scene_cpu as scene_tests

def profile():
    p=scene_tests.SceneGuards().edit_profile()
    m=p['materialCpu'];m.update(nativeBindingRegions=4400,nativeBindingSampleSegments=136,nativeBindingSampleCycles=1360)
    rows=[{'id':i+1,'name':name,'cycles':310 if i in (0,1,2,5) else 60,'segments':31 if i in (0,1,2,5) else 6,'regions':1000 if i in (0,1,2,5) else 200} for i,name in enumerate(phases.PHASE_NAMES)]
    p['nativeMaterialPhases']={'protocol':'material_native_phases_v1','clock':'QueryThreadCycleTime','thread':7,'stride':32,'draws':1000,'sampledDraws':31,'errors':0,'phases':rows}
    return p

class PhaseGuards(unittest.TestCase):
    def test_exact_partition_and_calibration(self):
        p=profile();rows=phases.phases(p)
        self.assertEqual(len(rows),6)
        self.assertEqual(sum(r['raw_cycles_per_frame'] for r in rows),p['materialCpu']['nativeBindingSampleCycles']*32/10)
        self.assertAlmostEqual(sum(r['raw_binding_share'] for r in rows),1)
        self.assertEqual(rows[0]['sample_cycles_per_region'],10)
        self.assertEqual(rows[0]['median_pair_tax_adjusted_cycles_per_frame'],(310-31*2)*32/10)
    def test_phase_partition_is_not_added_again_to_material_total(self):
        import ProfileMaterialSceneCpu as aggregate
        p=profile();before=aggregate.material(p,True,'combined')
        p.pop('nativeMaterialPhases')
        self.assertEqual(before,aggregate.material(p,True,'combined'))
    def test_summary_requires_all_observer_pairs_and_controls(self):
        detail=phases.phases(profile())
        rows=[{'variant':variant,'edit_case':case,'mode':mode,'repetition':rep,
               'material':{'raw_counters':{},'raw_native_binding_cycles_per_frame':100,'raw_combined_cycles_per_frame':300},
               'profile':{'renderOwnerUpdateMeanMs':100},'phases':detail} for variant,case,mode,rep in phases.tasks(3)]
        summary=phases.summarize(rows,3)
        self.assertEqual(len(summary),3)
        self.assertTrue(all(len(r['phase_medians'])==6 and len(r['observer_pairs'])==3 for r in summary))
        for change in ('missing','duplicate','wrong-mode'):
            broken=copy.deepcopy(rows)
            if change=='missing':broken.pop()
            elif change=='duplicate':broken[-1]=copy.deepcopy(broken[0])
            else:broken[-1]['mode']='unknown'
            with self.subTest(change=change),self.assertRaises(ValueError):phases.summarize(broken,3)
    def test_missing_wrong_identity_sampling_or_clock_rejects(self):
        for key,value in [('protocol','wrong'),('clock','qpc'),('thread',8),('stride',16),('draws',999),('sampledDraws',30),('errors',1),('draws',True)]:
            p=profile();p['nativeMaterialPhases'][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):phases.phases(p)
        p=profile();p.pop('nativeMaterialPhases')
        with self.assertRaises(ValueError):phases.phases(p)
    def test_partition_mismatch_or_invalid_phase_record_rejects(self):
        for key,value in [('cycles',311),('segments',30),('regions',999),('id',2),('name','other'),('cycles',-1),('regions',False)]:
            p=profile();p['nativeMaterialPhases']['phases'][0][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):phases.phases(p)
        for change in ('missing','duplicate','reordered','nondict'):
            p=profile();rows=p['nativeMaterialPhases']['phases']
            if change=='missing':rows.pop()
            elif change=='duplicate':rows[4]=copy.deepcopy(rows[3])
            elif change=='reordered':rows.reverse()
            else:rows[4]=None
            with self.subTest(change=change),self.assertRaises(ValueError):phases.phases(p)
    def test_conditional_skips_are_allowed_but_unconditional_missing_is_not(self):
        p=profile()
        for i in (3,4):p['nativeMaterialPhases']['phases'][i].update(cycles=0,segments=0,regions=0)
        p['materialCpu'].update(nativeBindingRegions=4000,nativeBindingSampleSegments=124,nativeBindingSampleCycles=1240)
        self.assertEqual(phases.phases(p)[3]['raw_cycles_per_frame'],0)
        p['nativeMaterialPhases']['phases'][3]['cycles']=1
        with self.assertRaises(ValueError):phases.phases(p)
    def test_overlay_only_changes_begin_event_payloads(self):
        root=Path(__file__).resolve().parents[2];raw=(root/'Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c').read_bytes();bridge=Path('C:/probe/bridge.h')
        tagged=native_phase_overlay(raw,bridge)
        self.assertEqual(canonical_phase_overlay(tagged),native_overlay(raw,bridge))
        for bad in (tagged.replace(b'BIND_BEGIN, 6',b'BIND_BEGIN, 7'),tagged.replace(b'BIND_BEGIN, 6',b'BIND_BEGIN, 5'),b'unknown'):
            with self.assertRaises(ValueError):canonical_phase_overlay(bad)
    def test_phase_probe_adds_no_clock_queries(self):
        root=Path(__file__).resolve().parents[2];raw=(root/'Samples/cpp/VehicleAdvanced/MaterialSceneCpuPhaseProbe.hpp').read_text()
        self.assertNotIn('clock()',raw);self.assertNotIn('QueryThreadCycleTime(',raw)
    def test_observer_control_coverage_and_alternating_modes(self):
        tasks=phases.tasks(3);self.assertEqual(len(tasks),24);self.assertEqual(len(set(tasks)),24)
        self.assertEqual(sum(t[2]=='phases' for t in tasks),9);self.assertEqual(sum(t[2]=='aggregate' for t in tasks),9)
        self.assertEqual(sum(t[2] in ('normal','inactive') for t in tasks),6)
        orders=set()
        for a,b in zip(tasks[6::2],tasks[7::2]):
            self.assertEqual((a[0],a[1],a[3]),(b[0],b[1],b[3]));orders.add((a[2],b[2]))
        self.assertEqual(len(orders),2)
        with self.assertRaises(ValueError):phases.summarize([],3)

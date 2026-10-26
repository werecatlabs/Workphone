"""Reject incomplete textured/custom-program attribution without starting a GPU."""
import copy,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'))
import ProfileMaterialSceneCpu as scene
from MaterialSceneCpuAttributionOverlay import overlay

def profile(custom=True):
    return {'renderOwnerUpdatesMeasured':10,'measurementThreadId':7,'materialCpu':dict(
        protocol='material_scene_cpu_v1',clock='QueryThreadCycleTime',stride=32,thread=7,
        preparationSampleCycles=100,preparationSampleSegments=20,bindingSampleCycles=30 if custom else 0,
        bindingSampleSegments=10 if custom else 0,publicationCommitCycles=12,captureSealCycles=14,recordClearCycles=15,
        publicationCommitSegments=10,captureSealSegments=10,recordClearSegments=10,
        visitedSections=1000,sampledSections=31,texturePaths=500,bindCalls=4 if custom else 0,
        successfulBinds=4 if custom else 0,unbindCalls=4 if custom else 0,
        clockOverheadMedian=2,clockOverheadMax=3,clockErrors=0)}

class SceneGuards(unittest.TestCase):
    def test_real_texture_thread_clock_and_sampling_are_required(self):
        self.assertEqual(scene.material(profile(),True)['raw_preparation_cycles_per_frame'],320)
        for field,value in [('texturePaths',0),('thread',8),('clockErrors',1),('sampledSections',0),
                            ('stride',1),('clockOverheadMedian',0),('clockOverheadMax',1),('clock','wall')]:
            p=profile();p['materialCpu'][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):scene.material(p,True)

    def test_custom_programs_must_bind_successfully_and_retire(self):
        for field,value in [('bindCalls',0),('successfulBinds',3),('unbindCalls',3),('bindingSampleCycles',0)]:
            p=profile();p['materialCpu'][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):scene.material(p,True)

    def test_builtin_scene_must_not_silently_activate_custom_programs(self):
        self.assertEqual(scene.material(profile(False),False)['raw_custom_binding_cycles_per_frame'],0)
        with self.assertRaises(ValueError):scene.material(profile(),False)

    def test_native_intervals_are_not_subtracted_to_infer_material_cpu(self):
        p=profile();before=scene.material(p,True);p['nativeFrameNs']=10**15
        self.assertEqual(before,scene.material(p,True))

    def test_bind_overlay_wraps_real_functions_and_success_path(self):
        source=('#include "pch.hpp"\n'
            '    bool MaterialProgramDX11::Publication::Bind(int x) const\n    {\n        return true;\n    }\n'
            '    void MaterialProgramDX11::Publication::Unbind(int x) const\n    { realUnbind(); }\n').encode()
        result=overlay('MaterialProgramDX11',source,Path('C:/probe.hpp')).decode()
        self.assertEqual(result.count('BindingScope wpBindingCpu'),2)
        self.assertEqual(result.count('successfulBind()'),1)
        self.assertEqual(result.count('realUnbind()'),1)

    def test_overlay_refuses_an_unknown_source_shape(self):
        with self.assertRaises((AssertionError,ValueError)):overlay('ClawHammerSystem',b'#include "pch"\nunknown',Path('probe.hpp'))

if __name__=='__main__':unittest.main()

"""Reject incomplete textured/custom-program attribution without starting a GPU."""
import copy,sys,unittest,subprocess,tempfile,re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'))
import ProfileMaterialSceneCpu as scene
from MaterialSceneCpuAttributionOverlay import overlay,native_overlay

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
    def native_profile(self):
        p=profile();p['materialCpu'].update(protocol='material_scene_cpu_v2',nativeUploadSampleCycles=1000,
            nativeUploadSampleSegments=28,nativeBindingSampleCycles=2000,nativeBindingSampleSegments=62,
            nativeDrawCalls=1000,nativeUnscopedDrawCalls=200,nativeSampledDraws=31,nativeFailedDraws=0,
            nativeCombinedDraws=1000,nativeSplitDraws=0,nativeUploadCalls=900,nativeAuxUploadCalls=300,
            nativeFailedUploads=0,nativeBindingRegions=2000)
        return p

    def test_native_upload_and_binding_are_additive_and_mode_checked(self):
        p=self.native_profile();r=scene.material(p,True,'combined')
        self.assertEqual(r['raw_native_upload_cycles_per_frame'],3200)
        self.assertEqual(r['raw_native_binding_cycles_per_frame'],6400)
        self.assertEqual(r['raw_combined_cycles_per_frame'],scene.material(profile(),True)['raw_combined_cycles_per_frame']+9600)
        with self.assertRaises(ValueError):scene.material(p,True,'split')
        with self.assertRaises(ValueError):scene.material(profile(),True,'combined')

    def test_native_failures_or_missing_coverage_reject(self):
        for key,value in [('nativeFailedDraws',1),('nativeFailedUploads',1),('nativeSampledDraws',0),
                          ('nativeUnscopedDrawCalls',1001),('nativeBindingRegions',1999),
                          ('nativeBindingSampleSegments',61),('nativeUploadSampleSegments',0),('nativeSplitDraws',1)]:
            p=self.native_profile();p['materialCpu'][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):scene.material(p,True,'combined')

    def test_v3_requires_eligibility_and_pipeline_state_regions(self):
        p=self.native_profile();p['materialCpu']['protocol']='material_scene_cpu_v3'
        with self.assertRaises(ValueError):scene.material(p,True,'combined')
        p['materialCpu'].update(nativeBindingRegions=4000,nativeBindingSampleSegments=124)
        self.assertGreater(scene.material(p,True,'combined')['raw_native_binding_cycles_per_frame'],0)

    def edit_profile(self,case='all',custom=True):
        p=self.native_profile();p['materialCpu'].update(protocol='material_scene_cpu_v4',
            nativeBindingRegions=4000,nativeBindingSampleSegments=124,
            authoringCycles=300,editPublicationCycles=400,editVerificationCycles=90000,
            authoringSegments=10,editPublicationSegments=20,editVerificationSegments=20)
        p['workload']={'uniqueMaterials':30};p['telemetry']={'materialPreparations':30}
        count=300 if case=='all' else 10
        p['materialEdits']=dict(mode=case,eligibleMaterials=30,customMaterials=10 if custom else 0,
            frames=10,changedFrames=10,materialsEdited=count,customTintEdits=100 if custom and case=='all' else 0,
            verifiedRevisions=count)
        return p

    def test_changed_frame_authoring_and_publication_exclude_verification(self):
        p=self.edit_profile();result=scene.material(p,True,'combined')
        scene.edits(p,'all',True,True)
        legacy=copy.deepcopy(p);legacy['materialCpu']['protocol']='material_scene_cpu_v3'
        self.assertEqual(result['raw_combined_cycles_per_frame'],scene.material(legacy,True,'combined')['raw_combined_cycles_per_frame']+70)
        self.assertEqual(result['observer_edit_verification_cycles_per_frame'],9000)
        p['materialCpu']['editVerificationCycles']*=100
        self.assertEqual(result['raw_combined_cycles_per_frame'],scene.material(p,True,'combined')['raw_combined_cycles_per_frame'])

    def test_missing_revisions_preparations_or_edit_intervals_reject(self):
        for field,value in [('verifiedRevisions',299),('materialsEdited',299),('frames',9),
                            ('changedFrames',9),('eligibleMaterials',29),('customTintEdits',99),('customMaterials',31)]:
            p=self.edit_profile();p['materialEdits'][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):scene.edits(p,'all',True,True)
        p=self.edit_profile();p['telemetry']['materialPreparations']=0
        with self.assertRaises(ValueError):scene.edits(p,'all',True,True)
        for field in ('authoringSegments','editPublicationSegments','editVerificationSegments','authoringCycles','editPublicationCycles','editVerificationCycles'):
            p=self.edit_profile();p['materialCpu'][field]=0
            with self.subTest(field=field),self.assertRaises(ValueError):scene.edits(p,'all',True,True)

    def test_unchanged_control_and_rotating_custom_edit_coverage(self):
        p=self.edit_profile('single');scene.edits(p,'single',True,True)
        p['renderOwnerUpdatesMeasured']=30;p['materialEdits'].update(frames=30,changedFrames=30,materialsEdited=30,verifiedRevisions=30)
        with self.assertRaises(ValueError):scene.edits(p,'single',True,False)
        p['materialEdits']['customTintEdits']=10;scene.edits(p,'single',True,False)
        p=self.edit_profile();p['materialEdits'].update(mode='none',changedFrames=0,materialsEdited=0,verifiedRevisions=0,customTintEdits=0)
        p['materialCpu'].update(authoringSegments=0,editPublicationSegments=0,editVerificationSegments=0,
                               authoringCycles=0,editPublicationCycles=0,editVerificationCycles=0)
        scene.edits(p,'none',True,True)
        p['materialEdits']['customTintEdits']=1
        with self.assertRaises(ValueError):scene.edits(p,'none',True,True)

    def test_nonfinite_duration_rejects_before_launch(self):
        tool=Path(scene.__file__)
        with tempfile.TemporaryDirectory() as folder:
            output=Path(folder)/'new'
            result=subprocess.run([sys.executable,'-B',str(tool),'--receipt','missing','--plugins','missing',
                '--output',str(output),'--seconds','nan'],capture_output=True,text=True)
            self.assertEqual(result.returncode,2);self.assertFalse(output.exists())

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

    def test_native_overlay_preserves_draws_and_excludes_ia_mvp_from_regions(self):
        root=Path(__file__).resolve().parents[2]
        original=(root/'Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c').read_bytes()
        generated=native_overlay(original,Path('C:/bridge.h')).decode()
        for call in ('ID3D11DeviceContext_DrawIndexed(', 'ID3D11DeviceContext_DrawIndexedInstanced('):
            self.assertEqual(generated.count(call),original.decode().count(call))
        body=generated[generated.index('static wp_s32 wp_renderer_dx11_bind_geometry_pntc_cpu_body('):]
        body=body[:body.index('\nstatic wp_s32 wp_renderer_dx11_bind_geometry_pntc(')]
        regions=body.split('wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN, 1);')[1:]
        self.assertEqual(len(regions),6)
        for region in regions:
            timed=re.split(r'wp_material_scene_native_emit\(WP_SCENE_NATIVE_BIND_END,[^)]*\);',region)[0]
            self.assertNotIn('IASet',timed);self.assertNotIn('upload_mvp',timed);self.assertNotIn('DrawIndexed',timed)
        start=original.index(b'static wp_s32 wp_renderer_dx11_bind_geometry_pntc(')
        changed=original[:start]+original[start:].replace(b'        ID3D11DeviceContext_PSSetShaderResources( r->context, 9,',
                                 b'        return 0;\n        ID3D11DeviceContext_PSSetShaderResources( r->context, 9,',1)
        with self.assertRaises(ValueError):native_overlay(changed,Path('bridge.h'))

if __name__=='__main__':unittest.main()

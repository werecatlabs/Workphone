import copy
import json
from pathlib import Path
import sys,tempfile,unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'))
import ProfileMaterialCpuAttribution as attribution
from MaterialCpuAttributionOverlay import instrument
from test_profile_material_cpu import rows


def attributed_rows(kind='optimized'):
    result=rows(kind)
    for row in result:
        row.update(attribution_protocol='material_cpu_scopes_v1',thread_cycle_clock='QueryThreadCycleTime',
            publication_cycles=1000000,capture_cycles=1000000,retirement_cycles=1000000,submit_owner_cycles=100000000,
            material_sample_cycles=1000000,material_sample_segments=1250,material_visited_sections=20000,
            material_sampled_sections=625,material_sampling_stride=32,clock_overhead_median=100,clock_overhead_max=200,
            clock_errors=0,native_texture_paths=0)
    return result


class AttributionGuards(unittest.TestCase):
    def parse(self,data,kind='optimized'):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'rows.log'
            path.write_text('\n'.join('material_cpu '+' '.join(f'{k}={v}' for k,v in row.items()) for row in data))
            return attribution.parse(path,kind,1000,32,5,2,32)

    def test_exact_coverage_and_cpu_clock_are_required(self):
        self.assertEqual(len(self.parse(attributed_rows())),6)
        for field,value in (('clock_errors',1),('native_texture_paths',1),('material_visited_sections',19999),
                            ('material_sample_segments',624),('material_sampled_sections',0),
                            ('material_sampling_stride',64),('clock_overhead_max',1),('thread_cycle_clock','rdtsc')):
            data=attributed_rows();data[0][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):self.parse(data)

    def test_total_includes_publication_capture_and_retirement_not_native_submit(self):
        data=attributed_rows()[0];before=attribution.attributed(data)
        data['submit_owner_cycles']*=100
        self.assertEqual(before,attribution.attributed(data))
        data['publication_cycles']+=1000
        self.assertEqual(attribution.attributed(data)['raw_cycles'],before['raw_cycles']+200)
        self.assertEqual(before['raw_cycles'],7000000)

    def test_native_output_and_sampling_parity_are_required(self):
        old=attributed_rows('baseline957');new=attributed_rows()
        for field in ('checksum','draws','adapter_device','material_sampled_sections'):
            changed=copy.deepcopy(new);changed[0][field]+=1
            with self.subTest(field=field),self.assertRaises(ValueError):
                attribution.comparisons([{'mode':'active','kind':'baseline','rows':old},
                                        {'mode':'active','kind':'optimized','rows':changed}])

    def test_conservative_comparison_does_not_subtract_current_probe_tax(self):
        old=attributed_rows('baseline957');new=attributed_rows()
        for row in new:
            for field in ('publication_cycles','capture_cycles','retirement_cycles','material_sample_cycles'):row[field]//=4
        result=attribution.comparisons([{'mode':'active','kind':'baseline','rows':old},
                                      {'mode':'active','kind':'optimized','rows':new}])
        self.assertEqual(len(result),3)
        self.assertTrue(all(row['scoped_50_percent_observed'] for row in result))
        self.assertLess(result[0]['conservative_material_cpu_reduction'],result[0]['raw_material_cpu_reduction'])

    def test_overhead_controls_require_the_same_workload_and_device(self):
        runs=[{'kind':'baseline','mode':mode,'control':True,'rows':attributed_rows('baseline957')}
              for mode in ('normal','inactive','active')]
        self.assertEqual(len(attribution.controls(runs)),3)
        for field in ('checksum','draws','adapter_device','sample'):
            changed=copy.deepcopy(runs);changed[-1]['rows'][0][field]+=1
            with self.subTest(field=field),self.assertRaises(ValueError):attribution.controls(changed)


class OverlayGuards(unittest.TestCase):
    def source(self,baseline):
        resolve='auto pMaterial = resolveMeshMaterial( mesh, submeshIndex );' if baseline else 'const auto record = records->get(*binding, submeshIndex);'
        return ('#include "WPGraphics/WPClawHammerPCH.hpp"\n'
            'u64 ClawRendererDX11::renderMeshBatch( int unused )\n{\n'
            '        for( s32 submeshIndex = 0; submeshIndex < drawCount; ++submeshIndex )\n        {\n'
            '            '+resolve+'\n'
            '            if( instances )\n            {\n                if( geometry && count > 0 && count % 3u == 0 )\n'
            '                { realNativeDraw(); }\n            }\n        }\n    return 0;\n}\n').encode()

    def test_both_real_adapter_shapes_get_scopes_around_native_draw(self):
        for baseline in (True,False):
            output=instrument(self.source(baseline),Path('C:/test/probe.hpp'),baseline).decode()
            self.assertLess(output.index('MaterialScope wpMaterialCpuScope'),output.index('wpMaterialCpuScope.stop()'))
            self.assertLess(output.index('wpMaterialCpuScope.stop()'),output.index('realNativeDraw()'))
            self.assertLess(output.index('realNativeDraw()'),output.index('wpMaterialCpuScope.resume()'))
            self.assertEqual(output.count('realNativeDraw()'),1)

    def test_explicit_texture_resolution_counts_once_before_both_paths(self):
        original=self.source(False).decode()
        helper=('            const auto resolveTexture = [&]( ITexture *texture ) -> void * {\n'
                '                if(auto view=texture->getNativeTextureView(dx11)) return view;\n'
                '                void *view=nullptr;\n'
                '                texture->getTextureFinal( &view );\n'
                '                return view;\n            };\n')
        draw='            if( instances )'
        source=original.replace(draw,helper+draw,1).encode()
        output=instrument(source,Path('probe.hpp'),False).decode()
        self.assertEqual(output.count('::material_cpu_attribution::texturePath();'),1)
        self.assertLess(output.index('texturePath();'),output.index('getNativeTextureView'))
        self.assertLess(output.index('texturePath();'),output.index('getTextureFinal'))
        self.assertLess(output.index('texturePath();'),output.index('wpMaterialCpuScope.stop()'))
        with self.assertRaises(ValueError):
            instrument(original.replace(draw,helper+helper+draw,1).encode(),Path('probe.hpp'),False)

    def test_unknown_or_already_instrumented_algorithm_rejects(self):
        with self.assertRaises(ValueError):instrument(b'unknown renderer',Path('probe.hpp'),True)
        value=instrument(self.source(True),Path('probe.hpp'),True)
        with self.assertRaises(ValueError):instrument(value,Path('probe.hpp'),True)


if __name__=='__main__':unittest.main()

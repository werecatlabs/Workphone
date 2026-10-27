"""Guard matched scene cohorts before reporting optimization percentages."""
import copy,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'))
import ProfileMaterialBulkBindingComparison as comparison


class MatchedSceneGuards(unittest.TestCase):
    def rows(self):
        return [{'cohort':cohort,'variant':variant,'mode':mode,'edit_case':case,'repetition':rep,
                 'material':{'raw_combined_cycles_per_frame':100 if cohort=='control' else 80,
                             'raw_edit_publication_cycles_per_frame':0 if case=='none' else (20 if cohort=='control' else 10),
                             'raw_counters':{}}}
                for cohort,variant,mode,case,rep in comparison.tasks(3)]

    def test_complete_alternating_pairs_and_controls(self):
        tasks=comparison.tasks(3);self.assertEqual(len(tasks),20);self.assertEqual(len(set(tasks)),20)
        self.assertEqual(sum(t[2]=='active' for t in tasks),12)
        orders=set()
        for first,second in zip(tasks[::2],tasks[1::2]):
            self.assertEqual(first[1:],second[1:]);self.assertNotEqual(first[0],second[0])
            if first[2]=='active':orders.add((first[0],second[0]))
        self.assertEqual(len(orders),2)

    def test_missing_duplicate_or_wrong_case_rejects(self):
        for change in ('missing','duplicate','wrong-case'):
            rows=self.rows()
            if change=='missing':rows.pop()
            elif change=='duplicate':rows[-1]=copy.deepcopy(rows[0])
            else:rows[-1]['edit_case']='other'
            with self.subTest(change=change),self.assertRaises(ValueError):comparison.compare(rows,3)

    def test_paired_stage_medians_and_zero_work(self):
        result=comparison.compare(self.rows(),3);self.assertEqual(len(result),2)
        for row in result:
            self.assertAlmostEqual(row['observed_reductions']['raw_combined_cycles_per_frame'],.2)
            expected=None if row['edit_case']=='none' else .5
            self.assertEqual(row['observed_reductions']['raw_edit_publication_cycles_per_frame'],expected)
            self.assertEqual(len(row['pairs']),3)

    def test_only_reviewed_engine_changes_permitted(self):
        path=next(iter(comparison.ALLOWED_CHANGES));before={path:'old','unrelated':'same'}
        after={path:'new','unrelated':'same'}
        self.assertEqual(set(comparison.source_changes(before,after)),{path})
        for wrong in (before,{path:'new','unrelated':'changed'},{path:'new'}):
            with self.assertRaises(ValueError):comparison.source_changes(before,wrong)

    def test_native_event_scope_must_remain_identical(self):
        old=b'wp_material_scene_native_emit(BEGIN, 1); real_old(); wp_material_scene_native_emit(END, result);'
        new=old.replace(b'real_old()',b'real_new()')
        self.assertEqual(comparison.native_events(old),comparison.native_events(new))
        self.assertNotEqual(comparison.native_events(old),comparison.native_events(new.replace(b'END',b'BEGIN')))

    def test_native_scope_normalizes_only_one_exact_bridge_include(self):
        old=Path('G:/old/bridge.h');new=Path('C:/new/bridge.h')
        raw=lambda path:('#include "'+path.as_posix()+'"\nreal_native_code();\n').encode()
        self.assertEqual(comparison.canonical_native_overlay(raw(old),old),comparison.canonical_native_overlay(raw(new),new))
        self.assertNotEqual(comparison.canonical_native_overlay(raw(old),old),comparison.canonical_native_overlay(raw(new)+b'extra_probe();',new))
        for invalid in (b'no include',raw(new)+raw(new)):
            with self.assertRaises(ValueError):comparison.canonical_native_overlay(invalid,new)


if __name__=='__main__':unittest.main()

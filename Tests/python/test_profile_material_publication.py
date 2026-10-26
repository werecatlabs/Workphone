"""Guard paired publication reports against mixed workloads and misleading totals."""
import importlib.util
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'))
import ProfileMaterialPublication as tool

def row():
    return dict(materials=1000,parameters=32,edits_percent=100,frames=5,
        draws=20000,checksum=750,adapter_vendor=5140,adapter_device=140,feature_level=45056,
        driver_version=1,driver_version_available=1,dedicated_video_memory=0,
        update_publication_ns=100,capture_prepare_ns=100,submit_ns=700,retirement_ns=100,total_ns=1000)

class PublicationComparisonGuards(unittest.TestCase):
    def test_reports_publication_and_total_separately(self):
        before=row(); after=row();after.update(update_publication_ns=50,submit_ns=800,total_ns=1050)
        result=tool.compare([{'kind':'control','rows':[before]},{'kind':'candidate','rows':[after]}])[0]
        self.assertAlmostEqual(result['publication_reduction'],.5)
        self.assertAlmostEqual(result['inclusive_reduction'],-.05)
    def test_missing_variant_rejected(self):
        with self.assertRaises(ValueError): tool.compare([{'kind':'control','rows':[row()]}])
    def test_mismatched_values_or_device_rejected(self):
        for field in ('draws','checksum','adapter_device','driver_version','feature_level'):
            after=row();after[field]+=1
            with self.subTest(field=field),self.assertRaises(ValueError):
                tool.compare([{'kind':'control','rows':[row()]},{'kind':'candidate','rows':[after]}])

if __name__=='__main__': unittest.main()

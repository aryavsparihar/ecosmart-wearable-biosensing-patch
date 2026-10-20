import importlib.util
import sys
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "serial_plotter", Path(__file__).resolve().parents[2] / "tools" / "serial_plotter.py")
sp = importlib.util.module_from_spec(spec)
sys.modules["serial_plotter"] = sp  # dataclasses look the module up by name
spec.loader.exec_module(sp)


def test_parses_firmware_rows():
    row = sp.parse_line("12000,5.61,8.02,34.21,MONITOR,12,1\r\n")
    assert row.t == 12.0 and row.ph == 5.61 and row.cond == 8.02
    assert row.temp == 34.21 and row.phase == "MONITOR" and row.alert == 1


def test_skips_comments_headers_and_noise():
    for line in ("# window=3 phase=CALIB alert=0", "ms,ph,cond_mScm,temp_C,phase,p_anomaly,alert",
                 "", "garbage", "1,2,3"):
        assert sp.parse_line(line) is None


def test_saved_log_renders(tmp_path):
    log = tmp_path / "log.csv"
    lines = ["# banner", "ms,ph,cond_mScm,temp_C,phase,p_anomaly,alert"]
    for i in range(120):
        phase = "CALIB" if i < 40 else "MONITOR"
        alert = 1 if 80 <= i < 100 else 0
        lines.append(f"{i * 1000},5.6{i % 10},8.50,34.20,{phase},{90 if alert else 0},{alert}")
    log.write_text("\n".join(lines))
    out = tmp_path / "plot.png"
    argv = sys.argv
    sys.argv = ["serial_plotter.py", "--file", str(log), "--save", str(out)]
    try:
        assert sp.main() == 0
    finally:
        sys.argv = argv
    assert out.stat().st_size > 10_000

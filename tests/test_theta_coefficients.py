"""The theta coefficient table must have a live provenance chain.

Through rev 5 the committed table was correct but its documented provenance was
not: the discovery tool, run verbatim, returned different constants while
printing a cross-check that looked like success (readiness review finding A4).
The tool is now a confirmer against the closed form of MATHS.md D1a, and this
wrapper is what makes it run on every push rather than when someone remembers.
"""

import subprocess
import sys
from pathlib import Path

TOOL = Path(__file__).resolve().parents[1] / "tools" / "discover_theta_coefficients.py"


def test_closed_form_reproduces_the_production_table():
    r = subprocess.run([sys.executable, str(TOOL)], capture_output=True,
                       text=True)
    assert r.returncode == 0, r.stdout + r.stderr
    assert "SELFTEST OK" in r.stdout
    # The confirmer must actually have compared something.
    assert r.stdout.count("OK") >= 13

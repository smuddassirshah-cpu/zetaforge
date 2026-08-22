from tools import cli


def test_cli_version_exits_zero(capsys):
    assert cli.main(["--version"]) == 0
    assert capsys.readouterr().out.strip() == "zetaforge-ops 0.1.0"


def test_bare_invocation_is_rejected():
    assert cli.main([]) == 2


def test_version_constant_shape():
    assert cli.VERSION.startswith("zetaforge-ops ")

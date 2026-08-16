#!/usr/bin/python3

import subprocess
from dataclasses import dataclass

import pytest


@dataclass
class ExprCase:  # renamed from Testcase — pytest auto-collects classes named "Test*"
    input_str: str
    output_str: str
    return_code: int


CASES = [
    ExprCase("42", "42", 0),
    ExprCase("12 + 300 - 5", "307", 0),
    ExprCase("1 2 3", "", 234),
    ExprCase("((1 + 2) * 3)", "9", 0),
    ExprCase("100 % 7 ^ 2 / 3", "0", 0),
    ExprCase("-5 + 3", "-2", 0),
    ExprCase("12 + +3", "15", 0),
    ExprCase("__2", "", 234),
    ExprCase("\n", "", 234),
    ExprCase("", "", 234),
    ExprCase("2^3^2", "512", 0),
    ExprCase("(2^3)^2", "64", 0),
    ExprCase("-2^2", "-4", 0),
    ExprCase(
        "2^-2", "", 234
    ),  # rejected: negative exponent, consistent with existing eval() restriction),
    ExprCase("-(2+3)-3^2+(-3)^3", "-41", 0),
    ExprCase("----------41", "41", 0),
    ExprCase("invalid_input", "", 234),
    ExprCase("(((((((((((2+2*(3333))))))))))))", "6668", 0),
    ExprCase("(1 + 2", "", 234),
    ExprCase("1 + 2)", "", 234),
    ExprCase("((1 + 2)", "", 234),
    ExprCase("- - 5", "5", 0),
    ExprCase("1 + - - 2", "3", 0),
    ExprCase("3 + -4 * 2", "-5", 0),
    ExprCase("1 * / 2", "", 234),
    ExprCase("+", "", 234),
    ExprCase("10 / 0", "", 234),
    ExprCase(
        "((((((((((((((((((((((((((((((((((((xd))))))))))))))))))))))))))))))))))))",
        "",
        234,
    ),
]


@pytest.fixture(scope="session")
def shuntingyard_binary(tmp_path_factory):
    """Compile shuntingyard.c once for the whole test session."""
    binary_path = tmp_path_factory.mktemp("build") / "shuntingyard_c"
    subprocess.run(
        [
            "gcc",
            "-Wall",
            "-Wextra",
            "-fsanitize=address,undefined",
            "./shuntingyard.c",
            "-o",
            str(binary_path),
        ],
        capture_output=True,
        check=True,
    )
    return str(binary_path)


@pytest.mark.parametrize(
    "case",
    CASES,
    ids=[repr(c.input_str) for c in CASES],  # readable test names in pytest output
)
def test_shuntingyard(shuntingyard_binary, case):
    result = subprocess.run(
        [shuntingyard_binary],
        input=case.input_str,
        text=True,
        capture_output=True,
    )
    assert result.returncode == case.return_code
    if result.returncode == 0:
        assert case.output_str + "\n" == result.stdout

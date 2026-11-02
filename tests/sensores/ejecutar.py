"""Compila y ejecuta Sensores.cpp con hardware simulado, sin tocar la placa."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--cxx", default=shutil.which("g++"), help="Ruta a g++")
args = parser.parse_args()
if not args.cxx:
    parser.error("Indicá --cxx con la ruta a g++ o agregalo al PATH")

# MinGW tambien necesita encontrar las DLL de sus subprogramas de compilacion.
entorno = os.environ.copy()
entorno["PATH"] = str(Path(args.cxx).resolve().parent) + os.pathsep + entorno.get("PATH", "")

pruebas = Path(__file__).resolve().parent
repo = pruebas.parents[1]
with tempfile.TemporaryDirectory(prefix="ta-sensores-") as temporal:
    ejecutable = Path(temporal) / "test_sensores.exe"
    subprocess.run(
        [args.cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror",
         "-static-libgcc", "-static-libstdc++",
         "-I", str(pruebas / "fakes"), "-I", str(repo),
         str(repo / "Sensores.cpp"), str(pruebas / "test_sensores.cpp"),
         "-o", str(ejecutable)],
        check=True, env=entorno,
    )
    subprocess.run([str(ejecutable)], check=True, env=entorno)

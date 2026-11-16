"""Generate an isolated certificate and exercise the native TLS socket pair."""

import pathlib
import subprocess
import sys
import tempfile

binary, openssl, case = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="mantids30-sockets-tls-") as directory:
    certificate = pathlib.Path(directory) / "certificate.pem"
    key = pathlib.Path(directory) / "key.pem"
    subprocess.run(
        [openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
         "-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost",
         "-keyout", str(key), "-out", str(certificate)],
        check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        timeout=10,
    )
    subprocess.run([binary, "tls", case, str(certificate), str(key)], check=True, timeout=12)

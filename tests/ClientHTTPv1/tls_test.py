"""Exercise HTTPS trust and hostname verification using a temporary local CA."""

import pathlib
import socket
import ssl
import subprocess
import sys
import tempfile
import threading


def run_case(binary, certificate, key, host, trusted, expected):
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certificate, key)
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(10)
        port = listener.getsockname()[1]
        errors = []

        def serve():
            try:
                connection, _ = listener.accept()
                with connection:
                    connection.settimeout(5)
                    with context.wrap_socket(connection, server_side=True) as secure:
                        request = b""
                        while b"\r\n\r\n" not in request:
                            part = secure.recv(4096)
                            if not part:
                                return
                            request += part
                        secure.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello")
            except (ssl.SSLError, ConnectionError):
                if expected in ("success", "injected"):
                    errors.append("TLS server connection failed")
            except Exception as error:
                errors.append(str(error))

        worker = threading.Thread(target=serve)
        worker.start()
        try:
            subprocess.run(
                [binary, "--tls", f"https://{host}:{port}/", str(certificate) if trusted else "-", expected],
                check=True,
                timeout=15,
            )
        finally:
            worker.join(timeout=10)
        if worker.is_alive() or errors:
            raise RuntimeError(f"TLS fixture failed: {errors}")


def main():
    binary, openssl = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="mantids30-httpv1-") as directory:
        certificate = pathlib.Path(directory) / "certificate.pem"
        key = pathlib.Path(directory) / "key.pem"
        subprocess.run(
            [openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
             "-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost",
             "-keyout", str(key), "-out", str(certificate)],
            check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        run_case(binary, certificate, key, "localhost", True, "success")
        run_case(binary, certificate, key, "localhost", True, "injected")
        run_case(binary, certificate, key, "localhost", False, "failure")
        run_case(binary, certificate, key, "127.0.0.1", True, "failure")
    print("HTTPS trust and hostname tests passed")


if __name__ == "__main__":
    main()

import concurrent.futures
import os
import socket
import struct
import subprocess
import sys
import time
import tempfile

server, client, backend, library_client = sys.argv[1:]
environment = dict(os.environ)
if backend == 'poll':
    environment['REACTORNET_USE_POLL'] = '1'
else:
    environment.pop('REACTORNET_USE_POLL', None)
    environment.pop('MUDUO_USE_POLL', None)

def exercise(host, ipv6=False):
    family = socket.AF_INET6 if ipv6 else socket.AF_INET
    with socket.socket(family) as probe:
        probe.bind((host, 0))
        port = probe.getsockname()[1]
    logs = tempfile.TemporaryFile(mode='w+t')
    process = subprocess.Popen([server, str(port), '2', host], env=environment,
                               stdout=logs, stderr=subprocess.STDOUT, text=True)
    try:
        deadline = time.monotonic() + 5
        while True:
            try:
                with socket.create_connection((host, port), timeout=0.2):
                    break
            except OSError:
                if process.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError('echo server did not start before the deadline')
                time.sleep(0.02)

        def roundtrip(index):
            with socket.create_connection((host, port), timeout=5) as connection:
                payload = bytes((i + index) % 251 for i in range(2097152 if index == 0 else 131071))
                connection.sendall(payload[:17])
                connection.sendall(payload[17:])
                # Half-close still has to deliver the complete echo.
                connection.shutdown(socket.SHUT_WR)
                received = bytearray()
                while len(received) < len(payload):
                    chunk = connection.recv(8191)
                    if not chunk:
                        raise AssertionError('echo truncated after half-close')
                    received.extend(chunk)
                assert bytes(received) == payload
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
            list(executor.map(roundtrip, range(24)))
        subprocess.run([library_client, host, str(port)], env=environment, check=True, timeout=10)
        for _ in range(30):
            with socket.create_connection((host, port), timeout=2) as connection:
                connection.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
                connection.sendall(b'reset')
        result = subprocess.run([client, host, str(port), '8', '4096', '0.25', '2', '0'],
                                capture_output=True, text=True, timeout=10)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        assert 'errors=0' in result.stdout
        assert process.poll() is None
        print(backend, host, result.stdout.strip())
    finally:
        process.terminate()
        process.wait(timeout=5)
        logs.seek(0)
        output = logs.read()
        logs.close()
        if process.returncode not in (0, -15):
            raise AssertionError(output)

exercise('127.0.0.1')
exercise('::1', True)

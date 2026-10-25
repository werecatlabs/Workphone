"""Exercise the packaged .NET Framework stdio adapter as an external process."""
import argparse
import concurrent.futures
import json
from pathlib import Path
import socket
import subprocess


def frame(value):
    payload = json.dumps(value, ensure_ascii=False).encode("utf-8")
    return f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii") + payload


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("adapter", type=Path)
    args = parser.parse_args()
    adapter = args.adapter.resolve(strict=True)
    request = frame({"seq": 1, "type": "request", "command": "evaluate",
                     "arguments": {"expression": "actor.position", "label": "Lua π 🚦"}})
    response = frame({"seq": 1, "type": "response", "request_seq": 1,
                      "command": "evaluate", "success": True,
                      "body": {"result": "π 🚦", "variablesReference": 0}})
    with socket.socket() as listener, concurrent.futures.ThreadPoolExecutor() as workers:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(10)

        def serve():
            with listener.accept()[0] as client:
                client.settimeout(10)
                received = bytearray()
                while len(received) < len(request):
                    chunk = client.recv(7)
                    if not chunk:
                        raise AssertionError("Adapter closed before forwarding the request")
                    received.extend(chunk)
                assert received == request, "Adapter changed DAP framing or UTF-8"
                for byte in response:
                    client.sendall(bytes([byte]))
                # Socket EOF must end the adapter even with stdin still open.

        server = workers.submit(serve)
        process = subprocess.Popen([str(adapter), str(listener.getsockname()[1])],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        try:
            for start in range(0, len(request), 3):
                process.stdin.write(request[start:start + 3])
                process.stdin.flush()
            output = workers.submit(process.stdout.read)
            assert output.result(timeout=15) == response, "Adapter polluted or truncated protocol stdout"
            server.result(timeout=15)
            assert process.wait(timeout=5) == 0, process.stderr.read().decode()
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            for stream in (process.stdin, process.stdout, process.stderr):
                stream.close()
    invalid = subprocess.run([str(adapter), "0"], capture_output=True, timeout=10)
    assert invalid.returncode == 2 and not invalid.stdout, "Invalid arguments reached protocol stdout"
    print("Lua DAP stdio adapter: PASS")


if __name__ == "__main__":
    main()

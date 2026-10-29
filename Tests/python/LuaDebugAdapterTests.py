"""Exercise the packaged .NET Framework stdio adapter as an external process."""
import argparse
import concurrent.futures
import json
from pathlib import Path
import socket
import subprocess
import threading


def frame(value):
    payload = json.dumps(value, ensure_ascii=False).encode("utf-8")
    return f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii") + payload


def messages(packet):
    result = []
    while packet:
        header, separator, body = packet.partition(b"\r\n\r\n")
        assert separator, "Truncated adapter DAP header"
        fields = dict(line.split(b":", 1) for line in header.split(b"\r\n"))
        size = int(fields[b"Content-Length"])
        assert len(body) >= size, "Truncated adapter DAP body"
        result.append(json.loads(body[:size]))
        packet = body[size:]
    return result


def relay_case(adapter, request, engine_output):
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
                for byte in engine_output:
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
            received = output.result(timeout=15)
            server.result(timeout=15)
            status = process.wait(timeout=5)
            return status, received, process.stderr.read().decode()
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            for stream in (process.stdin, process.stdout, process.stderr):
                stream.close()


def ide_disconnect_case(adapter):
    accepted = threading.Event()
    with socket.socket() as listener, concurrent.futures.ThreadPoolExecutor() as workers:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(10)

        def serve():
            with listener.accept()[0] as client:
                client.settimeout(10)
                accepted.set()
                assert client.recv(1) == b"", "IDE EOF did not disconnect the engine"

        server = workers.submit(serve)
        process = subprocess.Popen([str(adapter), str(listener.getsockname()[1])],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        try:
            assert accepted.wait(10), "Adapter did not connect before IDE disconnect"
            process.stdin.close()
            output = workers.submit(process.stdout.read)
            assert process.wait(timeout=10) == 0, process.stderr.read().decode()
            assert output.result(timeout=5) == b"", "IDE EOF generated an unsolicited termination event"
            server.result(timeout=5)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            for stream in (process.stdin, process.stdout, process.stderr):
                stream.close()


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
    progress = frame({"seq": 7, "type": "event", "event": "output",
                      "body": {"output": "Editor closing π 🚦"}})
    status, output, error = relay_case(adapter, request, response + progress)
    assert status == 0 and not error, error
    assert output.startswith(response + progress), "Adapter changed engine DAP frames or UTF-8"
    assert messages(output)[-1] == {"seq": 8, "type": "event", "event": "terminated"}, \
        "Engine EOF did not send a correctly sequenced termination event"
    assert len(messages(output)) == 3, "Engine EOF generated duplicate termination events"

    terminated = frame({"seq": 2, "type": "event", "event": "terminated"})
    status, output, error = relay_case(adapter, request, response + terminated)
    assert status == 0 and not error, error
    assert output == response + terminated, "Adapter duplicated or changed engine termination"

    status, output, error = relay_case(adapter, request, response + b"Content-Length: 20\r\n\r\n{")
    assert status == 1 and "closed during a DAP message" in error, "Truncated engine frames became clean EOF"
    assert output == response, "Truncated frame or synthetic termination polluted protocol stdout"
    ide_disconnect_case(adapter)

    with socket.socket() as unavailable:
        unavailable.bind(("127.0.0.1", 0))
        port = unavailable.getsockname()[1]
        # A bound, non-listening endpoint deterministically refuses connection.
        refused = subprocess.run([str(adapter), str(port)], capture_output=True, timeout=15)
    assert refused.returncode == 1 and not refused.stdout and refused.stderr, "Connection errors became successful termination"
    invalid = subprocess.run([str(adapter), "0"], capture_output=True, timeout=10)
    assert invalid.returncode == 2 and not invalid.stdout, "Invalid arguments reached protocol stdout"
    print("Lua DAP stdio adapter: PASS")


if __name__ == "__main__":
    main()

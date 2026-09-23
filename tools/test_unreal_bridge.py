import json
import sys
from urllib.error import URLError, HTTPError
from urllib.request import Request, urlopen

BASE = "http://127.0.0.1:8000/api/unreal"


def request(path, method="GET", payload=None):
    data = None if payload is None else json.dumps(payload).encode("utf-8")
    req = Request(
        BASE + path,
        data=data,
        headers={
            "Accept": "application/json",
            "Content-Type": "application/json",
            "User-Agent": "Venky-Agent-Test",
        },
        method=method,
    )
    with urlopen(req, timeout=15) as resp:
        return json.loads(resp.read().decode("utf-8"))


try:
    print("1) Checking Unreal bridge...")
    health = request("/health")
    print(json.dumps(health, indent=2))

    print("\\n2) Creating test cube...")
    created = request(
        "/command",
        "POST",
        {
            "action": "create_actor",
            "primitive": "Cube",
            "name": "VenkyBridgeTestCube",
            "location": [0, 0, 100],
        },
    )
    print(json.dumps(created, indent=2))

    print("\\n3) Listing actors...")
    actors = request("/command", "POST", {"action": "list_actors"})
    print(json.dumps(actors, indent=2))

    print("\\n4) Moving test cube...")
    moved = request(
        "/command",
        "POST",
        {
            "action": "move_actor",
            "name": "VenkyBridgeTestCube",
            "location": [300, 0, 100],
        },
    )
    print(json.dumps(moved, indent=2))

    print("\\n5) Deleting test cube...")
    deleted = request(
        "/command",
        "POST",
        {"action": "delete_actor", "name": "VenkyBridgeTestCube"},
    )
    print(json.dumps(deleted, indent=2))

    print("\\n✅ Unreal bridge test sequence finished.")
except HTTPError as exc:
    detail = exc.read().decode("utf-8", errors="replace")
    print(f"❌ HTTP {exc.code}: {detail}")
    sys.exit(1)
except URLError as exc:
    print("❌ Could not reach Venky Agent. Start server.py first.")
    print(exc)
    sys.exit(1)
except Exception as exc:
    print("❌ Test failed:", exc)
    sys.exit(1)

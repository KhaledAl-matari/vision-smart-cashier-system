import json
import os
import subprocess
from pathlib import Path


# ط¨ط±ظ†ط§ظ…ط¬ MQTT ط§ظ„ظ…ط³طھط®ط¯ظ… ظ„ظ„ط¥ط±ط³ط§ظ„ ظ…ظ† ط§ظ„ظ„ط§ط¨طھظˆط¨
MOSQUITTO_PUB = r"C:\Program Files\mosquitto\mosquitto_pub.exe"

# ط´ظ‡ط§ط¯ط© TLS ط§ظ„ط®ط§طµط© ط¨ظ€ ThingsBoard
CA_CERT = str(Path(__file__).resolve().parents[1] / "certs" / "tb-cloud-root-ca.pem")

# ط¥ط¹ط¯ط§ط¯ط§طھ ThingsBoard
HOST = "mqtt.eu.thingsboard.cloud"
PORT = "8883"
TOPIC = "v1/devices/me/telemetry"

# =========================================================
# ط§ظ„طھط®ط²ظٹظ† ط§ظ„ظ…ط­ظ„ظٹ ظ„ظ„ط£ط­ط¯ط§ط« ط¹ظ†ط¯ ط§ظ†ظ‚ط·ط§ط¹ ط§ظ„ط¥ظ†طھط±ظ†طھ
# =========================================================
PROJECT_ROOT = Path(__file__).resolve().parents[1]
QUEUE_DIR = PROJECT_ROOT / "data"
QUEUE_FILE = QUEUE_DIR / "pending_thingsboard_events.jsonl"

QUEUE_DIR.mkdir(parents=True, exist_ok=True)


def _get_token():
    """ظ‚ط±ط§ط،ط© Access Token ظ…ظ† ظ…طھط؛ظٹط± ط§ظ„ط¨ظٹط¦ط©."""
    return os.environ.get("TB_AI_TOKEN", "").strip()


def _publish_payload(payload: str):
    """
    ط¥ط±ط³ط§ظ„ Payload ظˆط§ط­ط¯ ط¥ظ„ظ‰ ThingsBoard ط¹ط¨ط± MQTTS/TLS.
    ظ‡ط°ظ‡ ط§ظ„ط¯ط§ظ„ط© ظ„ط§ طھط­ظپط¸ ظپظٹ ط§ظ„ظ€ Queue ط­طھظ‰ ظ„ط§ ظٹط­ط¯ط« طھظƒط±ط§ط±.
    """
    token = _get_token()

    if not token:
        print("ERROR: TB_AI_TOKEN is not set")
        return False

    command = [
        MOSQUITTO_PUB,
        "-q", "1",
        "-V", "mqttv311",
        "--cafile", CA_CERT,
        "-h", HOST,
        "-p", PORT,
        "-t", TOPIC,
        "-u", token,
        "-m", payload
    ]

    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=12
        )
    except Exception as e:
        print("ThingsBoard connection error:", e)
        return False

    if result.returncode == 0:
        return True

    print("ThingsBoard ERROR:")
    print(result.stderr.strip())
    return False


def _save_pending_payload(payload: str):
    """
    ط­ظپط¸ ط§ظ„ط­ط¯ط« ظ…ط­ظ„ظٹظ‹ط§ ط¥ط°ط§ طھط¹ط°ط± ط¥ط±ط³ط§ظ„ظ‡.
    ظƒظ„ ط³ط·ط± ظپظٹ ط§ظ„ظ…ظ„ظپ ظٹظ…ط«ظ„ ط­ط¯ط«ظ‹ط§ ظˆط§ط­ط¯ظ‹ط§.
    """
    try:
        with open(QUEUE_FILE, "a", encoding="utf-8") as f:
            f.write(payload + "\n")

        print(f"[OFFLINE QUEUE] Saved locally: {QUEUE_FILE}")
        return True

    except Exception as e:
        print("[OFFLINE QUEUE] Save failed:", e)
        return False


def get_pending_count():
    """ط¥ط±ط¬ط§ط¹ ط¹ط¯ط¯ ط§ظ„ط£ط­ط¯ط§ط« ط§ظ„ظ…ظˆط¬ظˆط¯ط© ط­ط§ظ„ظٹظ‹ط§ ظپظٹ ط§ظ„ظ€ Queue."""
    if not QUEUE_FILE.exists():
        return 0

    try:
        with open(QUEUE_FILE, "r", encoding="utf-8") as f:
            return sum(1 for line in f if line.strip())
    except Exception:
        return 0


def flush_pending_events():
    """
    ظ…ط­ط§ظˆظ„ط© ط¥ط¹ط§ط¯ط© ط¥ط±ط³ط§ظ„ ظƒظ„ ط§ظ„ط£ط­ط¯ط§ط« ط§ظ„ظ…ط®ط²ظ†ط© ظ…ط­ظ„ظٹظ‹ط§.
    ط¥ط°ط§ ظپط´ظ„ ط§ظ„ط§طھطµط§ظ„ ط£ط«ظ†ط§ط، ط§ظ„ط¥ط±ط³ط§ظ„طŒ ظ†ط­ط§ظپط¸ ط¹ظ„ظ‰ ط§ظ„ط­ط¯ط« ط§ظ„ط­ط§ظ„ظٹ
    ظˆظƒظ„ ط§ظ„ط£ط­ط¯ط§ط« ط§ظ„طھظٹ ط¨ط¹ط¯ظ‡ ط¯ط§ط®ظ„ ط§ظ„ظ…ظ„ظپ.
    """
    if not QUEUE_FILE.exists():
        return True

    try:
        with open(QUEUE_FILE, "r", encoding="utf-8") as f:
            pending = [line.strip() for line in f if line.strip()]
    except Exception as e:
        print("[OFFLINE QUEUE] Read failed:", e)
        return False

    if not pending:
        try:
            QUEUE_FILE.unlink(missing_ok=True)
        except Exception:
            pass
        return True

    print(f"[OFFLINE QUEUE] Trying to resend {len(pending)} event(s)...")

    remaining = []

    for index, payload in enumerate(pending):
        if _publish_payload(payload):
            print(f"[OFFLINE QUEUE] Resent {index + 1}/{len(pending)}")
        else:
            remaining = pending[index:]
            break

    try:
        if remaining:
            temp_file = QUEUE_FILE.with_suffix(".tmp")

            with open(temp_file, "w", encoding="utf-8") as f:
                for payload in remaining:
                    f.write(payload + "\n")

            temp_file.replace(QUEUE_FILE)

            print(
                f"[OFFLINE QUEUE] {len(remaining)} event(s) still pending."
            )
            return False

        QUEUE_FILE.unlink(missing_ok=True)
        print("[OFFLINE QUEUE] All pending events were sent successfully.")
        return True

    except Exception as e:
        print("[OFFLINE QUEUE] Update failed:", e)
        return False


def send_cashier_event(
    product_name,
    event_type,
    confidence,
    cart_count,
    cart_total,
    invoice_id=None,
    quantity=1
):
    """
    ط¥ط±ط³ط§ظ„ ط­ط¯ط« ط§ظ„ظƒط§ط´ظٹط± ط¥ظ„ظ‰ ThingsBoard.

    ط§ظ„ط³ظ„ظˆظƒ:
    1) ظٹط­ط§ظˆظ„ ط£ظˆظ„ظ‹ط§ ط¥ط±ط³ط§ظ„ ط£ظٹ ط£ط­ط¯ط§ط« ظ‚ط¯ظٹظ…ط© ظ…ط®ط²ظ†ط© ظ…ط­ظ„ظٹظ‹ط§.
    2) ظٹط±ط³ظ„ ط§ظ„ط­ط¯ط« ط§ظ„ط­ط§ظ„ظٹ.
    3) ط¥ط°ط§ ظپط´ظ„ ط§ظ„ط¥ط±ط³ط§ظ„طŒ ظٹط­ظپط¸ ط§ظ„ط­ط¯ط« ظ…ط­ظ„ظٹظ‹ط§ ظ„ط¥ط¹ط§ط¯ط© ط¥ط±ط³ط§ظ„ظ‡ ظ„ط§ط­ظ‚ظ‹ط§.
    """

    data = {
        "product_name": product_name,
        "event_type": event_type,
        "confidence": float(confidence),
        "quantity": int(quantity),
        "cart_count": int(cart_count),
        "cart_total": float(cart_total)
    }

    if invoice_id:
        data["invoice_id"] = str(invoice_id)

    payload = json.dumps(
        data,
        ensure_ascii=False,
        separators=(",", ":")
    )

    # ط¥ط¹ط§ط¯ط© ط¥ط±ط³ط§ظ„ ط§ظ„ط£ط­ط¯ط§ط« ط§ظ„ظ‚ط¯ظٹظ…ط© ط£ظˆظ„ظ‹ط§ ط¥ظ† ظˆط¬ط¯طھ
    if get_pending_count() > 0:
        flush_pending_events()

    # ظ…ط­ط§ظˆظ„ط© ط¥ط±ط³ط§ظ„ ط§ظ„ط­ط¯ط« ط§ظ„ط­ط§ظ„ظٹ
    if _publish_payload(payload):
        print("ThingsBoard:", payload)
        return True

    # ظپط´ظ„ ط§ظ„ط¥ط±ط³ط§ظ„: طھط®ط²ظٹظ† ظ…ط­ظ„ظٹ
    _save_pending_payload(payload)

    print(
        f"[OFFLINE QUEUE] Current pending count: {get_pending_count()}"
    )

    return False


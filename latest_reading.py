import json
import os
from datetime import datetime
from hashlib import sha256
from zoneinfo import ZoneInfo

import dotenv
import requests


def main() -> None:
    dotenv.load_dotenv()

    base_url = os.environ.get("LIBRE_LINK_UP_URL", "https://api.libreview.io")
    version = os.environ.get("LIBRE_LINK_UP_VERSION", "4.16.0")

    headers = {
        "product": "llu.android",
        "version": version,
        "accept-encoding": "gzip",
        "cache-control": "no-cache",
        "connection": "Keep-Alive",
        "content-type": "application/json",
        "user-agent": (
            "Mozilla/5.0 (Linux; Android 10; Pixel 3) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Chrome/88.0.4324.181 Mobile Safari/537.36"
        ),
    }

    login_response = requests.post(
        f"{base_url}/llu/auth/login",
        headers=headers,
        json={
            "email": os.environ["LIBRE_LINK_UP_USERNAME"],
            "password": os.environ["LIBRE_LINK_UP_PASSWORD"],
        },
        timeout=20,
    )
    login_response.raise_for_status()
    login_data = login_response.json()["data"]

    headers["authorization"] = f"Bearer {login_data['authTicket']['token']}"
    headers["Account-Id"] = sha256(
        login_data["user"]["id"].encode("utf-8")
    ).hexdigest()

    connections_response = requests.get(
        f"{base_url}/llu/connections", headers=headers, timeout=20
    )
    connections_response.raise_for_status()
    connections = connections_response.json()["data"]
    if not connections:
        raise RuntimeError("No LibreLinkUp connections found for this account.")

    connection = connections[0]
    graph_response = requests.get(
        f"{base_url}/llu/connections/{connection['patientId']}/graph",
        headers=headers,
        timeout=20,
    )
    graph_response.raise_for_status()
    raw = graph_response.json()["data"]["connection"]["glucoseMeasurement"]

    timestamp = datetime.strptime(raw["Timestamp"], "%m/%d/%Y %I:%M:%S %p")
    timestamp_brt = timestamp.replace(tzinfo=ZoneInfo("America/Sao_Paulo"))

    print(
        json.dumps(
            {
                "patient": " ".join(
                    value
                    for value in [
                        connection.get("firstName", ""),
                        connection.get("lastName", ""),
                    ]
                    if value
                ).strip(),
                "timestamp_brt": timestamp_brt.isoformat(),
                "value_mg_dl": raw["ValueInMgPerDl"],
                "trend_arrow": raw.get("TrendArrow"),
                "measurement_color": raw.get("MeasurementColor"),
            },
            ensure_ascii=False,
            indent=2,
        )
    )


if __name__ == "__main__":
    main()

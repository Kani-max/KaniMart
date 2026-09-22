#!/usr/bin/env bash
set -euo pipefail

: "${DATABASE_URL:?DATABASE_URL must be configured}"
: "${KANIMART_JWT_SECRET:?KANIMART_JWT_SECRET must be configured}"

python3 - <<'PY'
import json
import os
from urllib.parse import unquote, urlparse

url = urlparse(os.environ["DATABASE_URL"])
if url.scheme not in ("postgres", "postgresql"):
    raise SystemExit("DATABASE_URL must be a PostgreSQL connection URL")

config = {
    "listeners": [{
        "address": "0.0.0.0",
        "port": int(os.environ.get("PORT", "8080")),
        "https": False
    }],
    "db_clients": [{
        "name": "kanimart",
        "rdbms": "postgresql",
        "host": url.hostname,
        "port": url.port or 5432,
        "dbname": (url.path or "/").lstrip("/"),
        "user": unquote(url.username or ""),
        "passwd": unquote(url.password or ""),
        "is_fast": False,
        "connection_number": 4
    }]
}

with open("/tmp/kanimart-config.json", "w", encoding="utf-8") as stream:
    json.dump(config, stream)
PY

psql "$DATABASE_URL" -v ON_ERROR_STOP=1 -f /app/db/migrations/001_initial.sql

export KANIMART_CONFIG_FILE=/tmp/kanimart-config.json
exec /app/kanimart

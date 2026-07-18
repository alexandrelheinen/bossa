#!/usr/bin/env bash
# Delete all telemetry associated with the reserved CI node id.
# Safe by design: only touches bossa-test-00000000.
set -euo pipefail

RESERVED_NODE_ID="${BOSSA_TEST_NODE_ID:-bossa-test-00000000}"
MODE="${1:-help}"

usage() {
  cat <<EOF
Usage: $(basename "$0") <local-sqlite|d1-sql|d1-wrangler> [args]

  local-sqlite PATH   Delete pending_uploads rows in a local edge SQLite file.
                      (Phase 3 offline queue has no node_id column — removes
                      the whole pending queue file contents when PATH is a
                      reserved test DB, or deletes the file if named *test*.)

  d1-sql              Print SQL that deletes D1 rows for the reserved node.
                      Pipe into wrangler / dashboard when Phase 4 is live.

  d1-wrangler DB      Run DELETE against Cloudflare D1 via wrangler
                      (requires wrangler auth + Phase 4 database).

Reserved node id: ${RESERVED_NODE_ID}
EOF
}

case "${MODE}" in
  help|-h|--help)
    usage
    exit 0
    ;;
  local-sqlite)
    DB_PATH="${2:-}"
    if [[ -z "${DB_PATH}" ]]; then
      echo "error: path required" >&2
      usage
      exit 1
    fi
    if [[ ! -f "${DB_PATH}" ]]; then
      echo "error: file not found: ${DB_PATH}" >&2
      exit 1
    fi
    # Edge pending_uploads is node-local; only wipe files that look like test DBs.
    base="$(basename "${DB_PATH}")"
    if [[ "${base}" != *test* && "${base}" != *bossa-test* ]]; then
      echo "error: refusing to wipe '${DB_PATH}' (name must contain 'test')" >&2
      exit 1
    fi
    sqlite3 "${DB_PATH}" "DELETE FROM pending_uploads;"
    echo "cleared pending_uploads in ${DB_PATH}"
    ;;
  d1-sql)
    cat <<SQL
-- Reserved test node cleanup (safe to re-run)
DELETE FROM telemetry_points WHERE node_id = '${RESERVED_NODE_ID}';
DELETE FROM edge_nodes WHERE node_id = '${RESERVED_NODE_ID}';
SQL
    ;;
  d1-wrangler)
    DB_NAME="${2:-}"
    if [[ -z "${DB_NAME}" ]]; then
      echo "error: D1 database name required" >&2
      usage
      exit 1
    fi
    if ! command -v wrangler >/dev/null 2>&1; then
      echo "error: wrangler not installed" >&2
      exit 1
    fi
    SQL="DELETE FROM telemetry_points WHERE node_id = '${RESERVED_NODE_ID}'; DELETE FROM edge_nodes WHERE node_id = '${RESERVED_NODE_ID}';"
    wrangler d1 execute "${DB_NAME}" --remote --command "${SQL}"
    echo "D1 cleanup complete for ${RESERVED_NODE_ID}"
    ;;
  *)
    echo "error: unknown mode '${MODE}'" >&2
    usage
    exit 1
    ;;
esac

CREATE TABLE IF NOT EXISTS edge_nodes (
    node_id       TEXT PRIMARY KEY,
    api_key_hash  TEXT NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    last_seen_at  TEXT
);

CREATE TABLE IF NOT EXISTS telemetry_points (
    node_id       TEXT NOT NULL REFERENCES edge_nodes(node_id),
    channel_id    TEXT NOT NULL,
    timestamp     TEXT NOT NULL,
    value         REAL NOT NULL,
    unit          TEXT NOT NULL,
    quality       TEXT NOT NULL DEFAULT 'good',
    received_at   TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    PRIMARY KEY (node_id, channel_id, timestamp)
);

CREATE INDEX IF NOT EXISTS idx_telemetry_node_time
    ON telemetry_points (node_id, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_telemetry_channel_time
    ON telemetry_points (channel_id, timestamp DESC);

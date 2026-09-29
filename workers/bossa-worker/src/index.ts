export interface Env {
  DB: D1Database;
}

export default {
  async fetch(request: Request, env: Env, ctx: ExecutionContext): Promise<Response> {
    const url = new URL(request.url);

    if (request.method === "POST" && url.pathname === "/api/v1/telemetry") {
      return handleTelemetry(request, env);
    }

    if (request.method === "GET") {
      if (url.pathname === "/api/v1/health") {
        return new Response(JSON.stringify({ status: "ok" }), {
          headers: { "Content-Type": "application/json" }
        });
      }
      if (url.pathname === "/api/v1/health/ready") {
        try {
          await env.DB.prepare("SELECT 1").first();
          return new Response(JSON.stringify({ status: "ready" }), {
            headers: { "Content-Type": "application/json" }
          });
        } catch (e) {
          return new Response(JSON.stringify({ status: "error", message: String(e) }), {
            status: 500,
            headers: { "Content-Type": "application/json" }
          });
        }
      }
      if (url.pathname === "/api/v1/nodes") {
        const { results } = await env.DB.prepare("SELECT * FROM edge_nodes").all();
        return new Response(JSON.stringify(results), {
          headers: { "Content-Type": "application/json" }
        });
      }
      if (url.pathname === "/api/v1/data") {
        const { results } = await env.DB.prepare("SELECT * FROM telemetry_points ORDER BY timestamp DESC LIMIT 100").all();
        return new Response(JSON.stringify(results), {
          headers: { "Content-Type": "application/json" }
        });
      }
      if (url.pathname === "/") {
        return handleDashboard(request, env);
      }
    }

    return new Response("Not Found", { status: 404 });
  }
};

async function handleTelemetry(request: Request, env: Env): Promise<Response> {
  const authHeader = request.headers.get("Authorization");
  if (!authHeader || !authHeader.startsWith("Bearer ")) {
    return new Response("Unauthorized", { status: 401 });
  }
  // In a real application, we would hash the token and compare it with the DB.
  // For this fake environment, we just accept "dummy-api-key".

  let payload: any;
  try {
    payload = await request.json();
  } catch (e) {
    return new Response("Bad Request", { status: 400 });
  }

  if (!payload.node_id || !Array.isArray(payload.samples)) {
    return new Response("Bad Request", { status: 400 });
  }

  // Insert or update node
  await env.DB.prepare(
    `INSERT INTO edge_nodes (node_id, api_key_hash, last_seen_at)
     VALUES (?, 'fake_hash', datetime('now'))
     ON CONFLICT(node_id) DO UPDATE SET last_seen_at=datetime('now')`
  ).bind(payload.node_id).run();

  const statements = [];
  for (const sample of payload.samples) {
    if (!sample.channel_id || !sample.timestamp || sample.value === undefined || !sample.unit) {
      continue;
    }

    statements.push(
      env.DB.prepare(
        `INSERT INTO telemetry_points (node_id, channel_id, timestamp, value, unit, quality)
         VALUES (?, ?, ?, ?, ?, ?)
         ON CONFLICT(node_id, channel_id, timestamp) DO NOTHING`
      ).bind(
        payload.node_id,
        sample.channel_id,
        sample.timestamp,
        sample.value,
        sample.unit,
        sample.quality || "good"
      )
    );
  }

  if (statements.length > 0) {
    await env.DB.batch(statements);
  }

  return new Response("Accepted", { status: 202 });
}

async function handleDashboard(request: Request, env: Env): Promise<Response> {
  const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BOSSA Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: sans-serif; margin: 20px; }
    .container { max-width: 1200px; margin: auto; }
    .chart-container { width: 100%; height: 400px; margin-bottom: 40px; }
    table { width: 100%; border-collapse: collapse; margin-bottom: 40px; }
    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
    th { background-color: #f2f2f2; }
  </style>
</head>
<body>
  <div class="container">
    <h1>BOSSA Telemetry Dashboard</h1>

    <h2>Edge Nodes</h2>
    <table id="nodesTable">
      <thead>
        <tr>
          <th>Node ID</th>
          <th>Created At</th>
          <th>Last Seen At</th>
        </tr>
      </thead>
      <tbody></tbody>
    </table>

    <h2>Telemetry Data</h2>
    <div class="chart-container">
      <canvas id="telemetryChart"></canvas>
    </div>
  </div>

  <script>
    async function fetchData() {
      const [nodesRes, dataRes] = await Promise.all([
        fetch('/api/v1/nodes'),
        fetch('/api/v1/data')
      ]);
      const nodes = await nodesRes.json();
      const data = await dataRes.json();

      const tbody = document.querySelector('#nodesTable tbody');
      tbody.innerHTML = nodes.map(n =>
        \`<tr><td>\${n.node_id}</td><td>\${n.created_at}</td><td>\${n.last_seen_at}</td></tr>\`
      ).join('');

      renderChart(data);
    }

    let chart;
    function renderChart(data) {
      const ctx = document.getElementById('telemetryChart').getContext('2d');

      // Group by channel
      const channels = {};
      data.forEach(d => {
        if (!channels[d.channel_id]) channels[d.channel_id] = [];
        channels[d.channel_id].push(d);
      });

      const datasets = Object.keys(channels).map(channelId => {
        // Sort by time ascending for chart
        const points = channels[channelId].sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
        return {
          label: channelId,
          data: points.map(p => ({ x: p.timestamp, y: p.value })),
          borderColor: getRandomColor(),
          fill: false,
          tension: 0.1
        };
      });

      if (chart) {
        chart.destroy();
      }

      chart = new Chart(ctx, {
        type: 'line',
        data: { datasets },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            x: {
              type: 'category',
              labels: datasets.length > 0 ? datasets[0].data.map(d => new Date(d.x).toLocaleTimeString()) : []
            },
            y: { beginAtZero: false }
          }
        }
      });
    }

    function getRandomColor() {
      const letters = '0123456789ABCDEF';
      let color = '#';
      for (let i = 0; i < 6; i++) {
        color += letters[Math.floor(Math.random() * 16)];
      }
      return color;
    }

    fetchData();
    setInterval(fetchData, 5000); // Refresh every 5s
  </script>
</body>
</html>`;

  return new Response(html, {
    headers: { "Content-Type": "text/html" }
  });
}

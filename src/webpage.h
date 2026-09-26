#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>24VDC Power & Environmental Monitor</title>
  <style>
    :root {
      --bg-base: #0b0f19;
      --bg-card: rgba(18, 26, 44, 0.85);
      --border-card: rgba(255, 255, 255, 0.08);
      --border-highlight: rgba(0, 212, 255, 0.3);
      --text-main: #f1f5f9;
      --text-muted: #94a3b8;
      --cyan: #00d4ff;
      --green: #10b981;
      --amber: #f59e0b;
      --red: #ef4444;
      --purple: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", sans-serif; }
    body {
      background: radial-gradient(circle at 50% 0%, #172554 0%, var(--bg-base) 70%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 1.25rem;
    }
    .container { max-width: 1200px; margin: 0 auto; }
    header {
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 1.5rem;
      padding-bottom: 1rem;
      border-bottom: 1px solid var(--border-card);
      gap: 1rem;
    }
    .brand { display: flex; align-items: center; gap: 0.75rem; }
    .brand-icon {
      width: 42px; height: 42px;
      background: linear-gradient(135deg, #00d4ff, #3b82f6);
      border-radius: 12px;
      display: flex; align-items: center; justify-content: center;
      box-shadow: 0 0 20px rgba(0, 212, 255, 0.4);
    }
    .brand-icon svg { width: 24px; height: 24px; fill: #fff; }
    .brand h1 { font-size: 1.35rem; font-weight: 700; letter-spacing: -0.5px; }
    .brand p { font-size: 0.8rem; color: var(--text-muted); }
    .status-bar {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.75rem;
      background: var(--bg-card);
      padding: 0.5rem 1rem;
      border-radius: 9999px;
      border: 1px solid var(--border-card);
      font-size: 0.82rem;
    }
    .pill { display: flex; align-items: center; gap: 0.4rem; }
    .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--green); box-shadow: 0 0 8px var(--green); }
    .dot.warn { background: var(--amber); box-shadow: 0 0 8px var(--amber); }
    .dot.err { background: var(--red); box-shadow: 0 0 8px var(--red); }
    
    /* Grid */
    .grid-cards {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 1.25rem;
      margin-bottom: 1.5rem;
    }
    .card {
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      border: 1px solid var(--border-card);
      border-radius: 16px;
      padding: 1.25rem;
      position: relative;
      overflow: hidden;
      transition: border-color 0.2s, transform 0.2s;
    }
    .card:hover { border-color: var(--border-highlight); transform: translateY(-2px); }
    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 0.75rem;
    }
    .card-title { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-muted); font-weight: 600; }
    .card-icon { width: 32px; height: 32px; border-radius: 8px; display: flex; align-items: center; justify-content: center; }
    .val-container { display: flex; align-items: baseline; gap: 0.35rem; margin: 0.25rem 0 0.5rem; }
    .val-large { font-size: 2.2rem; font-weight: 800; letter-spacing: -1px; }
    .val-unit { font-size: 1.1rem; color: var(--text-muted); font-weight: 500; }
    .sub-stats {
      display: flex;
      justify-content: space-between;
      font-size: 0.75rem;
      color: var(--text-muted);
      margin-top: 0.75rem;
      padding-top: 0.75rem;
      border-top: 1px solid rgba(255,255,255,0.05);
    }
    .sub-stat-col {
      display: flex;
      flex-direction: column;
      gap: 0.2rem;
    }
    .sub-stat-col span { color: var(--text-main); font-weight: 600; }

    /* Progress bar */
    .bar-wrap { width: 100%; height: 6px; background: rgba(255,255,255,0.1); border-radius: 999px; overflow: hidden; margin-top: 0.5rem; }
    .bar-fill { height: 100%; border-radius: 999px; transition: width 0.4s ease; }
    
    /* Alerts Panel */
    .alert-banner {
      background: rgba(239, 68, 68, 0.15);
      border: 1px solid var(--red);
      color: #fca5a5;
      padding: 0.75rem 1.25rem;
      border-radius: 12px;
      margin-bottom: 1.25rem;
      display: none;
      align-items: center;
      gap: 0.75rem;
      font-size: 0.9rem;
    }
    
    /* Charts */
    .charts-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(450px, 1fr));
      gap: 1.25rem;
      margin-bottom: 1.5rem;
    }
    @media (max-width: 600px) {
      .charts-grid { grid-template-columns: 1fr; }
    }
    .chart-card {
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      border: 1px solid var(--border-card);
      border-radius: 16px;
      padding: 1.25rem;
    }
    .chart-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; }
    .chart-title { font-size: 0.95rem; font-weight: 600; }
    .chart-canvas-wrap { position: relative; width: 100%; height: 180px; }
    canvas { width: 100% !important; height: 100% !important; }

    /* Power Quality & Diagnostics Grid */
    .pq-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(360px, 1fr));
      gap: 1.25rem;
      margin-bottom: 1.5rem;
    }
    .pq-card {
      background: var(--bg-card);
      border: 1px solid var(--border-card);
      border-radius: 16px;
      padding: 1.25rem;
    }
    .pq-title { font-size: 0.9rem; font-weight: 600; margin-bottom: 0.75rem; color: var(--cyan); display: flex; justify-content: space-between; align-items: center; }
    .pq-stat-row { display: flex; justify-content: space-between; align-items: center; padding: 0.4rem 0; border-bottom: 1px solid rgba(255,255,255,0.04); font-size: 0.82rem; }
    .pq-stat-row:last-child { border-bottom: none; }
    .pq-stat-label { color: var(--text-muted); }
    .pq-stat-val { font-weight: 600; color: var(--text-main); }
    .tag { display: inline-block; padding: 0.2rem 0.5rem; border-radius: 6px; font-size: 0.75rem; font-family: monospace; background: rgba(255,255,255,0.08); margin-right: 0.4rem; margin-bottom: 0.4rem; }
    .tag-ok { background: rgba(16, 185, 129, 0.2); color: #6ee7b7; border: 1px solid rgba(16, 185, 129, 0.4); }
    .tag-err { background: rgba(239, 68, 68, 0.2); color: #fca5a5; border: 1px solid rgba(239, 68, 68, 0.4); }

    /* Footer & Controls */
    .bottom-bar {
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      gap: 1rem;
      background: var(--bg-card);
      border: 1px solid var(--border-card);
      border-radius: 12px;
      padding: 0.75rem 1.25rem;
      font-size: 0.82rem;
      color: var(--text-muted);
    }
    .btn {
      background: rgba(255,255,255,0.08);
      border: 1px solid var(--border-card);
      color: var(--text-main);
      padding: 0.45rem 0.9rem;
      border-radius: 8px;
      font-size: 0.8rem;
      cursor: pointer;
      text-decoration: none;
      display: inline-flex;
      align-items: center;
      transition: all 0.2s;
    }
    .btn:hover { background: rgba(255,255,255,0.15); }
    .btn-sm { padding: 0.25rem 0.6rem; font-size: 0.75rem; }
    .btn-primary { background: linear-gradient(135deg, #0284c7, #2563eb); border: none; color: #fff; }
    .btn-danger:hover { background: rgba(239, 68, 68, 0.3); border-color: var(--red); color: #fff; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="brand">
        <div class="brand-icon">
          <svg viewBox="0 0 24 24"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>
        </div>
        <div>
          <h1>24VDC Power & Enclosure Monitor</h1>
          <p>ESP32-S3 Dual-Core RTOS &bull; <span id="hdr-version" style="color:var(--cyan); font-weight:700;">v1.5.0-RTOS</span> (<span id="hdr-build" style="color:var(--text-muted);">--</span>)</p>
        </div>
      </div>
      <div class="status-bar">
        <div class="pill">
          <svg style="width:14px;height:14px;fill:var(--cyan);" viewBox="0 0 24 24"><path d="M12 4C7.31 4 3.07 5.9 0 8.98L12 21 24 8.98A16.88 16.88 0 0 0 12 4zm0 3.8c3.48 0 6.64 1.35 9.02 3.56L12 18.59 2.98 11.36A12.02 12.02 0 0 1 12 7.8z"/></svg>
          <span id="dot-wifi" class="dot"></span>
          <span id="lbl-wifi" style="font-weight:600;">WiFi: Connecting...</span>
        </div>
        <div class="pill"><span id="dot-ina" class="dot"></span> <span id="lbl-ina">INA226: Active</span></div>
        <div class="pill"><span id="dot-sht" class="dot"></span> <span id="lbl-sht">SHT30: Active</span></div>
        <div class="pill">Uptime: <span id="lbl-uptime" style="color:var(--text-main); font-weight:600; margin-left:3px;">0h 0m</span></div>
      </div>
    </header>

    <div id="alertBanner" class="alert-banner">
      <svg style="width:20px;height:20px;fill:currentColor;flex-shrink:0;" viewBox="0 0 24 24"><path d="M12 2L1 21h22L12 2zm0 3.5L19.5 19h-15L12 5.5zM11 10v4h2v-4h-2zm0 6v2h2v-2h-2z"/></svg>
      <span id="alertText">Warning: Transient condition detected!</span>
    </div>

    <!-- Main Metrics Grid -->
    <div class="grid-cards">
      <!-- Voltage Card with Peaks, Dips and Ripple -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">24V Bus Voltage</span>
          <div class="card-icon" style="background: rgba(0, 212, 255, 0.15); color: var(--cyan);">
            <svg style="width:18px;height:18px;fill:currentColor" viewBox="0 0 24 24"><path d="M11 15H6l7-14v8h5l-7 14v-8z"/></svg>
          </div>
        </div>
        <div class="val-container">
          <span id="val-voltage" class="val-large" style="color: var(--cyan);">--.--</span>
          <span class="val-unit">V</span>
        </div>
        <div class="bar-wrap">
          <div id="bar-voltage" class="bar-fill" style="background: var(--cyan); width: 0%;"></div>
        </div>
        <div class="sub-stats">
          <div class="sub-stat-col">
            <span style="color:var(--text-muted); font-size:0.7rem;">LOWEST DIP</span>
            <span id="val-lowest-dip" style="color:#38bdf8;">--.-- V</span>
          </div>
          <div class="sub-stat-col" style="text-align:center;">
            <span style="color:var(--text-muted); font-size:0.7rem;">RIPPLE (Vpp)</span>
            <span id="val-v-ripple" style="color:#a78bfa;">-- mV</span>
          </div>
          <div class="sub-stat-col" style="text-align:right;">
            <span style="color:var(--text-muted); font-size:0.7rem;">PEAK / MAX</span>
            <span id="val-peak-volt" style="color:#f472b6;">--.-- V</span>
          </div>
        </div>
      </div>

      <!-- Current & Session Ah Card -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Load Current & Charge</span>
          <div class="card-icon" style="background: rgba(16, 185, 129, 0.15); color: var(--green);">
            <svg style="width:18px;height:18px;fill:currentColor" viewBox="0 0 24 24"><path d="M13 2.05v3.03c3.39.49 6 3.39 6 6.92 0 .9-.18 1.75-.48 2.54l2.6 1.53c.56-1.24.88-2.62.88-4.07 0-5.18-3.95-9.45-9-9.95zM12 19c-3.87 0-7-3.13-7-7 0-3.53 2.61-6.43 6-6.92V2.05c-5.05.5-9 4.77-9 9.95 0 5.52 4.47 10 9.99 10 3.31 0 6.24-1.61 8.01-4.09l-2.49-1.46C16.27 17.84 14.28 19 12 19z"/></svg>
          </div>
        </div>
        <div class="val-container">
          <span id="val-current" class="val-large" style="color: var(--green);">--.---</span>
          <span class="val-unit">A</span>
        </div>
        <div class="bar-wrap">
          <div id="bar-current" class="bar-fill" style="background: var(--green); width: 0%;"></div>
        </div>
        <div class="sub-stats">
          <div class="sub-stat-col">
            <span style="color:var(--text-muted); font-size:0.7rem;">SESSION Ah</span>
            <span id="val-session-ah" style="color:#34d399;">0.000 Ah</span>
          </div>
          <div class="sub-stat-col" style="text-align:right;">
            <span style="color:var(--text-muted); font-size:0.7rem;">PEAK CURRENT</span>
            <span id="peak-current">-- A</span>
          </div>
        </div>
      </div>

      <!-- Power & Energy to Date (kWh / Wh) Card -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Power & Energy to Date</span>
          <div class="card-icon" style="background: rgba(245, 158, 11, 0.15); color: var(--amber);">
            <svg style="width:18px;height:18px;fill:currentColor" viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 14h-2v-2h2v2zm0-4h-2V7h2v5z"/></svg>
          </div>
        </div>
        <div class="val-container">
          <span id="val-power" class="val-large" style="color: var(--amber);">--.-</span>
          <span class="val-unit">W</span>
        </div>
        <div class="bar-wrap">
          <div id="bar-power" class="bar-fill" style="background: var(--amber); width: 0%;"></div>
        </div>
        <div class="sub-stats">
          <div class="sub-stat-col">
            <span style="color:var(--text-muted); font-size:0.7rem;">TOTAL TO DATE</span>
            <span id="val-total-kwh" style="color:#fbbf24;">0.000 kWh</span>
          </div>
          <div class="sub-stat-col" style="text-align:right;">
            <span style="color:var(--text-muted); font-size:0.7rem;">SESSION Wh</span>
            <span id="val-session-wh" style="color:#fde68a;">0.0 Wh</span>
          </div>
        </div>
      </div>

      <!-- Temperature & Enclosure Safety Card -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Enclosure Climate</span>
          <div class="card-icon" style="background: rgba(168, 85, 247, 0.15); color: var(--purple);">
            <svg style="width:18px;height:18px;fill:currentColor" viewBox="0 0 24 24"><path d="M12 2c-4.42 0-8 3.58-8 8 0 2.21.9 4.21 2.35 5.65L12 21.3l5.65-5.65C19.1 14.21 20 12.21 20 10c0-4.42-3.58-8-8-8zm0 11c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3z"/></svg>
          </div>
        </div>
        <div class="val-container">
          <span id="val-temp" class="val-large" style="color: var(--purple);">--.-</span>
          <span class="val-unit">°C</span>
        </div>
        <div class="bar-wrap">
          <div id="bar-temp" class="bar-fill" style="background: var(--purple); width: 0%;"></div>
        </div>
        <div class="sub-stats">
          <div class="sub-stat-col">
            <span style="color:var(--text-muted); font-size:0.7rem;">HUMIDITY</span>
            <span id="val-humidity">--%</span>
          </div>
          <div class="sub-stat-col" style="text-align:right;">
            <span style="color:var(--text-muted); font-size:0.7rem;">CONDENSATION MARGIN</span>
            <span id="val-cond-margin" style="color:#c084fc;">-- °C</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Live Charts Grid -->
    <div class="charts-grid">
      <!-- Electrical Chart -->
      <div class="chart-card">
        <div class="chart-header">
          <span class="chart-title">Electrical Telemetry (Voltage & Current)</span>
          <span style="font-size:0.75rem; color:var(--text-muted);">Real-time rolling buffer</span>
        </div>
        <div class="chart-canvas-wrap">
          <canvas id="canvasElectrical"></canvas>
        </div>
      </div>

      <!-- Climate Chart -->
      <div class="chart-card">
        <div class="chart-header">
          <span class="chart-title">Enclosure Climate (Temp & Humidity)</span>
          <span style="font-size:0.75rem; color:var(--text-muted);">Real-time rolling buffer</span>
        </div>
        <div class="chart-canvas-wrap">
          <canvas id="canvasClimate"></canvas>
        </div>
      </div>
    </div>

    <!-- Advanced Power Quality & Load Analytics Grid -->
    <div class="pq-grid">
      <!-- Power Quality & Voltage Transients -->
      <div class="pq-card">
        <div class="pq-title">
          <span>Voltage Transients & Rail Quality</span>
          <button class="btn btn-sm" onclick="resetVoltageStats()">Reset Min/Max</button>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Peak-to-Peak Ripple (V<sub>pp</sub>):</span>
          <span class="pq-stat-val" id="pq-ripple" style="color:#a78bfa;">-- mV</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Lowest Voltage Dip:</span>
          <span class="pq-stat-val" id="pq-dip-v" style="color:#38bdf8;">--.-- V</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Peak Surge Voltage:</span>
          <span class="pq-stat-val" id="pq-peak-v" style="color:#f472b6;">--.-- V</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Dip Events (&lt; 22.5V):</span>
          <span class="pq-stat-val" id="pq-dip-count">0</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Surge Events (&gt; 26.5V):</span>
          <span class="pq-stat-val" id="pq-surge-count">0</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Headroom to 21.6V Cutoff:</span>
          <span class="pq-stat-val" id="pq-headroom">-- V</span>
        </div>
      </div>

      <!-- Energy Analytics & Enclosure Safety -->
      <div class="pq-card">
        <div class="pq-title">Energy Analytics & Enclosure Safety</div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">1-Minute Average Power:</span>
          <span class="pq-stat-val" id="pq-avg-power">-- W</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Projected Monthly Consumption:</span>
          <span class="pq-stat-val" id="pq-proj-month" style="color:#fbbf24;">-- kWh</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Shunt Heat Loss (I&sup2;R):</span>
          <span class="pq-stat-val" id="pq-shunt-loss">-- mW</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Dew Point:</span>
          <span class="pq-stat-val" id="val-dewpoint">-- &deg;C</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">Condensation Margin (&Delta;T<sub>dew</sub>):</span>
          <span class="pq-stat-val" id="pq-cond-margin" style="color:#34d399;">-- &deg;C (Safe)</span>
        </div>
        <div class="pq-stat-row">
          <span class="pq-stat-label">I²C Bus Devices:</span>
          <span class="pq-stat-val" id="i2c-tags-list">Scanning...</span>
        </div>
      </div>
    </div>

    <!-- Bottom Controls & Info -->
    <div class="bottom-bar">
      <div>
        <span>SSID: <b id="lbl-ssid" style="color:var(--text-main)">--</b></span> &bull; 
        <span>IP: <b id="lbl-ip" style="color:var(--text-main)">192.168.0.191</b></span> &bull; 
        <span>FW: <b id="lbl-version" style="color:var(--cyan)">v1.5.0-RTOS</b></span> &bull; 
        <span>Built: <span id="lbl-build" style="color:var(--text-main)">--</span></span> &bull; 
        <span>Free Heap: <span id="lbl-heap">-- KB</span></span>
      </div>
      <div style="display:flex; gap:0.5rem;">
        <a class="btn" href="/setup">Configure Wi-Fi</a>
        <button class="btn btn-danger" onclick="resetEnergy()">Reset Cumulative Energy</button>
        <button class="btn" onclick="fetchData()">Refresh Now</button>
      </div>
    </div>
  </div>

  <script>
    const MAX_POINTS = 60;
    const historyData = {
      voltage: [],
      current: [],
      power: [],
      temp: [],
      humidity: []
    };

    function drawChart(canvasId, series1, color1, label1, series2, color2, label2, min1, max1, min2, max2) {
      const canvas = document.getElementById(canvasId);
      if (!canvas) return;
      const ctx = canvas.getContext('2d');
      const w = canvas.parentElement.clientWidth;
      const h = canvas.parentElement.clientHeight;
      canvas.width = w * window.devicePixelRatio;
      canvas.height = h * window.devicePixelRatio;
      ctx.scale(window.devicePixelRatio, window.devicePixelRatio);

      ctx.clearRect(0, 0, w, h);

      // Grid lines
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
      ctx.lineWidth = 1;
      for (let y = 20; y < h - 20; y += 35) {
        ctx.beginPath();
        ctx.moveTo(40, y);
        ctx.lineTo(w - 10, y);
        ctx.stroke();
      }

      function renderLine(data, color, minVal, maxVal) {
        if (data.length < 2) return;
        const range = (maxVal - minVal) || 1;
        const pts = data.map((v, i) => {
          const x = 40 + (i / (MAX_POINTS - 1)) * (w - 50);
          const y = (h - 25) - ((v - minVal) / range) * (h - 50);
          return { x, y: Math.max(15, Math.min(h - 15, y)) };
        });

        // Area fill
        ctx.beginPath();
        ctx.moveTo(pts[0].x, h - 25);
        pts.forEach(p => ctx.lineTo(p.x, p.y));
        ctx.lineTo(pts[pts.length - 1].x, h - 25);
        ctx.closePath();
        ctx.fillStyle = color.replace(')', ', 0.1)').replace('rgb', 'rgba');
        ctx.fill();

        // Line stroke
        ctx.beginPath();
        pts.forEach((p, i) => i === 0 ? ctx.moveTo(p.x, p.y) : ctx.lineTo(p.x, p.y));
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.stroke();
      }

      const v1Min = min1 !== undefined ? min1 : Math.min(...series1, 0);
      const v1Max = max1 !== undefined ? max1 : Math.max(...series1, 1);
      renderLine(series1, color1, v1Min, v1Max);

      const v2Min = min2 !== undefined ? min2 : Math.min(...series2, 0);
      const v2Max = max2 !== undefined ? max2 : Math.max(...series2, 1);
      renderLine(series2, color2, v2Min, v2Max);

      ctx.fillStyle = '#64748b';
      ctx.font = '10px sans-serif';
      ctx.fillText(label1 + ' (' + (series1[series1.length-1] || 0) + ')', 40, 12);
      ctx.fillStyle = color2;
      ctx.fillText(label2 + ' (' + (series2[series2.length-1] || 0) + ')', w - 130, 12);
    }

    async function fetchData() {
      try {
        const res = await fetch('/api/data?nocache=' + Date.now());
        if (!res.ok) throw new Error('Network error');
        const data = await res.json();

        // 1. Voltage, Peaks, Dips & Ripple
        if (data.voltage !== undefined) {
          const v = Number(data.voltage);
          document.getElementById('val-voltage').innerText = v.toFixed(2);
          document.getElementById('bar-voltage').style.width = Math.min(100, Math.max(0, (v / 30) * 100)) + '%';
          const headroom = Math.max(0, v - 21.6);
          document.getElementById('pq-headroom').innerText = '+' + headroom.toFixed(2) + ' V (above 21.6V)';
        }
        if (data.lowest_dip_v !== undefined) {
          document.getElementById('val-lowest-dip').innerText = (data.lowest_dip_v < 900) ? Number(data.lowest_dip_v).toFixed(2) + ' V' : '-- V';
          document.getElementById('pq-dip-v').innerText = (data.lowest_dip_v < 900) ? Number(data.lowest_dip_v).toFixed(2) + ' V' : '-- V';
        }
        if (data.peak_voltage !== undefined) {
          document.getElementById('val-peak-volt').innerText = Number(data.peak_voltage).toFixed(2) + ' V';
          document.getElementById('pq-peak-v').innerText = Number(data.peak_voltage).toFixed(2) + ' V';
        }
        if (data.v_ripple_mv !== undefined) {
          document.getElementById('val-v-ripple').innerText = Number(data.v_ripple_mv).toFixed(0) + ' mV';
          document.getElementById('pq-ripple').innerText = Number(data.v_ripple_mv).toFixed(1) + ' mV';
        }
        if (data.dip_count !== undefined) document.getElementById('pq-dip-count').innerText = data.dip_count;
        if (data.surge_count !== undefined) document.getElementById('pq-surge-count').innerText = data.surge_count;

        // 2. Current & Session Ah
        if (data.current !== undefined) {
          document.getElementById('val-current').innerText = Number(data.current).toFixed(3);
          document.getElementById('bar-current').style.width = Math.min(100, Math.max(0, (Number(data.current) / 8.0) * 100)) + '%';
        }
        if (data.peak_current !== undefined) {
          document.getElementById('peak-current').innerText = Number(data.peak_current).toFixed(2) + ' A';
        }
        if (data.session_ah !== undefined) {
          document.getElementById('val-session-ah').innerText = Number(data.session_ah).toFixed(3) + ' Ah';
        }

        // 3. Power, Energy To Date (kWh / Wh) & Analytics
        if (data.power !== undefined) {
          document.getElementById('val-power').innerText = Number(data.power).toFixed(1);
          document.getElementById('bar-power').style.width = Math.min(100, Math.max(0, (Number(data.power) / 200) * 100)) + '%';
        }
        if (data.total_kwh !== undefined) {
          document.getElementById('val-total-kwh').innerText = Number(data.total_kwh).toFixed(3) + ' kWh';
        } else if (data.energy_wh !== undefined) {
          document.getElementById('val-total-kwh').innerText = (Number(data.energy_wh) / 1000.0).toFixed(3) + ' kWh';
        }
        if (data.session_wh !== undefined) {
          document.getElementById('val-session-wh').innerText = Number(data.session_wh).toFixed(1) + ' Wh';
        }
        if (data.avg_power_1m !== undefined) {
          document.getElementById('pq-avg-power').innerText = Number(data.avg_power_1m).toFixed(1) + ' W';
        }
        if (data.projected_kwh_month !== undefined) {
          document.getElementById('pq-proj-month').innerText = Number(data.projected_kwh_month).toFixed(2) + ' kWh / mo';
        }
        if (data.shunt_loss_mw !== undefined) {
          document.getElementById('pq-shunt-loss').innerText = Number(data.shunt_loss_mw).toFixed(1) + ' mW';
        }

        // 4. SHT30 Climate & Condensation Margin
        if (data.sht_status) {
          const t = Number(data.temperature);
          const h = Number(data.humidity);
          const dew = Number(data.dew_point);
          const cMargin = Number(data.condensation_margin_c);

          document.getElementById('val-temp').innerText = t.toFixed(1);
          document.getElementById('val-humidity').innerText = h.toFixed(1) + '%';
          document.getElementById('val-dewpoint').innerText = dew.toFixed(1) + ' °C';
          document.getElementById('val-cond-margin').innerText = '+' + cMargin.toFixed(1) + ' °C';
          document.getElementById('bar-temp').style.width = Math.min(100, Math.max(0, (t / 80) * 100)) + '%';

          const cMarginEl = document.getElementById('pq-cond-margin');
          if (cMargin > 5.0) {
            cMarginEl.innerText = '+' + cMargin.toFixed(1) + ' °C (Safe)';
            cMarginEl.style.color = '#34d399';
          } else if (cMargin > 2.0) {
            cMarginEl.innerText = '+' + cMargin.toFixed(1) + ' °C (Warning: Dew Point close)';
            cMarginEl.style.color = '#fbbf24';
          } else {
            cMarginEl.innerText = '+' + cMargin.toFixed(1) + ' °C (DANGER: Condensation Risk!)';
            cMarginEl.style.color = '#ef4444';
          }

          document.getElementById('dot-sht').className = 'dot';
          document.getElementById('lbl-sht').innerText = 'SHT30: Active (0x' + (data.sht_addr ? data.sht_addr.toString(16) : '44') + ')';
        } else {
          document.getElementById('val-temp').innerText = 'N/C';
          document.getElementById('val-humidity').innerText = 'N/C';
          document.getElementById('val-dewpoint').innerText = 'N/C';
          document.getElementById('val-cond-margin').innerText = 'N/C';
          document.getElementById('pq-cond-margin').innerText = 'N/C';
          document.getElementById('dot-sht').className = 'dot err';
          document.getElementById('lbl-sht').innerText = 'SHT30: Disconnected';
        }

        // INA Status
        document.getElementById('dot-ina').className = data.ina_status ? 'dot' : 'dot err';
        document.getElementById('lbl-ina').innerText = data.ina_status ? 'INA226: Active' : 'INA226: Error';

        // WiFi Status, SSID & Signal Strength
        if (data.ssid && data.wifi_rssi !== undefined) {
          const rssi = Number(data.wifi_rssi);
          let signalQuality = Math.min(100, Math.max(0, Math.round(2 * (rssi + 100))));
          document.getElementById('lbl-wifi').innerText = data.ssid + ' (' + rssi + ' dBm, ' + signalQuality + '%)';
          document.getElementById('lbl-ssid').innerText = data.ssid;
          if (rssi > -65) {
            document.getElementById('dot-wifi').className = 'dot';
          } else if (rssi > -80) {
            document.getElementById('dot-wifi').className = 'dot warn';
          } else {
            document.getElementById('dot-wifi').className = 'dot err';
          }
        }

        // Firmware Version & Build Metadata
        if (data.version) {
          document.getElementById('hdr-version').innerText = data.version;
          document.getElementById('lbl-version').innerText = data.version;
        }
        if (data.build_date && data.build_time) {
          const bStr = data.build_date + ' ' + data.build_time;
          document.getElementById('hdr-build').innerText = bStr;
          document.getElementById('lbl-build').innerText = bStr;
        }

        // Uptime & System
        if (data.uptime_str) document.getElementById('lbl-uptime').innerText = data.uptime_str;
        if (data.ip) document.getElementById('lbl-ip').innerText = data.ip;
        if (data.free_heap) document.getElementById('lbl-heap').innerText = Math.round(data.free_heap / 1024) + ' KB';

        // I2C Tags
        const tagsContainer = document.getElementById('i2c-tags-list');
        if (data.i2c_devices && data.i2c_devices.length > 0) {
          tagsContainer.innerHTML = data.i2c_devices.map(addr => {
            const hex = '0x' + addr.toString(16).toUpperCase();
            let label = hex;
            if (addr === 0x40) label += ' (INA226)';
            else if (addr === 0x44 || addr === 0x45) label += ' (SHT30)';
            return `<span class="tag tag-ok">${label}</span>`;
          }).join(' ');
        } else {
          tagsContainer.innerHTML = `<span class="tag tag-err">None</span>`;
        }

        // Push to history
        if (historyData.voltage.length >= MAX_POINTS) {
          historyData.voltage.shift();
          historyData.current.shift();
          historyData.power.shift();
          historyData.temp.shift();
          historyData.humidity.shift();
        }
        historyData.voltage.push(data.voltage || 0);
        historyData.current.push(data.current || 0);
        historyData.power.push(data.power || 0);
        historyData.temp.push(data.sht_status ? (data.temperature || 0) : 0);
        historyData.humidity.push(data.sht_status ? (data.humidity || 0) : 0);

        // Render charts
        drawChart('canvasElectrical', historyData.voltage, 'rgb(0, 212, 255)', 'Voltage (V)', historyData.current, 'rgb(16, 185, 129)', 'Current (A)', 20, 28, 0, Math.max(...historyData.current, 2));
        drawChart('canvasClimate', historyData.temp, 'rgb(168, 85, 247)', 'Temp (°C)', historyData.humidity, 'rgb(245, 158, 11)', 'Humidity (%)', 15, 60, 0, 100);
      } catch (err) {
        console.error('Fetch error:', err);
      }
    }

    async function resetEnergy() {
      if (confirm('Are you sure you want to reset accumulated energy (Wh/kWh)?')) {
        await fetch('/api/reset-energy', { method: 'POST' });
        fetchData();
      }
    }

    async function resetVoltageStats() {
      if (confirm('Reset voltage min/max and transient dip counters?')) {
        await fetch('/api/reset-stats', { method: 'POST' });
        fetchData();
      }
    }

    for (let i = 0; i < MAX_POINTS; i++) {
      historyData.voltage.push(24.0);
      historyData.current.push(0.0);
      historyData.power.push(0.0);
      historyData.temp.push(25.0);
      historyData.humidity.push(45.0);
    }

    setInterval(fetchData, 1500);
    window.addEventListener('resize', () => {
      drawChart('canvasElectrical', historyData.voltage, 'rgb(0, 212, 255)', 'Voltage (V)', historyData.current, 'rgb(16, 185, 129)', 'Current (A)', 20, 28, 0, Math.max(...historyData.current, 2));
      drawChart('canvasClimate', historyData.temp, 'rgb(168, 85, 247)', 'Temp (°C)', historyData.humidity, 'rgb(245, 158, 11)', 'Humidity (%)', 15, 60, 0, 100);
    });
    fetchData();
  </script>
</body>
</html>
)rawliteral";

const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Wi-Fi Setup | 24V Power Monitor</title>
  <style>
    :root {
      --bg-base: #0b0f19;
      --bg-card: rgba(18, 26, 44, 0.9);
      --border-card: rgba(255, 255, 255, 0.12);
      --text-main: #f1f5f9;
      --text-muted: #94a3b8;
      --cyan: #00d4ff;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body {
      background: radial-gradient(circle at 50% 0%, #172554 0%, var(--bg-base) 80%);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 1.25rem;
    }
    .setup-box {
      background: var(--bg-card);
      backdrop-filter: blur(16px);
      border: 1px solid var(--border-card);
      border-radius: 20px;
      padding: 2rem;
      max-width: 440px;
      width: 100%;
      box-shadow: 0 20px 40px rgba(0,0,0,0.5);
    }
    .header { text-align: center; margin-bottom: 1.5rem; }
    .header h1 { font-size: 1.4rem; font-weight: 700; margin-bottom: 0.35rem; }
    .header p { font-size: 0.85rem; color: var(--text-muted); }
    .form-group { margin-bottom: 1.25rem; }
    label { display: block; font-size: 0.8rem; font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-muted); margin-bottom: 0.5rem; }
    input, select {
      width: 100%;
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid var(--border-card);
      border-radius: 10px;
      padding: 0.75rem 1rem;
      color: #fff;
      font-size: 0.95rem;
      outline: none;
      transition: border-color 0.2s;
    }
    input:focus, select:focus { border-color: var(--cyan); }
    select option { background: #0b0f19; color: #fff; }
    .btn-submit {
      width: 100%;
      background: linear-gradient(135deg, #00d4ff, #2563eb);
      border: none;
      color: #fff;
      font-weight: 600;
      padding: 0.85rem;
      border-radius: 10px;
      font-size: 0.95rem;
      cursor: pointer;
      margin-top: 0.5rem;
      box-shadow: 0 4px 15px rgba(0, 212, 255, 0.3);
      transition: opacity 0.2s;
    }
    .btn-submit:hover { opacity: 0.9; }
    .status-msg { margin-top: 1rem; text-align: center; font-size: 0.85rem; color: var(--cyan); display: none; }
  </style>
</head>
<body>
  <div class="setup-box">
    <div class="header">
      <div style="width:48px;height:48px;background:linear-gradient(135deg, #00d4ff, #3b82f6);border-radius:14px;display:flex;align-items:center;justify-content:center;margin:0 auto 1rem;box-shadow:0 0 20px rgba(0,212,255,0.4);">
        <svg style="width:24px;height:24px;fill:#fff;" viewBox="0 0 24 24"><path d="M12 4C7.31 4 3.07 5.9 0 8.98L12 21 24 8.98A16.88 16.88 0 0 0 12 4zm0 3.8c3.48 0 6.64 1.35 9.02 3.56L12 18.59 2.98 11.36A12.02 12.02 0 0 1 12 7.8z"/></svg>
      </div>
      <h1>Wi-Fi Network Setup</h1>
      <p>Configure 2.4GHz Wi-Fi for 24V Power Monitor</p>
    </div>

    <form method="POST" action="/save-wifi" id="setupForm">
      <div class="form-group">
        <label for="ssid">Select Network</label>
        <select id="ssidSelect" onchange="onSelectSSID(this.value)">
          <option value="">-- Choose detected network --</option>
          {{NETWORKS}}
        </select>
      </div>

      <div class="form-group">
        <label for="ssidManual">Or Enter SSID Manually</label>
        <input type="text" id="ssid" name="ssid" placeholder="Network SSID" required>
      </div>

      <div class="form-group">
        <label for="password">Wi-Fi Password</label>
        <input type="password" id="password" name="password" placeholder="Enter Wi-Fi Password">
      </div>

      <button type="submit" class="btn-submit" id="btnSubmit">Save & Connect</button>
      <div class="status-msg" id="statusMsg">Saving credentials and restarting monitor...</div>
    </form>
  </div>

  <script>
    function onSelectSSID(val) {
      if (val) {
        document.getElementById('ssid').value = val;
      }
    }
    document.getElementById('setupForm').addEventListener('submit', function() {
      document.getElementById('btnSubmit').disabled = true;
      document.getElementById('statusMsg').style.display = 'block';
    });
  </script>
</body>
</html>
)rawliteral";

#endif

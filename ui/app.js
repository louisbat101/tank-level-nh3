const $ = (sel) => document.querySelector(sel);

function fmtAgo(ms){
  if (!isFinite(ms)) return "--";
  const s = Math.max(0, Math.floor(ms/1000));
  if (s < 60) return `${s}s ago`;
  const m = Math.floor(s/60);
  if (m < 60) return `${m}m ago`;
  const h = Math.floor(m/60);
  if (h < 48) return `${h}h ago`;
  const d = Math.floor(h/24);
  return `${d}d ago`;
}

function timeToEmptyString(hours){
  if (!isFinite(hours)) return "--";
  if (hours < 0) return "--";
  if (hours < 48) return `${hours.toFixed(1)} hours`;
  return `${(hours/24).toFixed(1)} days`;
}

const tankGrid = $("#tankGrid");
const tpl = $("#tankCardTpl");
const subtitle = $("#subtitle");

const cards = new Map();

function upsertTankCard(t){
  let el = cards.get(t.id);
  if (!el){
    el = tpl.content.firstElementChild.cloneNode(true);
    cards.set(t.id, el);
    tankGrid.appendChild(el);
  }

  el.querySelector(".tankname").textContent = t.name || `Tank ${t.id}`;
  el.querySelector(".tankid").textContent = String(t.id);

  const status = el.querySelector(".status");
  status.classList.remove("ok","bad","warn");
  if (t.online){
    status.textContent = "ONLINE";
    status.classList.add("ok");
  } else {
    status.textContent = "OFFLINE";
    status.classList.add("bad");
  }

  el.querySelector(".level").textContent = (t.levelPct ?? 0).toFixed(0);
  el.querySelector(".batpct").textContent = (t.battPct ?? 0).toFixed(0);
  el.querySelector(".batv").textContent = (t.battV ?? 0).toFixed(2);

  const levelFill = el.querySelector(".levelFill");
  levelFill.classList.add("level");
  levelFill.style.width = `${Math.max(0, Math.min(100, t.levelPct ?? 0))}%`;

  const batFill = el.querySelector(".batFill");
  batFill.classList.add("bat");
  batFill.style.width = `${Math.max(0, Math.min(100, t.battPct ?? 0))}%`;

  el.querySelector(".tte").textContent = timeToEmptyString(t.timeToEmptyH);

  const now = Date.now();
  const lastSeenMs = t.lastSeenMs ?? 0;
  el.querySelector(".lastseen").textContent = lastSeenMs ? fmtAgo(now - lastSeenMs) : "--";
}

async function loadOnce(){
  const r = await fetch("/api/state");
  if (!r.ok) throw new Error("state fetch failed");
  const data = await r.json();
  subtitle.textContent = `Gateway: ${data.gatewayName || "Heltec"} • tanks: ${data.tanks?.length || 0}`;
  (data.tanks || []).forEach(upsertTankCard);
}

function connectWs(){
  const proto = location.protocol === "https:" ? "wss" : "ws";
  const ws = new WebSocket(`${proto}://${location.host}/ws`);

  ws.onopen = () => {
    subtitle.textContent = subtitle.textContent.replace("connecting…","connected");
  };

  ws.onmessage = (ev) => {
    try{
      const msg = JSON.parse(ev.data);
      if (msg.type === "state"){
        subtitle.textContent = `Gateway: ${msg.gatewayName || "Heltec"} • tanks: ${msg.tanks?.length || 0}`;
        (msg.tanks || []).forEach(upsertTankCard);
      }
    } catch { /* ignore */ }
  };

  ws.onclose = () => {
    subtitle.textContent = "Gateway: disconnected (retrying…)";
    setTimeout(connectWs, 1000);
  };
}

loadOnce().catch(()=>{});
connectWs();

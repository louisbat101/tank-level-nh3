const $ = (sel) => document.querySelector(sel);

const tankCountSel = $("#tankCount");
const offlineTimeoutS = $("#offlineTimeoutS");
const loraKey = $("#loraKey");
const tankList = $("#tankList");
const saveBtn = $("#saveBtn");
const saveStatus = $("#saveStatus");
const rowTpl = $("#tankRowTpl");

for (let i=1;i<=8;i++){
	const opt = document.createElement("option");
	opt.value = String(i);
	opt.textContent = String(i);
	tankCountSel.appendChild(opt);
}

function renderTankRows(count, current){
	tankList.innerHTML = "";
	for (let i=1;i<=count;i++){
		const row = rowTpl.content.firstElementChild.cloneNode(true);
		row.dataset.id = String(i);
		const c = (current || []).find(t => t.id === i) || {};
		row.querySelector(".name").value = c.name || `Tank ${i}`;
		row.querySelector(".capacityL").value = c.capacityL ?? 0;
		row.querySelector(".usePerDayL").value = c.usePerDayL ?? 0;
		tankList.appendChild(row);
	}
}

async function loadConfig(){
	const r = await fetch("/api/config");
	if (!r.ok) throw new Error("config fetch failed");
	const cfg = await r.json();
	tankCountSel.value = String(cfg.tankCount || 2);
	offlineTimeoutS.value = String(cfg.offlineTimeoutS || 30);
	loraKey.value = cfg.loraKey || "";
	renderTankRows(Number(tankCountSel.value), cfg.tanks || []);
}

async function saveConfig(){
	const count = Number(tankCountSel.value);
	const tanks = Array.from(tankList.querySelectorAll(".tankRow")).slice(0,count).map(row => {
		const id = Number(row.dataset.id);
		return {
			id,
			name: row.querySelector(".name").value,
			capacityL: Number(row.querySelector(".capacityL").value || 0),
			usePerDayL: Number(row.querySelector(".usePerDayL").value || 0),
		};
	});

	const body = {
		tankCount: count,
		offlineTimeoutS: Number(offlineTimeoutS.value || 30),
		loraKey: loraKey.value || "",
		tanks,
	};

	saveStatus.textContent = "Saving…";
	const r = await fetch("/api/config", {
		method: "POST",
		headers: {"content-type":"application/json"},
		body: JSON.stringify(body)
	});
	if (!r.ok){
		saveStatus.textContent = "Save failed";
		return;
	}
	saveStatus.textContent = "Saved";
}

tankCountSel.addEventListener("change", ()=>{
	renderTankRows(Number(tankCountSel.value));
});

saveBtn.addEventListener("click", ()=>{
	saveConfig().catch(()=>{ saveStatus.textContent = "Save failed"; });
});

loadConfig().catch(()=>{});

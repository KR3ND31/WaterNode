let selectedPeriod = '24h';
let refreshInterval = 5000;
let refreshTimer = null;

let smoothingMode = 'average';

let currentChartInstance = null;
let currentChartAddress = null;

let currentSettingsAddress = null;

async function renderDeviceCards() {
    const res = await fetch("/devices");
    const devices = await res.json();

    const container = document.getElementById("device-cards");

    devices.forEach(device => {
        const moisture = device.moisture ?? 0;
        const status = device.status === "online" ? "On" : "Closed";
        const barColor = moisture > 60 ? "#4caf50" : moisture > 30 ? "#ff9800" : "#ff4444";
        const valveIcon = device.status === "online"
            ? '<span class="valve-icon text-success me-1">🟢</span>'
            : '<span class="valve-icon text-danger me-1">🔴</span>';
        const lastWatered = device.last_watered ? new Date(device.last_watered).toLocaleString() : '—';
        const dangerClass = moisture < 30 ? "device-card-low" : "";

        let wrapper = container.querySelector(`.device-wrapper[data-address="${device.address}"]`);

        if (wrapper) {
            // Обновляем существующую карточку
            const card = wrapper.querySelector(".device-card");

            card.querySelector(".device-title").textContent = `${device.name || `Device ${device.address}`}`;
            card.querySelector(".device-moisture").textContent = `${moisture}%`;
            card.querySelector(".device-status").innerHTML = `${valveIcon}${status}`;

            const progressBar = card.querySelector(".progress-bar");
            progressBar.style.width = `${moisture}%`;
            progressBar.style.backgroundColor = barColor;

            card.className = `device-card ${dangerClass}`;
        } else {
            // Создаём новую карточку
            wrapper = document.createElement("div");
            wrapper.className = "col-12 col-sm-6 col-md-4 col-lg-3 device-wrapper";
            wrapper.dataset.address = device.address;

            const card = document.createElement("div");
            card.className = `device-card ${dangerClass}`;
            card.dataset.address = device.address;

            const graphBtn = `<button class="btn btn-sm btn-icon btn-graph" title="Show Graph" onclick="showDeviceChart('${device.address}'); event.stopPropagation();">
                <i class="bi bi-graph-up"></i>
            </button>`;

            const settingsBtn = `<button class="btn btn-sm btn-icon btn-settings" title="Settings" onclick="showDeviceSettings('${device.address}'); event.stopPropagation();">
                <i class="bi bi-gear-fill"></i>
            </button>`;

            card.innerHTML = `
                <div class="device-header d-flex justify-content-between align-items-start">
                    <div class="device-title">${device.name || `Device ${device.address}`}</div>
                    <div class="device-actions">${graphBtn}${settingsBtn}</div>
                </div>
                <div class="device-moisture">${moisture}%</div>
                <div class="progress-container">
                    <div class="progress-bar" style="width: ${moisture}%; background-color: ${barColor};"></div>
                </div>
                <div class="device-status">${valveIcon}${status}</div>
                <div class="device-last-watered">Last watered: ${lastWatered}</div>
            `;

            // Добавляем кнопку отдельно
            const btn = document.createElement("button");
            btn.className = "btn-water";
            btn.textContent = "WATER NOW";
            btn.addEventListener("click", (e) => {
                toggleValve(device.address, true);
            });
            card.appendChild(btn);

            wrapper.appendChild(card);
            container.appendChild(wrapper);
        }
    });
}

function applyChartData(chart, readings) {
    const processed = getProcessedReadings(readings);
    const timestamps = processed.map(r => new Date(r.t));
    const values = processed.map(r => r.v);

    chart.data.labels = timestamps;
    chart.data.datasets[0].data = values;
    chart.update();
}

async function fetchDeviceReadings(address, period = selectedPeriod) {
    try {
        const res = await fetch(`/history/${address}?period=${period}`);
        const device = await res.json();

        if (!device?.readings?.length) {
            return null;
        }

        return getProcessedReadings(device.readings);
    } catch (err) {
        console.error("Ошибка загрузки данных устройства:", err);
        return null;
    }
}

async function showDeviceChart(address, showModal = true) {
    const readings = await fetchDeviceReadings(address);
    if (!readings) {
        alert("Нет данных");
        return;
    }

    currentChartAddress = address;

    // Показываем модальное окно
    if (showModal) {
        const modalEl = document.getElementById("chartModal");
        const modalInstance = new bootstrap.Modal(modalEl);
        modalInstance.show();
    }

    const ctx = document.getElementById("chartCanvas").getContext("2d");

    if (!currentChartInstance) {
        // Создаём новый график
        currentChartInstance = new Chart(ctx, {
            type: "line",
            data: {
                datasets: [{
                    data: [],
                    borderColor: "#4dd0e1",
                    backgroundColor: "rgba(77,208,225,0.1)",
                    tension: 0.3,
                    pointRadius: 2,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: getTimeScaleOptions(selectedPeriod),
                    y: {
                        beginAtZero: true,
                        max: 100,
                        ticks: { color: "#ccc" },
                        grid: { color: "rgba(255,255,255,0.1)" }
                    }
                },
                plugins: {
                    zoom: {
                        pan: { enabled: true, mode: 'x' },
                        zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' }
                    },
                    legend: { display: false },
                    tooltip: {
                        backgroundColor: "#222",
                        borderColor: "#4dd0e1",
                        borderWidth: 1,
                        titleColor: "#fff",
                        bodyColor: "#fff"
                    }
                }
            }
        });
    } else {
        // Обновляем ось X при смене периода
        currentChartInstance.options.scales.x = getTimeScaleOptions(selectedPeriod);
    }

    applyChartData(currentChartInstance, readings);
}

async function updateCurrentChart() {
    if (!currentChartAddress || !currentChartInstance) return;

    const readings = await fetchDeviceReadings(currentChartAddress);
    if (!readings) return;

    applyChartData(currentChartInstance, readings);
}

function getProcessedReadings(readings) {
    const MAX_POINTS = 500;

    if (readings.length <= MAX_POINTS) return readings;
    if (smoothingMode === "average") return downsampleAvg(readings, MAX_POINTS);
    if (smoothingMode === "median") return downsampleMedian(readings, MAX_POINTS);

    const step = Math.ceil(readings.length / MAX_POINTS);
    return readings.filter((_, i) => i % step === 0);
}

function getTimeScaleOptions(period) {
    const base = {
        type: 'time',
        ticks: {
            color: '#aaa',
            autoSkip: true,
            maxTicksLimit: 20
        },
        grid: {
            color: 'rgba(255,255,255,0.05)'
        }
    };

    if (period === '1h') {
        base.time = {
            unit: 'minute',
            stepSize: 5,
            displayFormats: { minute: 'HH:mm' }
        };
    } else if (period === '24h') {
        base.time = {
            unit: 'hour',
            stepSize: 1,
            displayFormats: { hour: 'HH:mm' }
        };
    } else if (period === '7d') {
        base.time = {
            unit: 'hour',
            stepSize: 6,
            displayFormats: { hour: 'dd.MM HH' }
        };
    } else if (period === '30d') {
        base.time = {
            unit: 'day',
            stepSize: 1,
            displayFormats: { day: 'dd MMM' }
        };
    } else {
        base.time = {
            unit: 'minute',
            stepSize: 10,
            displayFormats: { minute: 'HH:mm' }
        };
    }

    return base;
}

function downsampleAvg(data, maxPoints = 100) {
    if (data.length <= maxPoints) return data;
    const result = [];
    const step = Math.ceil(data.length / maxPoints);
    for (let i = 0; i < data.length; i += step) {
        const chunk = data.slice(i, i + step);
        const avg = chunk.reduce((sum, v) => sum + v.v, 0) / chunk.length;
        const t = chunk[Math.floor(chunk.length / 2)].t;
        result.push({ t, v: avg });
    }
    return result;
}

function downsampleMedian(data, maxPoints = 100) {
    if (data.length <= maxPoints) return data;
    const result = [];
    const step = Math.ceil(data.length / maxPoints);
    for (let i = 0; i < data.length; i += step) {
        const chunk = data.slice(i, i + step);
        const sorted = chunk.map(r => r.v).sort((a, b) => a - b);
        const median = sorted[Math.floor(sorted.length / 2)];
        const t = chunk[Math.floor(chunk.length / 2)].t;
        result.push({ t, v: median });
    }
    return result;
}

function resetZoom() {
    if (currentChartInstance && currentChartInstance.resetZoom) {
        currentChartInstance.resetZoom();
    }
}

function changePeriod(period) {
    selectedPeriod = period;

    // Обновляем стили кнопок
    document.querySelectorAll('[id^="btn-"]').forEach(btn => {
        btn.classList.remove("active");
    });

    const activeBtn = document.getElementById(`btn-${period}`);
    if (activeBtn) {
        activeBtn.classList.add("active");
    }

    if (currentChartAddress) {
        showDeviceChart(currentChartAddress, false);
    }
}

async function showDeviceSettings(address) {
    const res = await fetch(`/device/${address}`);
    const device = await res.json();

    currentSettingsDevice = address;

    const modal = document.getElementById("settingsModal");

    // Заполняем значения
    modal.querySelector("#device-name").value = device.name || "";

    modal.querySelector("#settings-title").textContent = device.name || `Device ${address}`;
    modal.querySelector("#raw-moisture").textContent = device.raw_moisture ?? "—";

    const minInput = modal.querySelector("#input-calib-min");
    const maxInput = modal.querySelector("#input-calib-max");

    minInput.value = device.calib_min ?? "";
    maxInput.value = device.calib_max ?? "";

    // Устанавливаем кнопки "Запомнить как 0%" и "100%"
    modal.querySelector("#btn-remember-min").onclick = () => {
        minInput.value = device.raw_moisture;
    };
    modal.querySelector("#btn-remember-max").onclick = () => {
        maxInput.value = device.raw_moisture;
    };

    // Обработчик кнопки "Сохранить"
    modal.querySelector("#btn-save-calibration").onclick = async () => {
        const body = {
            name: modal.querySelector("#device-name").value,
            calib_min: parseInt(minInput.value),
            calib_max: parseInt(maxInput.value),
        };
        await fetch(`/device/${address}/calibrate`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(body),
        });
        bootstrap.Modal.getInstance(modal).hide();
        currentSettingsDevice = null;
        await renderDeviceCards(); // Обновим интерфейс
    };

    modal.querySelector("#btn-rename-device").onclick = async () => {
        const newName = modal.querySelector("#device-name").value.trim();
        if (!newName) return;

        await fetch(`/device/${address}/rename`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ name: newName })
        });

        await renderDeviceCards(); // обновить отображение
        await updateSettingsModal();
    };

    // При закрытии — сбрасываем текущую модалку
    modal.addEventListener("hidden.bs.modal", () => {
        currentSettingsDevice = null;
    }, { once: true });

    bootstrap.Modal.getOrCreateInstance(modal).show();
}

async function updateSettingsModal() {
    if (!currentSettingsDevice) return;

    const res = await fetch(`/device/${currentSettingsDevice}`);
    const device = await res.json();

    const modal = document.getElementById("settingsModal");

    modal.querySelector("#settings-title").textContent = device.name || `Device ${address}`;

    document.getElementById("raw-moisture").textContent = device.raw_moisture ?? '—';

    // Обновим значения, если поля пустые
    const minInput = modal.querySelector("#input-calib-min");
    const maxInput = modal.querySelector("#input-calib-max");

    modal.querySelector("#btn-remember-min").onclick = () => {
        minInput.value = device.raw_moisture ?? "";
    };
    modal.querySelector("#btn-remember-max").onclick = () => {
        maxInput.value = device.raw_moisture ?? "";
    };
}

async function calibrateDevice(address, type) {
    const res = await fetch(`/device/${address}`);
    const device = await res.json();
    const raw = device.raw_moisture;

    const body = {};
    if (type === "min") body.calib_min = raw;
    if (type === "max") body.calib_max = raw;

    const resp = await fetch(`/device/${address}/calibrate`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body)
    });

    if (resp.ok) {
        alert(`Калибровка ${type === "min" ? "0%" : "100%"} сохранена`);
    } else {
        const err = await resp.json();
        alert("Ошибка: " + (err.error || "Неизвестно"));
    }
}

async function toggleValve(address, on) {
    await fetch(`/valve/${address}`, {
        method: "POST",
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: on ? "on" : "off" })
    });
    await renderDeviceCards();
}

async function discoverDevices() {
    const res = await fetch("/autodiscover_bulk", { method: "POST" });
    const json = await res.json();
    alert("Найдено: " + json.new_devices.join(", "));
    await renderDeviceCards();
}

document.getElementById("smoothing-mode").addEventListener("change", (e) => {
    smoothingMode = e.target.value;
    if (currentChartAddress) showDeviceChart(currentChartAddress, false);
});

document.getElementById("update-interval").addEventListener("change", (e) => {
    refreshInterval = parseInt(e.target.value) * 1000;
    startAutoUpdate();
});

function startAutoUpdate() {
    clearInterval(refreshTimer);
    refreshTimer = setInterval(async () => {
        await renderDeviceCards();
        if (currentChartAddress) {
            await updateCurrentChart();
        }
        if (currentSettingsDevice) {
            await updateSettingsModal();
        }
    }, refreshInterval);
}

renderDeviceCards();
changePeriod(selectedPeriod);
startAutoUpdate();
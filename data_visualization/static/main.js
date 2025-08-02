// Indoor Air Quality Monitor JavaScript

// Update date and time
function updateDateTime() {
    const now = new Date();
    // 24-hour format
    const options = { 
        weekday: 'long', 
        year: 'numeric', 
        month: 'long', 
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
        hour12: false
    };
    document.getElementById('currentDateTime').textContent = now.toLocaleDateString('en-GB', options);
}

// Tab switching
function showTab(tabName) {
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
    });
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    
    document.getElementById(tabName).classList.add('active');
    event.target.classList.add('active');
    
    if (tabName === 'history') {
        initCharts();
    }
}

// Initialize charts with real data
async function initCharts() {
    try {
        const response = await fetch('/api/historical-data');
        const data = await response.json();
        
        if (data.error) {
            console.error('No historical data available');
            return;
        }
        
        const timeLabels = data.timestamps;
        
        // CO2 Chart
        const co2Ctx = document.getElementById('co2Chart').getContext('2d');
        new Chart(co2Ctx, {
            type: 'line',
            data: {
                labels: timeLabels,
                datasets: [{
                    label: 'CO₂ (ppm)',
                    data: data.co2,
                    borderColor: '#FF6384',
                    backgroundColor: 'rgba(255, 99, 132, 0.1)',
                    fill: true,
                    tension: 0.4
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: { display: false }
                },
                scales: {
                    y: {
                        beginAtZero: false
                    }
                }
            }
        });

        // PM Chart
        const pmCtx = document.getElementById('pmChart').getContext('2d');
        new Chart(pmCtx, {
            type: 'line',
            data: {
                labels: timeLabels,
                datasets: [
                    {
                        label: 'PM1.0',
                        data: data.pm1,
                        borderColor: '#36A2EB',
                        backgroundColor: 'rgba(54, 162, 235, 0.1)',
                    },
                    {
                        label: 'PM2.5',
                        data: data.pm25,
                        borderColor: '#FFCE56',
                        backgroundColor: 'rgba(255, 206, 86, 0.1)',
                    },
                    {
                        label: 'PM10',
                        data: data.pm10,
                        borderColor: '#4BC0C0',
                        backgroundColor: 'rgba(75, 192, 192, 0.1)',
                    }
                ]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: { position: 'top' }
                }
            }
        });

        // TVOC Chart
        const tvocCtx = document.getElementById('tvocChart').getContext('2d');
        new Chart(tvocCtx, {
            type: 'bar',
            data: {
                labels: timeLabels,
                datasets: [{
                    label: 'TVOC (ppb)',
                    data: data.tvoc,
                    backgroundColor: 'rgba(153, 102, 255, 0.6)',
                    borderColor: 'rgba(153, 102, 255, 1)',
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: { display: false }
                }
            }
        });

        // Temperature & Humidity Chart
        const tempHumCtx = document.getElementById('tempHumChart').getContext('2d');
        new Chart(tempHumCtx, {
            type: 'line',
            data: {
                labels: timeLabels,
                datasets: [
                    {
                        label: 'Temperature (°C)',
                        data: data.temperature,
                        borderColor: '#FF6384',
                        backgroundColor: 'rgba(255, 99, 132, 0.1)',
                        yAxisID: 'y'
                    },
                    {
                        label: 'Humidity (%)',
                        data: data.humidity,
                        borderColor: '#36A2EB',
                        backgroundColor: 'rgba(54, 162, 235, 0.1)',
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: { position: 'top' }
                },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        grid: {
                            drawOnChartArea: false,
                        },
                    }
                }
            }
        });
        
        // Load insights
        await loadInsights();
        
    } catch (error) {
        console.error('Error loading historical data:', error);
    }
}

// Load insights data
async function loadInsights() {
    try {
        const response = await fetch('/api/insights');
        const insights = await response.json();
        if (insights.error) {
            console.error('No insights data available');
            return;
        }
        // Card definitions for each parameter
        const paramCards = [
            {
                key: 'co2',
                icon: '📊',
                title: 'CO₂',
                unit: 'ppm',
                thresholdLabel: 'Threshold',
            },
            {
                key: 'pm1',
                icon: '🌫️',
                title: 'PM 1.0',
                unit: 'µg/m³',
                thresholdLabel: 'Threshold',
            },
            {
                key: 'pm25',
                icon: '🌪️',
                title: 'PM 2.5',
                unit: 'µg/m³',
                thresholdLabel: 'Threshold',
            },
            {
                key: 'pm10',
                icon: '💨',
                title: 'PM 10',
                unit: 'µg/m³',
                thresholdLabel: 'Threshold',
            },
            {
                key: 'tvoc',
                icon: '🧪',
                title: 'TVOC',
                unit: 'ppb',
                thresholdLabel: 'Threshold',
            },
            {
                key: 'temperature',
                icon: '🌡️',
                title: 'Temperature',
                unit: '°C',
                thresholdLabel: 'Comfort Range',
            },
            {
                key: 'humidity',
                icon: '💧',
                title: 'Humidity',
                unit: '%',
                thresholdLabel: 'Comfort Range',
            },
        ];
        const grid = document.getElementById('insightsGrid');
        grid.innerHTML = '';
        paramCards.forEach(card => {
            const d = insights[card.key];
            if (!d) return;
            let thresholdInfo = '';
            if (card.key === 'temperature') {
                thresholdInfo = `<strong>${card.thresholdLabel}:</strong> ${d.threshold_low}–${d.threshold_high} ${card.unit}`;
            } else if (card.key === 'humidity') {
                thresholdInfo = `<strong>${card.thresholdLabel}:</strong> ${d.threshold_low}–${d.threshold_high} ${card.unit}`;
            } else {
                thresholdInfo = `<strong>${card.thresholdLabel}:</strong> ${d.threshold} ${card.unit}`;
            }
            grid.innerHTML += `
            <div class="insight-card">
                <div class="insight-title">${card.icon} ${card.title} Analysis</div>
                <div class="peak-info">
                    <div><strong>Max:</strong> ${d.max} <span style="color:#888;">at ${d.max_time}</span></div>
                    <div><strong>Min:</strong> ${d.min} <span style="color:#888;">at ${d.min_time}</span></div>
                    <div><strong>Average:</strong> ${d.avg}</div>
                    ${thresholdInfo}<br>
                    <strong>Threshold Exceeded:</strong> ${d.threshold_exceeded} times
                </div>
            </div>
            `;
        });
    } catch (error) {
        console.error('Error loading insights:', error);
    }
}

// Update connection status display
function updateConnectionStatus(status, lastUpdate) {
    const statusElement = document.getElementById('connectionStatus');
    const statusTextElement = document.getElementById('statusText');
    const lastUpdateElement = document.getElementById('lastUpdateCO2');
    
    if (status === 'Connected') {
        statusElement.className = 'status-indicator status-connected';
        statusTextElement.textContent = 'Connected (Real-time)';
    } else {
        statusElement.className = 'status-indicator status-disconnected';
        statusTextElement.textContent = 'Disconnected';
    }
    
    if (lastUpdate) {
        lastUpdateElement.textContent = lastUpdate;
    } else {
        lastUpdateElement.textContent = 'Never';
    }
}

// Fetch real-time data from backend
async function fetchRealTimeData() {
    try {
        const response = await fetch('/api/current-data');
        const data = await response.json();
        
        // Update connection status
        updateConnectionStatus(data.connection_status, data.last_update);
        
        // Update separate last update times
        document.getElementById('lastUpdateCO2').textContent = data.last_update_co2 || '-';
        document.getElementById('lastUpdatePM').textContent = data.last_update_pm || '-';

        // Update sensor values (handle both string and number values)
        document.getElementById('co2Value').textContent = data.co2;
        document.getElementById('tvocValue').textContent = data.tvoc;
        document.getElementById('eco2Value').textContent = data.eco2;
        
        // Round PM values to 2 decimal places
        const pm1Value = data.pm1;
        const pm25Value = data.pm25;
        const pm10Value = data.pm10;
        const tempValue = data.temperature;
        const humidityValue = data.humidity;
        
        document.getElementById('pm1Value').textContent = pm1Value === '-' ? '-' : parseFloat(pm1Value).toFixed(2);
        document.getElementById('pm25Value').textContent = pm25Value === '-' ? '-' : parseFloat(pm25Value).toFixed(2);
        document.getElementById('pm10Value').textContent = pm10Value === '-' ? '-' : parseFloat(pm10Value).toFixed(2);
        document.getElementById('tempValue').textContent = tempValue === '-' ? '-' : parseFloat(tempValue).toFixed(2);
        document.getElementById('humidityValue').textContent = humidityValue === '-' ? '-' : parseFloat(humidityValue).toFixed(2);
        
        // Update all arc gauges
        updateAllArcGauges(data);
        
        // Update AQI
        const aqiElement = document.getElementById('aqiValue');
        const aqiLabelElement = document.getElementById('aqiLabel');
        const aqiDescElement = document.getElementById('aqiDescription');
        const affectedGroupsElement = document.getElementById('affectedGroups');
        const affectedGroupsListElement = document.getElementById('affectedGroupsList');
        
        if (data.air_quality_status && data.air_quality_status.status && data.air_quality_status.status !== '-') {
            const status = data.air_quality_status;
            aqiElement.textContent = status.icon;
            aqiLabelElement.textContent = status.status;
            aqiLabelElement.className = `aqi-label ${status.color}`;
            aqiDescElement.textContent = status.description;
            
            // Show affected groups if any
            if (status.affected_groups && status.affected_groups.length > 0) {
                affectedGroupsElement.style.display = 'block';
                affectedGroupsListElement.innerHTML = status.affected_groups.map(group => 
                    `<div style="margin-bottom: 5px; padding: 5px; background: rgba(255,193,7,0.1); border-radius: 3px;">• ${group}</div>`
                ).join('');
            } else {
                affectedGroupsElement.style.display = 'none';
            }
        } else {
            aqiElement.textContent = '-';
            aqiLabelElement.textContent = '-';
            aqiLabelElement.className = 'aqi-label';
            aqiDescElement.textContent = '-';
            affectedGroupsElement.style.display = 'none';
        }
        
        // Update recommendations
        const recommendationsContainer = document.querySelector('.recommendations');
        let recommendationsHTML = `<h3>💡 Recommendations</h3>`;
        if (data.recommendations && data.recommendations.length === 1 && data.recommendations[0] === '-') {
            recommendationsHTML += `<div class="recommendation">-</div>`;
        } else {
            recommendationsHTML += data.recommendations.map(rec => `<div class="recommendation">${rec}</div>`).join('');
        }
        recommendationsContainer.innerHTML = recommendationsHTML;
        
    } catch (error) {
        console.error('Error fetching real-time data:', error);
        // Update status to show connection error
        updateConnectionStatus('Disconnected', 'Error');
    }
}

function updateAllArcGauges(data) {
    // CO₂
    if (data.co2 !== '-' && !isNaN(data.co2)) {
        const co2Thresholds = [
            { limit: 800, color: '#4CAF50' },
            { limit: 1000, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ];
        let statusText = "Good";
        if (data.co2 > 1000) statusText = "Very High";
        else if (data.co2 > 800) statusText = "Elevated";
        else statusText = "Good";
        setGaugeArc('co2', parseFloat(data.co2), 400, 2000, co2Thresholds, statusText);
    } else {
        setGaugeArc('co2', '-', 400, 2000, [
            { limit: 800, color: '#4CAF50' },
            { limit: 1000, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ], '-');
    }
    // TVOC
    if (data.tvoc !== '-' && !isNaN(data.tvoc)) {
        const tvocThresholds = [
            { limit: 250, color: '#4CAF50' },
            { limit: 500, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ];
        let statusText = "Good";
        if (data.tvoc > 500) statusText = "Very High";
        else if (data.tvoc > 250) statusText = "Elevated";
        else statusText = "Good";
        setGaugeArc('tvoc', parseFloat(data.tvoc), 0, 1000, tvocThresholds, statusText);
    } else {
        setGaugeArc('tvoc', '-', 0, 1000, [
            { limit: 250, color: '#4CAF50' },
            { limit: 500, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ], '-');
    }
    // eCO₂
    if (data.eco2 !== '-' && !isNaN(data.eco2)) {
        const eco2Thresholds = [
            { limit: 800, color: '#4CAF50' },
            { limit: 1000, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ];
        let statusText = "Good";
        if (data.eco2 > 1000) statusText = "Very High";
        else if (data.eco2 > 800) statusText = "Elevated";
        else statusText = "Good";
        setGaugeArc('eco2', parseFloat(data.eco2), 400, 2000, eco2Thresholds, statusText);
    } else {
        setGaugeArc('eco2', '-', 400, 2000, [
            { limit: 800, color: '#4CAF50' },
            { limit: 1000, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ], '-');
    }
    // PM1.0
    if (data.pm1 !== '-' && !isNaN(data.pm1)) {
        const pm1Thresholds = [
            { limit: 10, color: '#4CAF50' },
            { limit: 25, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ];
        let statusText = "Good";
        if (data.pm1 > 25) statusText = "Very High";
        else if (data.pm1 > 10) statusText = "Elevated";
        else statusText = "Good";
        setGaugeArc('pm1', parseFloat(data.pm1), 0, 50, pm1Thresholds, statusText);
    } else {
        setGaugeArc('pm1', '-', 0, 50, [
            { limit: 10, color: '#4CAF50' },
            { limit: 25, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ], '-');
    }
    // PM2.5
    if (data.pm25 !== '-' && !isNaN(data.pm25)) {
        const pm25Thresholds = [
            { limit: 25, color: '#4CAF50' },
            { limit: 35, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ];
        let statusText = "Good";
        if (data.pm25 > 35) statusText = "Very High";
        else if (data.pm25 > 25) statusText = "Elevated";
        else statusText = "Good";
        setGaugeArc('pm25', parseFloat(data.pm25), 0, 100, pm25Thresholds, statusText);
    } else {
        setGaugeArc('pm25', '-', 0, 100, [
            { limit: 25, color: '#4CAF50' },
            { limit: 35, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ], '-');
    }
    // PM10
    if (data.pm10 !== '-' && !isNaN(data.pm10)) {
        const pm10Thresholds = [
            { limit: 45, color: '#4CAF50' },
            { limit: 100, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ];
        let statusText = "Good";
        if (data.pm10 > 100) statusText = "Very High";
        else if (data.pm10 > 45) statusText = "Elevated";
        else statusText = "Good";
        setGaugeArc('pm10', parseFloat(data.pm10), 0, 200, pm10Thresholds, statusText);
    } else {
        setGaugeArc('pm10', '-', 0, 200, [
            { limit: 45, color: '#4CAF50' },
            { limit: 100, color: '#FFC107' },
            { limit: Infinity, color: '#FF5722' }
        ], '-');
    }
}

function updateModernGaugeNeedle(needleId, value, min, max) {
    const needle = document.getElementById(needleId);
    const percentage = Math.min(Math.max(((value - min) / (max - min)) * 100, 0), 100);
    const rotation = (percentage / 100) * 270 - 135; // 270 degrees total, starting at -135
    needle.style.transform = `rotate(${rotation}deg)`;
}

function polarToCartesian(cx, cy, r, deg) {
    const rad = (deg-90) * Math.PI / 180.0;
    return {
        x: cx + (r * Math.cos(rad)),
        y: cy + (r * Math.sin(rad))
    };
}

function setGaugeArc(id, value, min, max, thresholds, statusText) {
    // For large gauge: r=78, cx=90, cy=90, arc -135 to +135 (270deg)
    if (value === '-' || isNaN(value)) {
        // Reset arc and knob
        const arc = document.getElementById(id + 'Arc');
        arc.setAttribute('d', '');
        arc.setAttribute('stroke', '#eee');
        const knob = document.getElementById(id + 'Knob');
        knob.setAttribute('cx', 90);
        knob.setAttribute('cy', 90);
        knob.setAttribute('stroke', '#eee');
        knob.setAttribute('fill', '#eee');
        document.getElementById(id + 'Value').textContent = '-';
        if (statusText) document.getElementById(id + 'StatusLabel').textContent = statusText;
        return;
    }
    const percent = Math.max(0, Math.min(1, (value - min) / (max - min)));
    const angle = percent * 270 - 135;
    const r = 78, cx = 90, cy = 90;
    const start = polarToCartesian(cx, cy, r, -135);
    const end = polarToCartesian(cx, cy, r, angle);
    const largeArc = percent > 0.5 ? 1 : 0;
    const arcPath = [
        'M', start.x, start.y,
        'A', r, r, 0, largeArc, 1, end.x, end.y
    ].join(' ');
    // Color logic
    let color = thresholds[0].color;
    for (let t of thresholds) {
        if (value > t.limit) color = t.color;
        else break;
    }
    // Set arc
    const arc = document.getElementById(id + 'Arc');
    arc.setAttribute('d', arcPath);
    arc.setAttribute('stroke', color);
    // Set knob
    const knob = document.getElementById(id + 'Knob');
    knob.setAttribute('cx', end.x);
    knob.setAttribute('cy', end.y);
    knob.setAttribute('stroke', color);
    knob.setAttribute('fill', color);
    // Set value
    document.getElementById(id + 'Value').textContent = value === '-' ? '-' : parseFloat(value).toFixed(0);
    // Set status label (e.g., "Very High")
    if (statusText) {
        document.getElementById(id + 'StatusLabel').textContent = statusText;
    }
}

// Initialize
updateDateTime();
setInterval(updateDateTime, 1000);
fetchRealTimeData(); // Initial load
setInterval(fetchRealTimeData, 5000); // Update every 5 seconds 
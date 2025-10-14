document.addEventListener('DOMContentLoaded', () => {
    // --- Element References ---
    const tankForm = document.getElementById('tank-form');
    const systemForm = document.getElementById('system-form');
    const networkForm = document.getElementById('network-form');
    const statusMessage = document.getElementById('status-message');

    const tankAreaInput = document.getElementById('tank-area');
    const tankDepthInput = document.getElementById('tank-depth');
    const tankVolumeInput = document.getElementById('tank-volume');

    // --- Fetch initial data from ESP32 ---
    fetch('/api/get-configs')
        .then(response => response.json())
        .then(data => {
            // Populate Tank Form
            document.getElementById('tank-area').value = data.tankCrossSectionAreaCm2;
            document.getElementById('tank-depth').value = data.tankDepthCm;
            document.getElementById('sensor-offset').value = data.sensorOffsetCm;
            document.getElementById('error-margin').value = data.heightErrorMarginCm;
            updateTankVolume();

            // Populate System Form
            document.getElementById('measure-interval').value = data.measurementInterval;
            document.getElementById('response-timeout').value = data.responseTimeout;

            // Populate Network Form
            document.getElementById('ap-ssid').value = data.ssid;
            document.getElementById('ap-password').value = data.password;
        })
        .catch(error => {
            console.error('Error fetching configs:', error);
            showStatus('Error loading configurations!', 'error');
        });

    // --- Tank Volume Calculation ---
    function updateTankVolume() {
        const area = parseFloat(tankAreaInput.value) || 0;
        const depth = parseFloat(tankDepthInput.value) || 0;
        const volumeCm3 = area * depth;
        const volumeLitres = volumeCm3 * 0.001;
        tankVolumeInput.value = volumeLitres.toFixed(2);
    }
    tankAreaInput.addEventListener('input', updateTankVolume);
    tankDepthInput.addEventListener('input', updateTankVolume);

    // --- Form Submit Handlers ---
    tankForm.addEventListener('submit', (e) => {
        e.preventDefault();
        const data = {
            tankCrossSectionAreaCm2: parseFloat(document.getElementById('tank-area').value),
            tankDepthCm: parseFloat(document.getElementById('tank-depth').value),
            sensorOffsetCm: parseFloat(document.getElementById('sensor-offset').value),
            heightErrorMarginCm: parseFloat(document.getElementById('error-margin').value)
        };
        postData('/api/update-tank', data);
    });

    systemForm.addEventListener('submit', (e) => {
        e.preventDefault();
        const data = {
            measurementInterval: parseInt(document.getElementById('measure-interval').value),
            responseTimeout: parseInt(document.getElementById('response-timeout').value)
        };
        postData('/api/update-system', data);
    });

    networkForm.addEventListener('submit', (e) => {
        e.preventDefault();
        const data = {
            ssid: document.getElementById('ap-ssid').value,
            password: document.getElementById('ap-password').value
        };
        postData('/api/update-network', data);
    });

    // --- Helper functions ---
    function postData(url, data) {
        fetch(url, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(data)
        })
        .then(response => response.json())
        .then(result => {
            showStatus(result.message, result.status);
        })
        .catch(error => {
            console.error(`Error updating ${url}:`, error);
            showStatus('An error occurred.', 'error');
        });
    }

    let statusTimeout;
    function showStatus(message, type) {
        clearTimeout(statusTimeout);
        statusMessage.textContent = message;
        statusMessage.className = type; // 'success' or 'error'
        statusTimeout = setTimeout(() => {
            statusMessage.className = '';
        }, 4000);
    }
});

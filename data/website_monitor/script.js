document.addEventListener('DOMContentLoaded', () => {
    // --- WebSocket Connection ---
    const gateway = `ws://${window.location.hostname}/ws`;
    let websocket;

    function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen = () => console.log('Connection opened');
        websocket.onclose = () => {
            console.log('Connection closed');
            setTimeout(initWebSocket, 2000); // Retry connection after 2 seconds
        };
        websocket.onmessage = (event) => {
            const data = JSON.parse(event.data);
            console.log(data);
            updateMonitorUI(data);
        };
    }

    initWebSocket();

    // --- Update UI with WebSocket Data ---
    function updateMonitorUI(data) {
        document.getElementById('water-level').style.height = `${data.percentage.toFixed(1)}%`;
        document.getElementById('percentage').textContent = `${data.percentage.toFixed(1)}%`;
        document.getElementById('volume').textContent = `${data.volume.toFixed(2)} L`;
        document.getElementById('depth').textContent = `${data.depth.toFixed(2)} cm`;
    }

    // --- Tank Parameter Form Handling ---
    const tankConfig = {
        name: 'tank',
        containerId: 'tank-fields',
        formId: 'tank-form'
    };

    loadAndRenderConfig(tankConfig.name, tankConfig.containerId);
    setupFormSubmit(tankConfig.name, tankConfig.formId);

    async function loadAndRenderConfig(configName, containerId) {
        try {
            const response = await fetch(`/api/${configName}`);
            const data = await response.json();
            const container = document.getElementById(containerId);
            container.innerHTML = '';
            for (const key in data) {
                const div = document.createElement('div');
                div.className = 'form-field';
                const label = document.createElement('label');
                label.setAttribute('for', key);
                label.textContent = key.replace(/_/g, ' ');
                const input = document.createElement('input');
                input.type = 'number';
                input.step = '0.01';
                input.id = key;
                input.name = key;
                input.value = data[key];
                div.appendChild(label);
                div.appendChild(input);
                container.appendChild(div);
            }
        } catch (error) {
            console.error(`Error loading ${configName} config:`, error);
        }
    }
    
    function setupFormSubmit(configName, formId) {
        const form = document.getElementById(formId);
        form.addEventListener('submit', async (event) => {
            event.preventDefault();
            const button = form.querySelector('button');
            button.textContent = 'Saving...';
            const formData = new FormData(form);
            const jsonData = Object.fromEntries(formData.entries());
            // Convert string values to numbers
            for (const key in jsonData) {
                jsonData[key] = parseFloat(jsonData[key]);
            }
            try {
                const response = await fetch(`/api/save/${configName}`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(jsonData, null, 2)
                });
                if (!response.ok) throw new Error('Save failed');
                button.textContent = 'Saved! ✅';
                setTimeout(() => { button.textContent = `Apply Tank Params`; }, 2000);
            } catch (error) {
                button.textContent = 'Save Failed! ❌';
            }
        });
    }
});
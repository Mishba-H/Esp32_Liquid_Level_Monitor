document.addEventListener('DOMContentLoaded', () => {

    const configs = [
        { name: 'network', containerId: 'network-fields', formId: 'network-form' },
        { name: 'pins', containerId: 'pin-fields', formId: 'pin-form' },
        { name: 'tasks', containerId: 'task-fields', formId: 'task-form' }
    ];

    configs.forEach(config => {
        loadAndRenderConfig(config.name, config.containerId);
        setupFormSubmit(config.name, config.formId);
    });

    async function loadAndRenderConfig(configName, containerId) {
        try {
            const response = await fetch(`/api/${configName}`);
            if (!response.ok) throw new Error(`Failed to fetch ${configName}`);
            const data = await response.json();
            const container = document.getElementById(containerId);
            container.innerHTML = '';
            createFormFields(data, container);
        } catch (error) {
            console.error(`Error loading ${configName} config:`, error);
            document.getElementById(containerId).innerHTML = `<p style="color:red;">Error loading configuration.</p>`;
        }
    }

    function createFormFields(obj, parentElement, prefix = '') {
        for (const key in obj) {
            if (key === 'default_mode') continue;
            const value = obj[key];
            const inputName = prefix ? `${prefix}.${key}` : key;
            if (typeof value === 'object' && value !== null) {
                const fieldset = document.createElement('fieldset');
                const legend = document.createElement('legend');
                legend.textContent = key.replace(/([A-Z])/g, ' $1').replace(/_/g, ' ');
                fieldset.appendChild(legend);
                parentElement.appendChild(fieldset);
                createFormFields(value, fieldset, inputName);
            } else {
                const div = document.createElement('div');
                div.className = 'form-field';
                const label = document.createElement('label');
                label.setAttribute('for', inputName);
                label.textContent = key.replace(/([A-Z])/g, ' $1').replace(/_/g, ' ');
                const input = document.createElement('input');
                input.type = (typeof value === 'number') ? 'number' : 'text';
                input.id = inputName;
                input.name = inputName;
                input.value = value;
                div.appendChild(label);
                div.appendChild(input);
                parentElement.appendChild(div);
            }
        }
    }

    function setupFormSubmit(configName, formId) {
        const form = document.getElementById(formId);
        form.addEventListener('submit', async (event) => {
            event.preventDefault();
            const button = form.querySelector('button');
            button.textContent = 'Saving...';
            const formData = new FormData(form);
            const jsonData = {};
            for (const [key, value] of formData.entries()) {
                const keys = key.split('.');
                keys.reduce((acc, currentKey, index) => {
                    if (index === keys.length - 1) {
                        acc[currentKey] = isNaN(value) || value === '' ? value : Number(value);
                    } else {
                        acc[currentKey] = acc[currentKey] || {};
                    }
                    return acc[currentKey];
                }, jsonData);
            }
            try {
                const response = await fetch(`/api/save/${configName}`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(jsonData, null, 2)
                });
                if (!response.ok) throw new Error('Save failed');
                button.textContent = 'Saved! ✅';
                setTimeout(() => { button.textContent = `Apply ${configName.charAt(0).toUpperCase() + configName.slice(1)}`; }, 2000);
            } catch (error) {
                console.error(`Error saving ${configName} config:`, error);
                button.textContent = 'Save Failed! ❌';
            }
        });
    }
});
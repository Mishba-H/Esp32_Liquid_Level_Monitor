document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('tank-form');
    const errorMessage = document.getElementById('error-message');

    form.addEventListener('submit', async (event) => {
        event.preventDefault();
        const button = form.querySelector('button[type="submit"]');
        button.textContent = 'Saving...';
        errorMessage.style.display = 'none';

        const formData = new FormData(form);
        const jsonData = {};
        formData.forEach((value, key) => {
            jsonData[key] = parseFloat(value) || 0;
        });

        try {
            const response = await fetch('/api/save/tank', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(jsonData)
            });

            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(errorText);
            }
            window.location.href = '/'; // Success, redirect to monitor page
        } catch (error) {
            errorMessage.textContent = error.message;
            errorMessage.style.display = 'block';
            button.textContent = 'Apply & Save';
        }
    });
});
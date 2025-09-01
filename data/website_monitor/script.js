document.addEventListener('DOMContentLoaded', () => {
    const gateway = `ws://${window.location.hostname}/ws`;
    let websocket;

    function initWebSocket() {
        websocket = new WebSocket(gateway);
        websocket.onopen = () => console.log('Connection opened');
        websocket.onclose = () => setTimeout(initWebSocket, 2000);
        websocket.onmessage = (event) => {
            const data = JSON.parse(event.data);
            
            // --- UI Update with Unit Conversion ---
            const volumeLiters = data.volume / 1000.0;
            const capacityLiters = data.capacity / 1000.0;
            
            document.getElementById('water-level').style.height = `${data.percentage.toFixed(1)}%`;
            document.getElementById('percentage').textContent = `${data.percentage.toFixed(1)} %`;
            document.getElementById('volume').textContent = `${volumeLiters.toFixed(1)} / ${capacityLiters.toFixed(1)} L`;
        };
    }
    initWebSocket();
});
/* Plumbing shared by the settings pages: the modal shown while the device
 * reboots, the number-input listeners, and the "wait until it answers again"
 * helpers. Loaded only by the pages that can restart the device. */

var overlay;
var timerId;

/* The modal lives here, so no page has to carry a copy of its markup. */
(function () {
    function inject() {
        var o = document.createElement('div');
        o.id = 'overlay';
        o.className = 'overlay';
        var n = document.createElement('div');
        n.id = 'reconnectNotification';
        n.className = 'reconnect-notification';
        n.innerHTML = '<div class="reconnect-content">' +
            '<div class="reconnect-spinner"></div>' +
            '<div id="reconnectMessage" class="reconnect-message"></div></div>';
        document.body.appendChild(o);
        document.body.appendChild(n);
        overlay = o;
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', inject);
    } else {
        inject();
    }
})();

/* Stop whatever the page polls before the device goes away. */
function stopTimers() {
    clearTimeout(timerId);
    if (typeof stopLiveView === 'function') {
        stopLiveView();
    }
}

function setupInputListeners() {
    return new Promise(resolve => {
        var ipInputs = document.querySelectorAll('input[type="number"]');
        ipInputs.forEach(function (input) {
            // Handle European decimal separator (comma) by replacing with dot
            input.addEventListener('keypress', function (e) {
                if (e.key === ',') {
                    e.preventDefault();
                    const cursorPos = this.selectionStart;
                    const currentValue = this.value;
                    // Insert a dot at cursor position
                    this.value = currentValue.slice(0, cursorPos) + '.' + currentValue.slice(cursorPos);
                    // Move cursor after the inserted dot
                    this.setSelectionRange(cursorPos + 1, cursorPos + 1);
                }
            });

            // Validation and bounds correction when leaving the field
            input.addEventListener('blur', function () {
                if (this.value === "" || this.value === "." || this.value === "-" || this.value === "-.") {
                    this.value = this.min; // Default to min if empty or invalid
                    return;
                }

                const isFloat = this.step && parseFloat(this.step) !== parseInt(this.step);
                let num = isFloat ? parseFloat(this.value) : parseInt(this.value, 10);

                if (isNaN(num)) {
                    this.value = this.min;
                } else if (num > parseFloat(this.max)) {
                    this.value = this.max;
                } else if (num < parseFloat(this.min)) {
                    this.value = this.min;
                }
            });
        });
        resolve();
    });
}
function showReconnectNotification(message) {
    var notification = document.getElementById('reconnectNotification');
    var messageElement = document.getElementById('reconnectMessage');
    messageElement.textContent = message;
    notification.style.display = 'block';
}
function hideReconnectNotification() {
    var notification = document.getElementById('reconnectNotification');
    notification.style.display = 'none';
}
function updateReconnectMessage(message) {
    var messageElement = document.getElementById('reconnectMessage');
    messageElement.textContent = message;
}
function waitForDeviceAndReload(expectedDPI) {
    var startTime = Date.now();
    var maxWaitTime = 30000; // Maximum 30 seconds
    var initialDelay = 5000; // Wait 5 seconds before first check (device is rebooting)
    var consecutiveSuccessfulReads = 0;
    var requiredSuccessfulReads = 3; // Require 3 consecutive successful reads with correct DPI

    setTimeout(function checkDevice() {
        // Check if we've exceeded max wait time
        if (Date.now() - startTime > maxWaitTime) {
            console.log("Timeout waiting for device, reloading anyway...");
            window.location.href = window.location.pathname + '?nocache=' + new Date().getTime();
            return;
        }

        // Try to reach the device
        var xhr = new XMLHttpRequest();
        xhr.timeout = 2000; // 2 second timeout per attempt
        xhr.open("GET", "/getDPI?nocache=" + new Date().getTime(), true);

        xhr.onload = function () {
            if (xhr.status === 200) {
                var currentDPI = xhr.responseText.trim();
                console.log("Device responded with DPI:", currentDPI);

                // Check if DPI matches expected value
                if (currentDPI === expectedDPI) {
                    consecutiveSuccessfulReads++;
                    console.log("DPI matches expected value (" + consecutiveSuccessfulReads + "/" + requiredSuccessfulReads + ")");

                    if (consecutiveSuccessfulReads >= requiredSuccessfulReads) {
                        console.log("Device ready with correct DPI confirmed, reloading page...");
                        // Reload the page to ensure UI is in sync with device state
                        window.location.href = window.location.pathname + '?nocache=' + new Date().getTime();
                    } else {
                        // Need more confirmations
                        setTimeout(checkDevice, 500);
                    }
                } else {
                    console.log("DPI not yet updated (got " + currentDPI + ", expected " + expectedDPI + "), retrying in 1s...");
                    consecutiveSuccessfulReads = 0; // Reset counter
                    setTimeout(checkDevice, 1000);
                }
            } else {
                // Device responded but with error, retry
                console.log("Device not ready yet (status: " + xhr.status + "), retrying in 1s...");
                consecutiveSuccessfulReads = 0; // Reset counter
                setTimeout(checkDevice, 1000);
            }
        };

        xhr.onerror = function () {
            // Device not reachable yet, retry
            console.log("Device not reachable yet, retrying in 1s...");
            consecutiveSuccessfulReads = 0; // Reset counter
            setTimeout(checkDevice, 1000);
        };

        xhr.ontimeout = function () {
            // Request timed out, retry
            console.log("Request timeout, retrying in 1s...");
            consecutiveSuccessfulReads = 0; // Reset counter
            setTimeout(checkDevice, 1000);
        };

        xhr.send();
    }, initialDelay);
}
function waitForNetworkChangeAndReload(newIP) {
    var startTime = Date.now();
    var maxWaitTime = 45000; // Maximum 45 seconds (longer for network change)
    var initialDelay = 10000; // Wait 10 seconds before first check (device is rebooting and reconfiguring network)
    var consecutiveSuccessfulReads = 0;
    var requiredSuccessfulReads = 3; // Require 3 consecutive successful reads

    setTimeout(function checkDevice() {
        // Check if we've exceeded max wait time
        if (Date.now() - startTime > maxWaitTime) {
            console.log("Timeout waiting for device at new IP, redirecting anyway...");
            window.location.href = 'http://' + newIP + window.location.pathname + '?nocache=' + new Date().getTime();
            return;
        }

        // Try to reach the device at new IP
        var xhr = new XMLHttpRequest();
        xhr.timeout = 3000; // 3 second timeout per attempt
        xhr.open("GET", "http://" + newIP + "/getFirmwareVersion?nocache=" + new Date().getTime(), true);

        xhr.onload = function () {
            if (xhr.status === 200) {
                consecutiveSuccessfulReads++;
                console.log("Device responded at new IP (" + consecutiveSuccessfulReads + "/" + requiredSuccessfulReads + ")");

                if (consecutiveSuccessfulReads >= requiredSuccessfulReads) {
                    console.log("Device ready at new IP confirmed, reloading page...");
                    // Reload the page at new IP
                    window.location.href = 'http://' + newIP + window.location.pathname + '?nocache=' + new Date().getTime();
                } else {
                    // Need more confirmations
                    setTimeout(checkDevice, 1000);
                }
            } else {
                // Device responded but with error, retry
                console.log("Device not ready yet at new IP (status: " + xhr.status + "), retrying in 2s...");
                consecutiveSuccessfulReads = 0; // Reset counter
                setTimeout(checkDevice, 2000);
            }
        };

        xhr.onerror = function () {
            // Device not reachable yet at new IP, retry
            console.log("Device not reachable yet at new IP, retrying in 2s...");
            consecutiveSuccessfulReads = 0; // Reset counter
            setTimeout(checkDevice, 2000);
        };

        xhr.ontimeout = function () {
            // Request timed out, retry
            console.log("Request timeout for new IP, retrying in 2s...");
            consecutiveSuccessfulReads = 0; // Reset counter
            setTimeout(checkDevice, 2000);
        };

        xhr.send();
    }, initialDelay);
}
function waitForFactoryResetAndReload(defaultIP) {
    var startTime = Date.now();
    var maxWaitTime = 45000; // Maximum 45 seconds
    var initialDelay = 10000; // Wait 10 seconds before first check (device is rebooting and resetting)
    var consecutiveSuccessfulReads = 0;
    var requiredSuccessfulReads = 3; // Require 3 consecutive successful reads

    setTimeout(function checkDevice() {
        // Check if we've exceeded max wait time
        if (Date.now() - startTime > maxWaitTime) {
            console.log("Timeout waiting for device at default IP, redirecting anyway...");
            window.location.href = 'http://' + defaultIP + window.location.pathname + '?nocache=' + new Date().getTime();
            return;
        }

        // Try to reach the device at default IP
        var xhr = new XMLHttpRequest();
        xhr.timeout = 3000; // 3 second timeout per attempt
        xhr.open("GET", "http://" + defaultIP + "/getFirmwareVersion?nocache=" + new Date().getTime(), true);

        xhr.onload = function () {
            if (xhr.status === 200) {
                consecutiveSuccessfulReads++;
                console.log("Device responded at default IP (" + consecutiveSuccessfulReads + "/" + requiredSuccessfulReads + ")");

                if (consecutiveSuccessfulReads >= requiredSuccessfulReads) {
                    console.log("Device ready at default IP confirmed, reloading page...");
                    // Reload the page at default IP
                    window.location.href = 'http://' + defaultIP + window.location.pathname + '?nocache=' + new Date().getTime();
                } else {
                    // Need more confirmations
                    setTimeout(checkDevice, 1000);
                }
            } else {
                // Device responded but with error, retry
                console.log("Device not ready yet at default IP (status: " + xhr.status + "), retrying in 2s...");
                consecutiveSuccessfulReads = 0; // Reset counter
                setTimeout(checkDevice, 2000);
            }
        };

        xhr.onerror = function () {
            // Device not reachable yet at default IP, retry
            console.log("Device not reachable yet at default IP, retrying in 2s...");
            consecutiveSuccessfulReads = 0; // Reset counter
            setTimeout(checkDevice, 2000);
        };

        xhr.ontimeout = function () {
            // Request timed out, retry
            console.log("Request timeout for default IP, retrying in 2s...");
            consecutiveSuccessfulReads = 0; // Reset counter
            setTimeout(checkDevice, 2000);
        };

        xhr.send();
    }, initialDelay);
}

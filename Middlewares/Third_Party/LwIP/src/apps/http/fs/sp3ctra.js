/* Shared shell for every device page: logo header + tab bar + device identity.
 * A page only declares which tab it is (<body data-page="scan">) and loads this
 * file; the markup lives here once instead of in four copies of the flash. */
(function () {
    var TABS = [
        { page: 'scan', href: 'scan.html', label: 'SCAN' },
        { page: 'imu', href: 'imu.html', label: 'IMU' },
        { page: 'network', href: 'network.html', label: 'NETWORK' },
        { page: 'gui', href: 'gui.html', label: 'GUI' },
        { page: 'update', href: 'update.html', label: 'UPDATE' }
    ];

    function build() {
        var here = document.body.getAttribute('data-page');

        var header = document.createElement('div');
        header.className = 'app-header';
        header.innerHTML = '<a href="scan.html"><img src="img/Sp3ctra.png" alt="Sp3ctra"></a>' +
            '<div class="app-id" id="appId">&nbsp;</div>';

        var bar = document.createElement('nav');
        bar.className = 'tabbar';
        for (var i = 0; i < TABS.length; i++) {
            var a = document.createElement('a');
            a.className = 'tab' + (TABS[i].page === here ? ' active' : '');
            a.href = TABS[i].href;
            a.textContent = TABS[i].label;
            bar.appendChild(a);
        }

        document.body.insertBefore(bar, document.body.firstChild);
        document.body.insertBefore(header, document.body.firstChild);

        /* Identity block, right of the logo: who this device is and its link state. */
        var xhr = new XMLHttpRequest();
        xhr.open('GET', '/getDeviceInfo', true);
        xhr.onload = function () {
            if (xhr.status !== 200) { return; }
            try {
                var d = JSON.parse(xhr.responseText);
                document.getElementById('appId').innerHTML =
                    '<b>' + d.name + '</b> &middot; ' + d.serial + '<br>' +
                    'fw ' + d.fw + ' &middot; ' + (d.bound ? 'host ' + d.peer : 'no host');
            } catch (e) { /* keep the placeholder */ }
        };
        xhr.send();
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', build);
    } else {
        build();
    }
})();

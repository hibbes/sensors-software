function setSSID(ssid) {
	document.getElementById('wlanssid').value = ssid.innerText || ssid.textContent;
	document.getElementById('wlanpwd').focus();
}
function load_wifi_list() {
	var x = new XMLHttpRequest();
	x.open('GET', '/wifi');
	x.onload = function () {
		if (x.status === 200) {
			document.getElementById('wifilist').innerHTML = x.responseText;
		}
	};
	x.send();
}

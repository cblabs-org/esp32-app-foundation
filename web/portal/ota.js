// web/portal/ota.js

(function() {
  const container = document.getElementById('tab-ota');
  container.innerHTML = `
    <article>
      <header>Over-the-Air Update</header>
      <p style="font-size: 0.9rem; color: var(--text-muted); margin-bottom: 1rem;">
        Select a compiled firmware binary (.bin) to upload to the device. The device will reboot automatically upon success.
      </p>
      
      <input type="file" id="ota-file" accept=".bin" style="background: transparent; border: none; padding: 0; margin-bottom: 1.5rem;">
      <button id="btn-ota-start">Upload Firmware</button>
      
      <div id="ota-status" style="margin-top: 1rem; font-weight: bold; text-align: center;"></div>
    </article>
  `;

  document.getElementById('btn-ota-start').addEventListener('click', async () => {
    const fileInput = document.getElementById('ota-file');
    const statusEl = document.getElementById('ota-status');
    
    if (!fileInput.files.length) {
      statusEl.style.color = 'var(--error)';
      statusEl.innerText = 'Please select a file first.';
      return;
    }
    
    const file = fileInput.files[0];
    const formData = new FormData();
    formData.append('firmware', file);
    
    statusEl.style.color = 'var(--primary)';
    statusEl.innerText = 'Uploading... Please do not disconnect.';
    
    try {
      // Use the serialized API fetch for OTA as well
      await window.apiFetch('/update', {
        method: 'POST',
        body: formData
      });
      statusEl.style.color = 'var(--success)';
      statusEl.innerText = 'Update successful! Device is rebooting.';
      fileInput.value = '';
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      statusEl.innerText = 'Update failed. Check connection or file validity.';
    }
  });
})();

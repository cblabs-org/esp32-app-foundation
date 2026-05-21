// web/portal/app.js

/**
 * ApiQueue - Serializes API requests to protect the ESP32 socket pool.
 * No concurrent XHR or fetch requests are allowed.
 */
class ApiQueue {
  constructor() {
    this.queue = [];
    this.isProcessing = false;
  }

  async enqueue(url, options = {}) {
    return new Promise((resolve, reject) => {
      this.queue.push({ url, options, resolve, reject });
      this.processNext();
    });
  }

  async processNext() {
    if (this.isProcessing || this.queue.length === 0) {
      return;
    }
    
    this.isProcessing = true;
    const { url, options, resolve, reject } = this.queue.shift();
    
    try {
      const response = await fetch(url, options);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }
      
      // Attempt to parse JSON, fallback to text if parsing fails
      const contentType = response.headers.get("content-type");
      if (contentType && contentType.indexOf("application/json") !== -1) {
        const data = await response.json();
        resolve(data);
      } else {
        const text = await response.text();
        resolve(text);
      }
    } catch (error) {
      console.error(`API Error for ${url}:`, error);
      reject(error);
    } finally {
      this.isProcessing = false;
      // Yield to event loop before processing next to avoid blocking UI
      setTimeout(() => this.processNext(), 0);
    }
  }
}

// Global instance for all fragments to use
const api = new ApiQueue();
window.apiFetch = (url, options) => api.enqueue(url, options);

// Tab and Lifecycle Management
document.addEventListener('DOMContentLoaded', () => {
  const tabs = document.querySelectorAll('.tab-link');
  const contents = document.querySelectorAll('.tab-content');

  tabs.forEach(tab => {
    tab.addEventListener('click', (e) => {
      e.preventDefault();
      
      // Remove active classes
      tabs.forEach(t => t.classList.remove('active'));
      contents.forEach(c => c.classList.remove('active'));
      
      // Add active to selected
      tab.classList.add('active');
      const targetId = tab.getAttribute('data-target');
      document.getElementById(targetId).classList.add('active');
      
      // Dispatch custom event for fragments to listen to
      window.dispatchEvent(new CustomEvent('tab-changed', { detail: targetId }));
    });
  });

  // Load LittleFS app fragments dynamically
  const scripts = ['wifi.js', 'system.js', 'ota.js'];
  scripts.forEach(src => {
    const script = document.createElement('script');
    script.src = src;
    document.body.appendChild(script);
  });
  
  // Check for AP mode
  window.apiFetch('/api/status').then(res => {
    if (res && res.apMode) {
      document.body.classList.add('ap-mode');
    }
  }).catch(() => {
    console.warn("Failed to fetch initial status");
  });
});

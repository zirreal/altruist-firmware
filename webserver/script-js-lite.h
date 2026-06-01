#ifndef _SCRIPT_JS_LITE_H
#define _SCRIPT_JS_LITE_H

/** Config page JS for ESP32-C3 Urban: tabs only (no Leaflet / map SVG). Coords field stays manual. */
const char WEB_PAGE_STATIC_JS_CONFIG[] PROGMEM = R"rawliteral(
document.addEventListener('DOMContentLoaded', () => {
  var tabs = document.querySelectorAll('.tab');
  var panels = document.querySelectorAll('.panel');
  function showTab(id) {
    panels.forEach(function(p) { p.classList.remove('active'); });
    tabs.forEach(function(t) { t.style.background = '#f4f4f4'; });
    if (panels[id - 1]) panels[id - 1].classList.add('active');
    if (tabs[id - 1]) tabs[id - 1].style.background = '#ddd';
  }
  if (tabs.length && panels.length) {
    tabs.forEach(function(tab) {
      tab.style.background = '#f4f4f4';
      tab.addEventListener('click', function(e) {
        showTab(parseInt(e.target.dataset.id, 10));
      });
    });
    showTab(1);
  }
});
)rawliteral";

#endif // _SCRIPT_JS_LITE_H

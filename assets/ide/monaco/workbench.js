(function () {
  var host = document.getElementById('editorHost');
  var saveBtn = document.getElementById('saveBtn');
  var runBtn = document.getElementById('runBtn');
  var fileLabel = document.getElementById('fileLabel');
  var hoverLabel = document.getElementById('hoverLabel');
  var diagnosticBadge = document.getElementById('diagnosticBadge');

  var editor = null;
  var setText = function () {};
  var getText = function () { return ''; };

  var bridge = {
    emit: function (payload) {
      try {
        if (window.__JUCE__ && window.__JUCE__.backend && window.__JUCE__.backend.emitEvent) {
          window.__JUCE__.backend.emitEvent('orionFrontend', payload);
        }
      } catch (e) {}
    },
    listen: function (handler) {
      try {
        if (window.__JUCE__ && window.__JUCE__.backend && window.__JUCE__.backend.addEventListener) {
          window.__JUCE__.backend.addEventListener('orionBackend', handler);
        }
      } catch (e) {}
    }
  };

  function safeAddEvent(el, name, fn) {
    if (!el) return;
    if (el.addEventListener) el.addEventListener(name, fn, false);
    else if (el.attachEvent) el.attachEvent('on' + name, fn);
  }

  function emitContentChanged() {
    bridge.emit({ type: 'contentChanged', content: getText() });
  }

  function extractTokenFromText(text, index) {
    if (!text || index < 0 || index > text.length) return '';
    var left = text.slice(0, index).match(/[A-Za-z0-9_\.]+$/);
    return left ? left[0] : '';
  }

  function notifyHoverToken(token) {
    if (!token) return;
    bridge.emit({ type: 'hoverSymbol', symbol: token });
  }

  function setupFallbackEditor() {
    var textarea = document.createElement('textarea');
    textarea.id = 'fallbackEditor';
    host.innerHTML = '';
    host.appendChild(textarea);

    setText = function (v) { textarea.value = v || ''; };
    getText = function () { return textarea.value; };

    safeAddEvent(textarea, 'input', emitContentChanged);
    safeAddEvent(textarea, 'keyup', function () {
      var token = extractTokenFromText(textarea.value, textarea.selectionStart || 0);
      notifyHoverToken(token);
    });

    editor = textarea;
  }

  function loadScript(src, onDone, onFail) {
    var el = document.createElement('script');
    el.src = src;
    el.async = true;
    el.onload = function () { if (onDone) onDone(); };
    el.onerror = function () { if (onFail) onFail(); };
    (document.head || document.getElementsByTagName('head')[0]).appendChild(el);
  }

  function startMonaco(monacoPath) {
    if (!window.require) {
      setupFallbackEditor();
      return;
    }

    try {
      window.require.config({ paths: { vs: monacoPath } });
      window.require(['vs/editor/editor.main'], function () {
        host.innerHTML = '';
        editor = monaco.editor.create(host, {
          value: '',
          language: 'lua',
          automaticLayout: true,
          minimap: { enabled: true },
          smoothScrolling: true,
          theme: 'vs-dark',
          fontFamily: 'Consolas, monospace',
          fontSize: 13,
          lineHeight: 20,
          renderLineHighlight: 'all',
          scrollbar: { verticalScrollbarSize: 12, horizontalScrollbarSize: 12 }
        });

        setText = function (v) {
          var m = editor.getModel();
          if (m) m.setValue(v || '');
        };

        getText = function () {
          var m = editor.getModel();
          return m ? m.getValue() : '';
        };

        editor.onDidChangeModelContent(emitContentChanged);
        editor.onDidChangeCursorPosition(function (e) {
          var m = editor.getModel();
          if (!m) return;
          var word = m.getWordAtPosition(e.position);
          if (word && word.word) notifyHoverToken(word.word);
        });

        window.orionRevealPosition = function (line, character) {
          var pos = {
            lineNumber: Math.max(1, (line || 0) + 1),
            column: Math.max(1, (character || 0) + 1)
          };
          editor.revealPositionInCenter(pos);
          editor.setPosition(pos);
          editor.focus();
        };
      }, setupFallbackEditor);
    } catch (e) {
      setupFallbackEditor();
    }
  }

  function setupMonacoEditor() {
    var localPath = window.__orionMonacoPath || './vs';
    var localLoader = (localPath + '/loader.js').replace('//', '/');
    var cdnPath = 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.52.2/min/vs';
    var cdnLoader = 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.52.2/min/vs/loader.min.js';

    if (window.require) {
      startMonaco(localPath);
      return;
    }

    loadScript(localLoader,
      function () { startMonaco(localPath); },
      function () {
        loadScript(cdnLoader,
          function () { startMonaco(cdnPath); },
          function () { setupFallbackEditor(); });
      });
  }

  safeAddEvent(saveBtn, 'click', function () {
    bridge.emit({ type: 'saveRequested' });
  });

  safeAddEvent(runBtn, 'click', function () {
    bridge.emit({ type: 'runRequested' });
  });

  bridge.listen(function (payload) {
    if (!payload || typeof payload !== 'object') return;

    switch (payload.type) {
      case 'setDocument':
        setText(payload.content || '');
        if (fileLabel) fileLabel.innerHTML = payload.name || payload.path || 'Untitled';
        break;
      case 'theme':
        try {
          if (payload.background) document.body.style.backgroundColor = payload.background;
          if (payload.text) document.body.style.color = payload.text;
        } catch (e) {}
        break;
      case 'dictionary':
        if (hoverLabel) hoverLabel.innerHTML = payload.signature || payload.symbol || 'Hover a symbol for docs';
        break;
      case 'diagnostics':
        var count = (payload.items && payload.items.length) ? payload.items.length : 0;
        if (diagnosticBadge) diagnosticBadge.innerHTML = count + (count === 1 ? ' problem' : ' problems');
        break;
      case 'revealPosition':
        if (window.orionRevealPosition) {
          window.orionRevealPosition(payload.line || 0, payload.character || 0);
        }
        break;
      default:
        break;
    }
  });

  window.orionGetContent = function () {
    return getText();
  };

  setupMonacoEditor();
  bridge.emit({ type: 'ready' });
})();

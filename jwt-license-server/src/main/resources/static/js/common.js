// ========================================
// JWT License Admin - Common JS Library
// ========================================

(function () {
  const API_BASE = '/api';
  const TOKEN_KEY = 'jwt-admin-token';
  const USER_KEY = 'jwt-admin-user';

  // ---- Token / Auth ----

  // 从 Cookie 读取指定名称的值
  function getCookie(name) {
    if (!document.cookie) return null;
    var cookies = document.cookie.split(';');
    for (var i = 0; i < cookies.length; i++) {
      var cookie = cookies[i].trim();
      if (cookie.indexOf(name + '=') === 0) {
        return decodeURIComponent(cookie.substring(name.length + 1));
      }
    }
    return null;
  }

  window.getToken = function () {
    // 优先从 localStorage 取，兜底从 Cookie 取
    var token = localStorage.getItem(TOKEN_KEY);
    if (token) return token;
    return getCookie(TOKEN_KEY);
  };

  window.setToken = function (token) {
    localStorage.setItem(TOKEN_KEY, token);
  };

  window.clearToken = function () {
    localStorage.removeItem(TOKEN_KEY);
    localStorage.removeItem(USER_KEY);
    // 同时清除 Cookie（多重保险：expires + Max-Age 确保不同浏览器兼容）
    document.cookie = TOKEN_KEY + '=; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0; SameSite=Lax';
  };

  window.getUserInfo = function () {
    var raw = localStorage.getItem(USER_KEY);
    return raw ? JSON.parse(raw) : null;
  };

  window.setUserInfo = function (info) {
    localStorage.setItem(USER_KEY, JSON.stringify(info));
  };

  window.isLoggedIn = function () {
    return !!window.getToken();
  };

  // ---- API Fetch Wrapper ----
  async function request(url, options = {}) {
    const token = window.getToken();
    const headers = {
      'Content-Type': 'application/json',
      ...(token ? { 'Authorization': 'Bearer ' + token } : {}),
      ...options.headers,
    };

    var res;
    try {
      res = await fetch(API_BASE + url, {
        ...options,
        headers,
      });
    } catch (e) {
      // 网络错误（断网、服务器未启动等）
      throw new Error('网络连接失败，请检查网络或服务器状态');
    }

    const data = await res.json().catch(function () { return null; });

    // 安全网：即使 HTTP 200，也检查业务状态码（兼容未使用 ResponseEntity 的旧接口）
    if (data && typeof data.code === 'number' && data.code !== 200) {
      if (data.code === 401) {
        window.clearToken();
        var currentPage = window.location.pathname.split('/').pop();
        if (currentPage !== 'login.html') {
          window.location.href = 'login.html';
        }
      }
      throw new Error(data.message || ('请求失败，错误码：' + data.code));
    }

    if (res.status === 401) {
      window.clearToken();
      // 如果已经在登录页，不再跳转，避免死循环
      var currentPage2 = window.location.pathname.split('/').pop();
      if (currentPage2 !== 'login.html') {
        window.location.href = 'login.html';
      }
      // 优先使用后端返回的消息（如"用户名或密码错误"），否则使用通用消息
      throw new Error(data?.message || '登录已过期，请重新登录');
    }

    if (!res.ok) {
      // 优先使用后端返回的消息，其次根据状态码给出友好提示
      var msg = data?.message;
      if (!msg) {
        if (res.status === 403) msg = '没有权限执行此操作';
        else if (res.status === 404) msg = '请求的资源不存在';
        else if (res.status === 500) msg = '服务器内部错误，请稍后重试';
        else msg = '请求失败（' + res.status + '）';
      }
      throw new Error(msg);
    }

    return data;
  }

  window.apiGet = function (url, params) {
    const qs = new URLSearchParams(params).toString();
    return request(url + (qs ? '?' + qs : ''));
  };

  window.apiPost = function (url, body, extra) {
    var opts = { method: 'POST', body: JSON.stringify(body) };
    if (extra) {
      if (extra.signal) opts.signal = extra.signal;
      if (extra.headers) Object.assign(opts.headers, extra.headers);
    }
    return request(url, opts);
  };

  window.apiPut = function (url, body) {
    return request(url, { method: 'PUT', body: JSON.stringify(body) });
  };

  window.apiPatch = function (url, body, extra) {
    var opts = { method: 'PATCH' };
    if (extra && extra.params) {
      var qs = new URLSearchParams(extra.params).toString();
      url = url + (url.indexOf('?') !== -1 ? '&' : '?') + qs;
    }
    if (body) {
      opts.body = JSON.stringify(body);
    }
    return request(url, opts);
  };

  window.apiDelete = function (url) {
    return request(url, { method: 'DELETE' });
  };

  // ---- Page Init Helpers ----
  function ensureLogin() {
    if (!window.isLoggedIn()) {
      window.location.href = 'login.html';
      return false;
    }
    return true;
  }

  // ---- Sidebar Toggle ----
  function initSidebar() {
    const toggle = document.getElementById('menuToggle');
    const sidebar = document.getElementById('sidebar');
    const overlay = document.getElementById('sidebarOverlay');
    if (!toggle || !sidebar || !overlay) return;
    toggle.addEventListener('click', function () {
      sidebar.classList.toggle('open');
      overlay.hidden = !sidebar.classList.contains('open');
    });
    overlay.addEventListener('click', function () {
      sidebar.classList.remove('open');
      overlay.hidden = true;
    });
  }

  // ---- Active Nav ----
  function setActiveNav() {
    const pathname = window.location.pathname.split('/').pop();
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(function (item) {
      const href = item.getAttribute('href');
      if (href === pathname) {
        item.classList.add('active');
      } else {
        item.classList.remove('active');
      }
    });
  }

  // ---- Toast ----
  window.toast = function (msg, type) {
    type = type || 'success';
    var container = document.getElementById('toastContainer');
    if (!container) {
      container = document.createElement('div');
      container.id = 'toastContainer';
      container.className = 'toast-container';
      document.body.appendChild(container);
    }
    var t = document.createElement('div');
    t.className = 'toast' + (type === 'error' ? ' error' : type === 'warning' ? ' warning' : '');
    // 兜底样式，确保颜色生效
    t.style.cssText = 'display:flex;align-items:center;gap:10px;padding:14px 20px;border-radius:8px;'
      + 'box-shadow:0 4px 16px rgba(0,0,0,.12);animation:toast-in .2s ease;max-width:420px;line-height:1.5;'
      + 'color:#fff;font-size:15px;font-weight:500;';
    if (type === 'error') t.style.background = '#d32f2f';
    else if (type === 'warning') t.style.background = '#d97706';
    else t.style.background = '#111111';
    var icon;
    if (type === 'success') {
      icon = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>';
    } else if (type === 'error') {
      icon = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>';
    } else {
      icon = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>';
    }
    t.innerHTML = icon + escapeHtmlText(msg);
    container.appendChild(t);
    setTimeout(function () {
      t.style.opacity = '0';
      t.style.transform = 'translateY(6px)';
      t.style.transition = 'opacity .2s ease, transform .2s ease';
      setTimeout(function () { t.remove(); }, 300);
    }, 2800);
  };

  function escapeHtmlText(s) {
    if (!s) return '';
    var d = document.createElement('div');
    d.textContent = s;
    return d.innerHTML;
  }

  // ---- Copy ----
  window.copyText = function (text, el) {
    if (navigator.clipboard && window.isSecureContext) {
      navigator.clipboard.writeText(text).then(function () {
        el.textContent = '已复制';
        window.toast('已复制到剪贴板');
        setTimeout(function () { el.textContent = '复制'; }, 1500);
      });
    } else {
      var ta = document.createElement('textarea');
      ta.value = text;
      ta.style.cssText = 'position:fixed;opacity:0';
      document.body.appendChild(ta);
      ta.select();
      document.execCommand('copy');
      ta.remove();
      el.textContent = '已复制';
      window.toast('已复制到剪贴板');
      setTimeout(function () { el.textContent = '复制'; }, 1500);
    }
  };

  // ---- Confirm ----
  window.confirmAction = function (msg) {
    return confirm(msg || '确认执行此操作？');
  };

  // ---- Loading ----
  window.showLoading = function (container) {
    if (!container) container = document.body;
    container.innerHTML = '<div class="empty-state"><div class="icon">⏳</div><h3>加载中...</h3></div>';
  };

  // ---- Init ----
  initSidebar();
  setActiveNav();

  // Export for module usage
  window.Common = {
    ensureLogin: ensureLogin,
    API_BASE: API_BASE,
    request: request,
    initSidebar: initSidebar,
    setActiveNav: setActiveNav,
  };
})();

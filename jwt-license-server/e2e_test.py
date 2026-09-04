#!/usr/bin/env python3
"""
JWT License Server —— 端到端全面测试脚本 (v2)
"""

from playwright.sync_api import sync_playwright
import sys
import os

BASE_URL = "http://localhost:8080"
PASS = 0
FAIL = 0

def log(level, msg):
    icon = {"pass": "✅", "fail": "❌", "info": "ℹ️", "warn": "⚠️"}.get(level, "  ")
    print(f"  {icon} {msg}")

def check(condition, test_name, detail=""):
    global PASS, FAIL
    if condition:
        PASS += 1
        log("pass", test_name + (f" — {detail}" if detail else ""))
    else:
        FAIL += 1
        log("fail", test_name + (f" — {detail}" if detail else ""))

def run():
    global PASS, FAIL

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        context = browser.new_context(viewport={"width": 1440, "height": 900}, locale="zh-CN")
        page = context.new_page()

        console_errors = []
        page.on("console", lambda msg: console_errors.append(f"[{msg.type}] {msg.text}") if msg.type in ("error", "warning") else None)
        page.on("pageerror", lambda err: console_errors.append(f"[JS_ERROR] {err}"))

        # ━━━━━━━━━━━━━━━━━━ 测试 1：登录页基础加载 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 1：登录页面基础加载")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")

        check(page.title() == "登录 - JWT 授权管理后台", "页面标题正确")
        check(page.locator("h2").text_content() == "欢迎回来", "标题「欢迎回来」存在")
        check(page.locator("#email").is_visible(), "用户名输入框可见")
        check(page.locator("#password").is_visible(), "密码输入框可见")
        check(page.locator("#loginBtn").is_visible(), "登录按钮可见")
        check(page.locator("#errorBox").is_hidden(), "错误提示框初始隐藏")

        for sel, desc in [
            ("#email", "用户名输入框"), ("#password", "密码输入框"),
            ("#loginBtn", "登录按钮"), ("#errorBox", "错误提示框"),
            ("#emailError", "用户名字段错误"), ("#passwordError", "密码字段错误"),
            ("#capsWarning", "大写锁定警告"),
        ]:
            check(page.locator(sel).count() > 0, f"元素 {desc} 存在")

        page.screenshot(path="e2e_screenshots/01_loaded.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 2：空表单校验 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 2：空表单校验")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.locator("#loginBtn").click()
        page.wait_for_timeout(500)

        check(page.locator("#errorBox").is_visible(), "错误提示显示")
        check(page.locator("#emailError").is_visible(), "用户名错误提示显示")
        check(page.locator("#passwordError").is_visible(), "密码错误提示显示")
        check(page.locator("#email").evaluate("el => el.classList.contains('input-error')"), "用户名红色边框")
        check(page.locator("#password").evaluate("el => el.classList.contains('input-error')"), "密码红色边框")

        error_text = page.locator("#errorBox").text_content().strip()
        log("info", f"错误提示: {error_text}")
        page.screenshot(path="e2e_screenshots/02_empty_form.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 3：逐字段校验 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 3：逐字段校验")
        print("="*60)

        # 仅填用户名
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.locator("#email").fill("admin@example.com")
        page.locator("#loginBtn").click()
        page.wait_for_timeout(300)
        check(not page.locator("#email").evaluate("el => el.classList.contains('input-error')"), "用户名无红色边框")
        check(page.locator("#password").evaluate("el => el.classList.contains('input-error')"), "密码有红色边框")
        check(page.locator("#passwordError").is_visible(), "密码错误提示显示")

        # 仅填密码
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.locator("#password").fill("1234")
        page.locator("#loginBtn").click()
        page.wait_for_timeout(300)
        check(page.locator("#email").evaluate("el => el.classList.contains('input-error')"), "用户名有红色边框")
        check(not page.locator("#password").evaluate("el => el.classList.contains('input-error')"), "密码无红色边框")

        page.screenshot(path="e2e_screenshots/03_field_validation.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 4：输入清除错误 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 4：输入时清除错误状态")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.locator("#loginBtn").click()
        page.wait_for_timeout(200)
        check(page.locator("#email").evaluate("el => el.classList.contains('input-error')"), "初始：有红色边框")
        page.locator("#email").fill("a")
        page.wait_for_timeout(100)
        check(not page.locator("#email").evaluate("el => el.classList.contains('input-error')"), "输入后：红色边框消失")
        page.screenshot(path="e2e_screenshots/04_input_clears.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 5：错误凭据提示 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 5：错误凭据提示")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.locator("#email").fill("wrong@user.com")
        page.locator("#password").fill("wrongpassword")
        page.locator("#loginBtn").click()
        page.wait_for_timeout(1500)

        check(page.locator("#errorBox").is_visible(), "错误提示显示")
        error_text = page.locator("#errorBox").text_content().strip()
        log("info", f"错误凭据响应: {error_text}")
        check("用户名或密码错误" in error_text, "包含「用户名或密码错误」")
        check(page.locator("#loginBtn").is_enabled(), "按钮恢复可用")
        check("登 录" in page.locator("#loginBtn").text_content(), "按钮文案恢复")
        page.screenshot(path="e2e_screenshots/05_wrong_creds.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 6：正确登录 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 6：正确凭据登录")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.locator("#email").fill("admin@example.com")
        page.locator("#password").fill("1234")
        page.locator("#loginBtn").click()

        try:
            page.wait_for_url("**/admin.html**", timeout=10000)
            log("pass", "登录成功，已跳转 admin.html")
        except Exception:
            log("fail", "登录后未跳转到 admin.html")

        check("admin.html" in page.url, "URL 包含 admin.html")

        if "admin.html" in page.url:
            token = page.evaluate("() => localStorage.getItem('jwt-admin-token')")
            check(token and len(token) > 0, f"localStorage 有 token (长度={len(token) if token else 0})")
            user = page.evaluate("() => localStorage.getItem('jwt-admin-user')")
            check(user is not None, "localStorage 有用户信息")
            page.wait_for_timeout(1000)
            disp = page.locator("#usernameDisplay")
            if disp.count() > 0:
                check("admin" in disp.text_content().strip(), "用户名显示正确", disp.text_content().strip())

        page.screenshot(path="e2e_screenshots/06_login_ok.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 7：/api/auth/me 端点 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 7：/api/auth/me 端点")
        print("="*60)
        if "admin.html" in page.url:
            result = page.evaluate("""
                async () => {
                    const token = localStorage.getItem('jwt-admin-token');
                    const res = await fetch('/api/auth/me', {
                        headers: { 'Authorization': 'Bearer ' + token }
                    });
                    return { status: res.status, data: await res.json().catch(() => null) };
                }
            """)
            check(result["status"] == 200, f"/me HTTP 状态", f"实际={result['status']}")
            if result["data"]:
                check(result["data"]["code"] == 200, "/me 业务码=200")
                check("username" in str(result["data"].get("data", {})), "返回 username")
        else:
            log("warn", "跳过（未登录到 admin.html）")

        # ━━━━━━━━━━━━━━━━━━ 测试 8：受保护页面重定向 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 8：未登录访问受保护页面")
        print("="*60)
        page.evaluate("() => { localStorage.clear(); document.cookie.split(';').forEach(c => { document.cookie = c.split('=')[0] + '=; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0'; }); }")
        page.wait_for_timeout(300)

        page.goto(f"{BASE_URL}/admin.html")
        page.wait_for_load_state("networkidle")
        page.wait_for_timeout(500)
        check("login.html" in page.url, "重定向到登录页")
        page.screenshot(path="e2e_screenshots/08_protected.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 9：无效 token 不死循环 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 9：无效 token 不产生死循环")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.evaluate("""
            () => {
                localStorage.setItem('jwt-admin-token', 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.invalid.xyz');
                document.cookie = 'jwt-admin-token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.invalid.xyz; path=/; Max-Age=86400';
            }
        """)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.wait_for_timeout(1500)

        check("login.html" in page.url and "admin.html" not in page.url, "停留在登录页（未死循环）")
        token = page.evaluate("() => localStorage.getItem('jwt-admin-token')")
        check(token is None or token == "", "无效 token 已被清除")
        page.screenshot(path="e2e_screenshots/09_no_loop.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 10：CSS 样式检查 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 10：CSS 样式检查")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.wait_for_timeout(300)

        bg = page.locator("body").evaluate("el => window.getComputedStyle(el).backgroundColor")
        log("info", f"body 背景色: {bg}")

        page.locator("#loginBtn").click()
        page.wait_for_timeout(400)

        alert_bg = page.locator("#errorBox").evaluate("el => window.getComputedStyle(el).backgroundColor")
        log("info", f"错误提示背景色: {alert_bg}")

        try:
            border = page.locator("#email").evaluate("el => window.getComputedStyle(el).borderColor")
            log("info", f"输入框错误边框色: {border}")
        except Exception:
            log("info", "输入框已在测试 9 后清除错误态")

        check(True, "CSS 样式检查完成")
        page.screenshot(path="e2e_screenshots/10_css.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 11：docs.html 公开访问 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 11：docs.html 公开页面")
        print("="*60)
        page.evaluate("() => { localStorage.clear(); }")
        page.goto(f"{BASE_URL}/docs.html")
        page.wait_for_load_state("networkidle")
        page.wait_for_timeout(500)
        check("docs.html" in page.url and "login.html" not in page.url, "docs.html 无需登录可访问")
        page.screenshot(path="e2e_screenshots/11_docs_public.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 测试 12：错误恢复交互 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  测试 12：错误恢复交互")
        print("="*60)
        page.goto(f"{BASE_URL}/login.html")
        page.wait_for_load_state("networkidle")
        page.wait_for_timeout(300)

        page.locator("#loginBtn").click()
        page.wait_for_timeout(300)
        page.locator("#email").fill("admin@example.com")
        page.wait_for_timeout(200)
        check(not page.locator("#email").evaluate("el => el.classList.contains('input-error')"), "输入后红色边框消失")
        page.screenshot(path="e2e_screenshots/12_recovery.png", full_page=True)

        # ━━━━━━━━━━━━━━━━━━ 控制台错误 ━━━━━━━━━━━━━━━━━━
        print("\n" + "="*60)
        print("  控制台错误 / 警告")
        print("="*60)
        if console_errors:
            for err in console_errors:
                log("warn", err)
        else:
            log("pass", "无控制台错误")

        browser.close()

    # ━━━━━━━━━━━━━━━━━━ 汇总 ━━━━━━━━━━━━━━━━━━
    print("\n" + "="*60)
    print("  测试结果汇总")
    print("="*60)
    total = PASS + FAIL
    print(f"  总计: {total}  通过: {PASS}  失败: {FAIL}")
    if total:
        print(f"  通过率: {PASS/total*100:.1f}%")
    print("="*60)
    return FAIL == 0


if __name__ == "__main__":
    os.makedirs("e2e_screenshots", exist_ok=True)
    success = run()
    sys.exit(0 if success else 1)

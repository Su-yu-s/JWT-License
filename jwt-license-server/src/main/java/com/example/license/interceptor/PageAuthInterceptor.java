package com.example.license.interceptor;

import jakarta.servlet.http.Cookie;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

/**
 * 页面拦截器：未登录 .html 页面重定向到登录页
 * 已登录的请求放行，让 Spring 静态资源处理器处理 .html 文件
 */
@Component
public class PageAuthInterceptor implements HandlerInterceptor {

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) throws Exception {
        String uri = request.getRequestURI();

        // 登录页和文档页直接放行
        if (uri.endsWith("/login.html") || uri.endsWith("/docs.html")) {
            return true;
        }

        // 检查 token（优先从 Cookie 取，其次从 URL 参数取，最后从 Authorization header 取）
        String token = getToken(request);
        if (token == null || token.isBlank()) {
            // 未登录，重定向到登录页
            response.sendRedirect("/login.html");
            return false;
        }

        // 有 token 放行，由静态资源处理器返回 .html
        return true;
    }

    private String getToken(HttpServletRequest request) {
        // 1. 从 Cookie 取（主要方式）
        Cookie[] cookies = request.getCookies();
        if (cookies != null) {
            for (Cookie c : cookies) {
                if ("jwt-admin-token".equals(c.getName())) {
                    return c.getValue();
                }
            }
        }
        // 2. 从 URL 参数取（兜底：刷新/直连场景）
        String tokenParam = request.getParameter("token");
        if (tokenParam != null && !tokenParam.isBlank()) {
            return tokenParam;
        }
        // 3. 从 Authorization header 取（兜底）
        String authHeader = request.getHeader("Authorization");
        if (authHeader != null && authHeader.startsWith("Bearer ")) {
            return authHeader.substring(7);
        }
        return null;
    }
}
